#ifndef GNL_FRAME_GRAPH_VULKAN_H
#define GNL_FRAME_GRAPH_VULKAN_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "../frameGraph.h"
#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

struct FrameGraphExecutor_Vulkan
{
    constexpr static uint32_t maxInputTextures = 10;

    struct Frame : public FrameBase {
        VkCommandBuffer          commandBuffer;
        VkFramebuffer            frameBuffer;
        VkRenderPass             renderPass;
        std::vector<VkImageView> inputAttachments;
        VkDescriptorSet          inputAttachmentSet;
        VkDescriptorSetLayout    inputAttachmentSetLayout;

        std::vector<VkClearValue> clearValue;

        void beginRenderPass()
        {
            VkRenderPassBeginInfo render_pass_info = {};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            render_pass_info.renderPass        = renderPass;
            render_pass_info.framebuffer       = frameBuffer;
            render_pass_info.renderArea.offset = {0, 0};
            render_pass_info.renderArea.extent = {renderableWidth, renderableHeight};
            render_pass_info.clearValueCount   = 1;

            render_pass_info.clearValueCount = static_cast<uint32_t>(clearValue.size());
            render_pass_info.pClearValues = clearValue.data();

            vkCmdBeginRenderPass(commandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        }

        void endRenderPass()
        {
            vkCmdEndRenderPass(commandBuffer);
        }
    };

    struct VKNodeInfo {
        VkFramebuffer            frameBuffer;
        VkRenderPass             renderPass;
        std::vector<VkImageView> inputAttachments;
        uint32_t                 width  = 0;
        uint32_t                 height = 0;
        bool                     isInit = false;

        VkDescriptorPool             descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet              descriptorSet = VK_NULL_HANDLE;
    };

    struct VKImageInfo
    {
        VkImage     image     = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;

        uint32_t   width     = 0;
        uint32_t   height    = 0;

        bool       resizable = true;

        VkImageCreateInfo info       = {};
        VmaAllocation     allocation = {};
        VmaAllocationInfo allocInfo  = {};
        VkImageViewType   viewType   = {};

        VkSampler linearSampler  = {};
        VkSampler nearestSampler = {};

        std::vector<VkDescriptorImageInfo> _imageInfo; // for writes
    };

    struct RenderInfo
    {
        VkCommandBuffer commandBuffer;
        uint32_t swapchainWidth;
        uint32_t swapchainHeight;
        VkImageView  swapchainImage;
        VkImageView  swapchainDepthImage = VK_NULL_HANDLE;
        VkFramebuffer swapchainFrameBuffer;
        VkRenderPass  swapchainRenderPass;
    };

    std::map<std::string, VKNodeInfo>                   _nodes;
    std::map<std::string, VKImageInfo>                  _imageNames;
    std::map<std::string, std::function<void(Frame &)>> _renderers;
    std::vector<std::string>                            _execOrder;

    VKImageInfo           m_nullImage;
    VkDescriptorSetLayout m_dsetLayout = VK_NULL_HANDLE;
    VkDevice              m_device     = VK_NULL_HANDLE;
    VmaAllocator          m_allocator  = VK_NULL_HANDLE;

    void init(VmaAllocator allocator, VkDevice device)
    {
        m_allocator = allocator;
        m_device = device;
    }
    /**
     * @brief setRenderer
     * @param renderPassName
     * @param f
     *
     * Set a renderer for the renderpass.
     */
    void setRenderer(std::string const& renderPassName, std::function<void(Frame&)> f)
    {
        _renderers[renderPassName] = f;
    }

    /**
     * @brief init
     * @param G
     *
     */
    void initGraphResources(FrameGraph &G)
    {
        // To DO: We should create the descriptor pool
        // over here
        auto order = G.findExecutionOrder();
        for (auto &x : order)
        {
            auto &n = G.getNodes().at(x);
            if (std::holds_alternative<RenderPassNode>(n))
            {
                auto &N = std::get<RenderPassNode>(n);

                for (auto &oT : N.outputRenderTargets)
                {
                }
                _nodes[x].isInit = true;
                //
                // 1. Generate the images
                // 2. Generate the render pass
                // 3. generate the frame buffer
                // 4. generate the descriptor set
            }
        }
    }

    void releaseGraphResources(FrameGraph &G)
    {
        auto order = G.findExecutionOrder();
        std::reverse(order.begin(), order.end());
        for (auto &x : order)
        {
            auto &n = G.getNodes().at(x);
            if (std::holds_alternative<RenderPassNode>(n))
            {
                auto &N = std::get<RenderPassNode>(n);
                _destroyNode(_nodes[x], true);
                _nodes[x].isInit = false;
            }
        }

        vkDestroyDescriptorSetLayout(m_device, m_dsetLayout,nullptr);
        m_dsetLayout = VK_NULL_HANDLE;

        // delete all the images
        for (auto it = _imageNames.begin(); it != _imageNames.end();)
        {
            auto &img = it->second;

            if(img.image)
            {
                _destroyImage(img);
                spdlog::info("Image Deleted: {}", it->first);
                it = _imageNames.erase(it);
                continue;
            }
            ++it;
        }

        _destroyImage(m_nullImage);
        m_nullImage = {};
        _nodes.clear();
        _imageNames.clear();
        _execOrder.clear();
    }

    /**
     * @brief resize
     * @param G
     * @param width
     * @param height
     *
     * Resizes the framgraph
     */
    void resize(FrameGraph &G, uint32_t width, uint32_t height)
    {
        _execOrder = G.findExecutionOrder();

        if(m_nullImage.image == VK_NULL_HANDLE)
        {
            VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            m_nullImage       =  image_Create(m_device,
                                              m_allocator,
                                              {8,8,1},
                                              VK_FORMAT_R8G8B8A8_UNORM,
                                              VK_IMAGE_VIEW_TYPE_2D,
                                              1,
                                              1,
                                              usage);
        }
        _createDescriptorSetLayout();

        // first go through all the images that have already been
        // created and destroy the ones that are not resizable
        for(auto it = _imageNames.begin(); it!=_imageNames.end();)
        {
            auto &img = it->second;
            if(img.resizable)
            {
                if(img.image)
                {
                    _destroyImage(img);
                    spdlog::info("Image Deleted: {}", it->first);
                    it = _imageNames.erase(it);
                    continue;
                }
            }
            ++it;
        }

        // Second, go through all the images that need to be created
        // and create/recreate them.
        for (auto &[name, imgDef] : G.getImages())
        {
            bool multisampled = false;
            int  samples      = 1;
            bool resizable    = false;
            auto iDef         = imgDef;

            if (iDef.width * iDef.height == 0)
            {
                iDef.width  = width;
                iDef.height = height;
                resizable = true;
            }
            if(_imageNames.count(name) == 0)
            {
                VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
                if( iDef.format == FrameGraphFormat::D32_SFLOAT ||
                        iDef.format == FrameGraphFormat::D32_SFLOAT_S8_UINT ||
                        iDef.format == FrameGraphFormat::D24_UNORM_S8_UINT )
                {
                    usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                }
                else
                {
                    usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                }

                _imageNames[name] =  image_Create(m_device,
                                                  m_allocator,
                                                  {iDef.width,iDef.height,1},
                                                  static_cast<VkFormat>(iDef.format),
                                                  VK_IMAGE_VIEW_TYPE_2D,
                                                  1,
                                                  1,
                                                  usage);

                _imageNames[name].width     = iDef.width;
                _imageNames[name].height    = iDef.height;
                _imageNames[name].resizable = resizable;
                spdlog::info("Image Created: {}   {}x{}", name, iDef.width, iDef.height);
            }
        }

        for (auto &name : _execOrder)
        {
            auto &Nv = G.getNodes().at(name);

            if (std::holds_alternative<RenderPassNode>(Nv))
            {
                auto &N       = std::get<RenderPassNode>(Nv);
                auto &_glNode = this->_nodes[name];

                _destroyNode(_glNode, false); // don't destroy the renderpass, but destroy FB

                _createFrameBuffer(G, N);

                _updateSets(G, N);
                _glNode.isInit = true;
            }
        }
    }

    void operator()(FrameGraph & G, RenderInfo const & Ri)
    {
        for(auto & x : _execOrder)
        {
            auto & N = G.getNodes().at(x);
            if( std::holds_alternative<RenderPassNode>(N))
            {
                auto &R = _renderers.at(x);
                Frame F;
                auto &NN  = _nodes.at(x);
                auto &RPN = std::get<RenderPassNode>(N);

                F.windowWidth    = Ri.swapchainWidth;
                F.windowHeight   = Ri.swapchainHeight;

                if( RPN.outputRenderTargets.size())
                {
                    for(auto & f : RPN.outputRenderTargets)
                    {
                        auto &cv = F.clearValue.emplace_back();
                        if( !isDepth(f.format) )
                        {
                            cv.color.float32[0] = 0.0f;
                            cv.color.float32[1] = 0.0f;
                            cv.color.float32[2] = 0.0f;
                            cv.color.float32[3] = 0.0f;
                        }
                        else
                        {
                            cv.depthStencil.stencil = 0;
                            cv.depthStencil.depth = 1.0f;
                        }

                        auto &GN           = G.getNodes();
                        auto &rtn          = GN.at(f.name);
                        auto &v            = std::get<RenderTargetNode>(rtn);
                        auto  extent       = _imageNames.at(v.imageResource.name).info.extent;
                        F.imageWidth       = extent.width;
                        F.imageHeight      = extent.height;
                        F.renderableWidth  = extent.width;
                        F.renderableHeight = extent.height;
                    }

                    F.commandBuffer    = Ri.commandBuffer;
                    F.frameBuffer      = NN.frameBuffer;
                    F.renderPass       = NN.renderPass;
                    F.inputAttachments = NN.inputAttachments;
                    //F.imageWidth       = NN.width;
                    //F.imageHeight      = NN.height;
                    F.inputAttachmentSet = NN.descriptorSet;

                    F.inputAttachmentSetLayout = NN.inputAttachments.size() == 0 ? VK_NULL_HANDLE : m_dsetLayout;

                    //F.renderableWidth       = NN.width;
                    //F.renderableHeight      = NN.height;
                }
                else
                {
                    F.clearValue.emplace_back();
                    if(Ri.swapchainDepthImage != VK_NULL_HANDLE)
                    {
                        auto & cv = F.clearValue.emplace_back();
                        cv.depthStencil.stencil = 0;
                        cv.depthStencil.depth = 1.0f;
                    }

                    F.commandBuffer      = Ri.commandBuffer;
                    F.frameBuffer        = Ri.swapchainFrameBuffer;
                    F.renderPass         = Ri.swapchainRenderPass;
                    F.inputAttachments   = NN.inputAttachments;
                    F.imageWidth         = Ri.swapchainWidth;
                    F.imageHeight        = Ri.swapchainHeight;
                    F.renderableWidth    = Ri.swapchainWidth;
                    F.renderableHeight   = Ri.swapchainHeight;
                    F.inputAttachmentSet = NN.descriptorSet;
                    F.inputAttachmentSetLayout = NN.inputAttachments.size() == 0 ? VK_NULL_HANDLE : m_dsetLayout;
                }

                R(F);
            }
        }
    }
protected:
    void _destroyImage(VKImageInfo &img)
    {
        if(img.imageView)
            vkDestroyImageView(m_device, img.imageView, nullptr);
        if(img.image)
            vmaDestroyImage(m_allocator, img.image, img.allocation);
        if(img.linearSampler)
            vkDestroySampler(m_device, img.linearSampler, nullptr);
        if(img.nearestSampler)
            vkDestroySampler(m_device, img.nearestSampler, nullptr);

        img.imageView      = VK_NULL_HANDLE;
        img.image          = VK_NULL_HANDLE;
        img.linearSampler  = VK_NULL_HANDLE;
        img.nearestSampler = VK_NULL_HANDLE;
    }

    void _destroyNode(VKNodeInfo & N, bool destroyRenderPass)
    {
        if(N.frameBuffer)
            vkDestroyFramebuffer(m_device, N.frameBuffer, nullptr);

        N.inputAttachments.clear();
        N.width = 0;
        N.height = 0;

        N.frameBuffer = VK_NULL_HANDLE;

        if( destroyRenderPass )
        {
            if(N.renderPass)
                vkDestroyRenderPass(m_device, N.renderPass, nullptr);
            N.renderPass = VK_NULL_HANDLE;

            vkDestroyDescriptorPool(m_device, N.descriptorPool, nullptr);
            N.descriptorPool = VK_NULL_HANDLE;
            N.descriptorSet = VK_NULL_HANDLE;
        }
        N.isInit = false;

    }


    void _createDescriptorSetLayout()
    {
        if(m_dsetLayout != VK_NULL_HANDLE)
            return;

        VkDescriptorSetLayoutBinding binding = {};
        binding.binding                      = 0;
        binding.descriptorCount              = maxInputTextures;
        binding.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo ci = {};
        ci.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ci.flags                           = {};
        ci.pBindings                       = &binding;
        ci.bindingCount                    = 1;

        VkDescriptorSetLayout l = {};
        {
            auto res = vkCreateDescriptorSetLayout(m_device, &ci, nullptr, &l);
            if (res != VK_SUCCESS)
            {
                std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl;
                assert(res == VK_SUCCESS);
            }
            m_dsetLayout = l;
        }

    }

    void _updateSets(FrameGraph & G, RenderPassNode const & N)
    {
        auto & out       = _nodes[N.name];

        if(N.inputRenderTargets.size() == 0)
            return;

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        //std::vector<VkDescriptorImageInfo> _imageInfo(maxInputTextures, VkDescriptorImageInfo{m_nullImage.linearSampler, m_nullImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        std::vector<VkDescriptorImageInfo> _imageInfo;//(maxInputTextures, VkDescriptorImageInfo{m_nullImage.linearSampler, m_nullImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        uint32_t i=0;
        for (auto r : N.inputRenderTargets)
        {
            auto &RTN = std::get<RenderTargetNode>(G.getNodes().at(r.name));

            auto  imgName = RTN.imageResource.name;
            auto &imgDef  = G.getImages().at(imgName);
            auto &imgID   = _imageNames.at(imgName);

            auto &ii = _imageInfo.emplace_back();//.at(i);

            ii.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ii.imageView   = imgID.imageView;
            ii.sampler     = imgID.nearestSampler;

            i++;
        }
        while(_imageInfo.size() < maxInputTextures)
            _imageInfo.push_back(_imageInfo.back());

        write.pImageInfo      = _imageInfo.data();
        write.descriptorCount = _imageInfo.size();
        write.dstArrayElement = 0;
        write.dstSet          = out.descriptorSet;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        spdlog::info("Input Set updated, {}", N.name);

        vkUpdateDescriptorSets(m_device,1, &write,0,nullptr);

    }

    void _createFrameBuffer(FrameGraph & G,  RenderPassNode const &N)
    {
        auto & out       = _nodes[N.name];
        out.inputAttachments.clear();

        for (auto r : N.inputRenderTargets)
        {
            auto &RTN = std::get<RenderTargetNode>(G.getNodes().at(r.name));

            auto  imgName = RTN.imageResource.name;
            auto &imgDef  = G.getImages().at(imgName);
            auto &imgID   = _imageNames.at(imgName);

             out.inputAttachments.push_back(imgID.imageView);
        }

        out.width       = N.width;
        out.height      = N.height;

        if(out.renderPass == VK_NULL_HANDLE)
            out.renderPass  = _createRenderPass(G, N);

        out.frameBuffer = _createFrameBuffer(G, N, out.renderPass);

        if(out.descriptorPool == VK_NULL_HANDLE)
        {
            out.descriptorPool = _createDescriptorPool(G, N);

            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorSetCount          = 1;
            allocInfo.pSetLayouts                 = &m_dsetLayout;
            allocInfo.descriptorPool              = out.descriptorPool;

            //for(uint32_t i=0;i<3;i++)
            {
                VkDescriptorSet dSet = {};
                auto res = vkAllocateDescriptorSets(m_device, &allocInfo, &dSet);
                if (res != VK_SUCCESS)
                {
                    std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl;
                    assert(res == VK_SUCCESS);
                }
                out.descriptorSet = dSet;
            }

        }
    }

    VkDescriptorPool _createDescriptorPool(FrameGraph &G, RenderPassNode const & N)
    {
        uint32_t maxSets = 3;
        VkDescriptorPoolCreateInfo Ci = {};

        std::vector<VkDescriptorPoolSize> poolSizes = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxSets * maxInputTextures}
        };

        Ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        Ci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        Ci.maxSets       = maxSets;
        Ci.poolSizeCount = poolSizes.size();
        Ci.pPoolSizes    = poolSizes.data();

        VkDescriptorPool pool = VK_NULL_HANDLE;
        {
            auto res = vkCreateDescriptorPool(m_device, &Ci, nullptr, &pool);
            if (res != VK_SUCCESS)
            {
                std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl;
                assert(res == VK_SUCCESS);
            }
        }
        return pool;
    }

    [[nodiscard]] VkFramebuffer _createFrameBuffer(FrameGraph & G,
                                                   RenderPassNode const &N,
                                                   VkRenderPass renderPass) const
    {
        std::vector<VkImageView> attachments;

        if(N.outputRenderTargets.size() == 0)
            return VK_NULL_HANDLE;

        uint32_t imgWidth  = 0;
        uint32_t imgHeight = 0;

        for(auto & r : N.outputRenderTargets)
        {
            auto & RTN     = std::get<RenderTargetNode>(G.getNodes().at(r.name));
            auto   imgName = RTN.imageResource.name;
            auto & imgDef  = G.getImages().at(imgName);
            auto & v       = _imageNames.at(imgName);
            {
                attachments.push_back( v.imageView);
                imgWidth = v.width;
                imgHeight= v.height;
            }

        }

        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext           = nullptr;
        fbufCreateInfo.renderPass      = renderPass;
        fbufCreateInfo.pAttachments    = attachments.data();
        fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbufCreateInfo.width           = imgWidth;
        fbufCreateInfo.height          = imgHeight;
        fbufCreateInfo.layers          = 1;

        VkFramebuffer frameBuffer = VK_NULL_HANDLE;

        {
            auto res = vkCreateFramebuffer(m_device, &fbufCreateInfo, nullptr, &frameBuffer);
            if (res != VK_SUCCESS)
            {
                std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl;
                assert(res == VK_SUCCESS);
            }
        }
        return frameBuffer;

    }

    [[nodiscard]] VkRenderPass _createRenderPass(FrameGraph & G,  RenderPassNode const &N) const
    {
        std::vector<VkAttachmentReference> colorReferences;
        VkAttachmentReference depthReference = {};
        depthReference.attachment = 0;
        depthReference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        std::vector<VkAttachmentDescription> m_attachmentDesc;

        // if there is no output render targets defined
        // return a null handle so that we can use the
        // renderpass provided by the window manager
        if(N.outputRenderTargets.size() == 0)
            return VK_NULL_HANDLE;

        for(auto & irt : N.outputRenderTargets)
        {
            auto & a = m_attachmentDesc.emplace_back();

            a.samples        = VK_SAMPLE_COUNT_1_BIT;
            a.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            a.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            a.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            a.format         = static_cast<VkFormat>(irt.format);

            if ( isDepth(irt.format) )
            {
                a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                a.finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            else
            {
                a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                a.finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        // Init attachment properties
        bool has_depth = false;
        {
            uint32_t i=0;
            for(auto & a : m_attachmentDesc)
            {
                if( !isDepth( static_cast<FrameGraphFormat>(a.format) ))
                {
                    colorReferences.push_back({ i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
                }
                else
                {
                    depthReference.attachment = i;
                    has_depth = true;
                }
                i++;
            }
        }

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pColorAttachments    = colorReferences.data();
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());

        if( has_depth )
        {
            subpass.pDepthStencilAttachment = &depthReference;
        }

        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass      = 0;
        dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass      = 0;
        dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pAttachments    = m_attachmentDesc.data();
        renderPassInfo.attachmentCount = static_cast<uint32_t>(m_attachmentDesc.size());
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies   = dependencies.data();

        VkRenderPass renderPass = VK_NULL_HANDLE;
        {
            auto res = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass);
            if (res != VK_SUCCESS)
            {
                std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl;
                assert(res == VK_SUCCESS);
            }
        }

        return renderPass;
    }





    static
    VKImageInfo       image_Create(  VkDevice device
                             ,VmaAllocator m_allocator
                             ,VkExtent3D extent
                             ,VkFormat format
                             ,VkImageViewType viewType
                             ,uint32_t arrayLayers
                             ,uint32_t miplevels // maximum mip levels
                             ,VkImageUsageFlags additionalUsageFlags)
    {
        VkImageCreateInfo imageInfo{};

        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = extent.depth==1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
        imageInfo.format        = format;
        imageInfo.extent        = extent;
        imageInfo.mipLevels     = miplevels;
        imageInfo.arrayLayers   = arrayLayers;

        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;// vk::SampleCountFlagBits::e1;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;// vk::ImageTiling::eOptimal;
        imageInfo.usage         = additionalUsageFlags | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;// vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;// vk::SharingMode::eExclusive;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;// vk::ImageLayout::eUndefined;

        if( arrayLayers == 6)
            imageInfo.flags |=  VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;// vk::ImageCreateFlagBits::eCubeCompatible;

        VmaAllocationCreateInfo allocCInfo = {};
        allocCInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImage           image;
        VmaAllocation     allocation;
        VmaAllocationInfo allocInfo;

        VkImageCreateInfo & imageInfo_c = imageInfo;
        {
            auto res = vmaCreateImage(m_allocator,  &imageInfo_c,  &allocCInfo, &image, &allocation, &allocInfo);
            if (res != VK_SUCCESS)
            {
                std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl;
                assert(res == VK_SUCCESS);
            }
        }

        VKImageInfo I;
        I.image      = image;
        I.info       = imageInfo;
        I.allocInfo  = allocInfo;
        I.allocation = allocation;
        I.viewType   = viewType;

        // create the image view
        {
            {
                VkImageViewCreateInfo ci{};
                ci.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                ci.image      = I.image;
                ci.viewType   = viewType;
                ci.format     = format;
                ci.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};

                ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

                ci.subresourceRange.baseMipLevel = 0;
                ci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

                ci.subresourceRange.baseArrayLayer = 0;
                ci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

                switch(ci.format)
                {
                    case VK_FORMAT_D16_UNORM:
                    case VK_FORMAT_D32_SFLOAT:
                    case VK_FORMAT_D16_UNORM_S8_UINT:
                    case VK_FORMAT_D24_UNORM_S8_UINT:
                    case VK_FORMAT_D32_SFLOAT_S8_UINT:
                        ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;// vk::ImageAspectFlagBits::eDepth;
                        break;
                    default:
                        ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; //vk::ImageAspectFlagBits::eColor;
                        break;
                }

                {
                    auto res = vkCreateImageView(device, &ci, nullptr, &I.imageView);
                    if (res != VK_SUCCESS)
                    {
                        std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl;
                        assert(res == VK_SUCCESS);
                    }
                }
                // Create one image view per mipmap level
#if 0
                for(uint32_t i=0;i<miplevels;i++)
                {
                    ci.subresourceRange.baseMipLevel = i;
                    ci.subresourceRange.levelCount = 1;

                    VkImageView vv;
                    VKB_CHECK_RESULT( vkCreateImageView(device, &ci, nullptr, &vv) );
                    I.mipMapViews.push_back(vv);
                }
#endif
            }
        }


        // create a sampler
        {
            // Temporary 1-mipmap sampler
            VkSamplerCreateInfo ci;
            ci.sType                   =  VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            ci.magFilter               =  VK_FILTER_LINEAR;//  vk::Filter::eLinear;
            ci.minFilter               =  VK_FILTER_LINEAR;//  vk::Filter::eLinear;
            ci.mipmapMode              =  VK_SAMPLER_MIPMAP_MODE_LINEAR;// vk::SamplerMipmapMode::eLinear ;
            ci.addressModeU            =  VK_SAMPLER_ADDRESS_MODE_REPEAT;//vk::SamplerAddressMode::eRepeat ;
            ci.addressModeV            =  VK_SAMPLER_ADDRESS_MODE_REPEAT;//vk::SamplerAddressMode::eRepeat ;
            ci.addressModeW            =  VK_SAMPLER_ADDRESS_MODE_REPEAT;// vk::SamplerAddressMode::eRepeat ;
            ci.mipLodBias              =  0.0f  ;
            ci.anisotropyEnable        =  VK_FALSE;
            ci.maxAnisotropy           =  1 ;
            ci.compareEnable           =  VK_FALSE ;
            ci.compareOp               =  VK_COMPARE_OP_ALWAYS;// vk::CompareOp::eAlways ;
            ci.minLod                  =  0 ;
            ci.maxLod                  =  VK_LOD_CLAMP_NONE;//static_cast<float>(miplevels);
            ci.borderColor             =  VK_BORDER_COLOR_INT_OPAQUE_BLACK;// vk::BorderColor::eIntOpaqueBlack ;
            ci.unnormalizedCoordinates =  VK_FALSE ;

            ci.magFilter               =  VK_FILTER_LINEAR;//vk::Filter::eLinear;
            ci.minFilter               =  VK_FILTER_LINEAR;//vk::Filter::eLinear;

            vkCreateSampler(device, &ci, nullptr, &I.linearSampler);

            ci.magFilter               =  VK_FILTER_NEAREST;//vk::Filter::eNearest;
            ci.minFilter               =  VK_FILTER_NEAREST;//vk::Filter::eNearest;

            vkCreateSampler(device, &ci, nullptr, &I.nearestSampler);
        }

        return I;
    }
};


#endif
