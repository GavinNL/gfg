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
    struct Frame : public FrameBase {
        VkFramebuffer frameBuffer;
        VkRenderPass  renderPass;
        //std::vector<VkImageView> inputAttachments;
    };

    struct VKNodeInfo {
        VkFramebuffer            frameBuffer;
        VkRenderPass             renderPass;
        std::vector<VkImageView> inputAttachments;
        uint32_t                 width  = 0;
        uint32_t                 height = 0;
        bool                     isInit = false;
    };

    struct VKImageInfo
    {
        VkImage     image     = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;

        uint32_t   width     = 0;
        uint32_t   height    = 0;

        bool       resizable = true;

        VkImageCreateInfo info = {};
        VmaAllocation     allocation = {};
        VmaAllocationInfo allocInfo  = {};
        VkImageViewType viewType = {};

        VkSampler linearSampler  = {};
        VkSampler nearestSampler = {};
    };

    std::map<std::string, VKNodeInfo>                   _nodes;
    std::map<std::string, VKImageInfo>                  _imageNames;
    std::map<std::string, std::function<void(Frame &)>> _renderers;
    std::vector<std::string>                            _execOrder;


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
#if 0
                if(_nodes[x].framebuffer)
                {
                    gl::glDeleteFramebuffers(1, &_nodes[x].framebuffer);
                    _nodes[x].framebuffer = 0u;
                    _nodes[x].isInit = false;
                }
#endif
            }
        }

        // delete all the images
        for (auto it = _imageNames.begin(); it != _imageNames.end();)
        {
            auto &img = it->second;
            if(img.resizable)
            {
#if 0
                if(img.textureID)
                {
                    gl::glDeleteTextures(1, &img.textureID);
                    img.textureID = 0;
                    spdlog::info("Image Deleted: {}", it->first);
                    it = _imageNames.erase(it);
                    continue;
                }
#endif
            }
            ++it;
        }

        _nodes.clear();
        _imageNames.clear();
        _execOrder.clear();
    }

    void resize(FrameGraph &G, uint32_t width, uint32_t height)
    {
        VmaAllocator m_allocator = nullptr;
        VkDevice m_device = nullptr;


        _execOrder = G.findExecutionOrder();
        // first go through all the images that have already been
        // created and destroy the ones that are not resizable
        for(auto it = _imageNames.begin(); it!=_imageNames.end();)
        {
            auto &img = it->second;
            if(img.resizable)
            {
#if 0
                if(img.textureID)
                {
                    gl::glDeleteTextures(1, &img.textureID);
                    img.textureID = 0;
                    it = _imageNames.erase(it);
                    spdlog::info("Image Deleted: {}", it->first);
                    continue;
                }
#endif
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
                _imageNames[name] =  image_Create(m_device,
                                                  m_allocator,
                                                  {iDef.width,iDef.height,1},
                                                  static_cast<VkFormat>(iDef.format),
                                                  VK_IMAGE_VIEW_TYPE_2D,
                                                  1,
                                                  1,
                                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

                _imageNames[name].width     = iDef.width;
                _imageNames[name].height    = iDef.height;
                _imageNames[name].resizable = resizable;
                spdlog::info("Image Created: {}   {}x{}", name, iDef.width, iDef.height);
            }


            spdlog::info("Texture2D created: {}", name);
        }
#if 0
        for (auto &name : _execOrder)
        {
            auto &Nv = G.getNodes().at(name);

            if (std::holds_alternative<RenderPassNode>(Nv))
            {
                auto &N       = std::get<RenderPassNode>(Nv);
                auto &_glNode = this->_nodes[name];

                for (auto r : N.inputRenderTargets)
                {
                    auto &RTN = std::get<RenderTargetNode>(G.getNodes().at(r.name));

                    auto  imgName = RTN.imageResource.name;
                    auto &imgDef  = G.getImages().at(imgName);
                    auto  imgID   = _imageNames.at(imgName).textureID;

                    _glNode.inputAttachments.push_back(imgID);
                }

                // if there are no output render targets
                // then this means this render pass is the
                // final pass that will render to the swapchain/window
                if (N.outputRenderTargets.size() == 0)
                {
                    _glNode.width = width;
                    _glNode.height = height;
                    continue;
                }

                if(_glNode.framebuffer==0)
                {
                    gl::GLuint framebuffer;
                    gl::glGenFramebuffers(1, &framebuffer);
                    _glNode.framebuffer = framebuffer;
                }

                gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, _glNode.framebuffer );

                uint32_t i = 0;

                for (auto r : N.outputRenderTargets)
                {
                    auto &RTN = std::get<RenderTargetNode>(G.getNodes().at(r.name));

                    auto  imgName = RTN.imageResource.name;
                    auto &imgDef  = G.getImages().at(imgName);
                    auto  imgID   = _imageNames.at(imgName).textureID;

                    _glNode.width  = _imageNames.at(imgName).width;
                    _glNode.height = _imageNames.at(imgName).height;
                    gl::glBindTexture(gl::GL_TEXTURE_2D, imgID);

                    if( imgDef.format == FrameGraphFormat::D32_SFLOAT ||
                            imgDef.format == FrameGraphFormat::D24_UNORM_S8_UINT ||
                            imgDef.format == FrameGraphFormat::D32_SFLOAT_S8_UINT)
                    {
                        gl::glFramebufferTexture2D( gl::GL_FRAMEBUFFER,
                                                    gl::GL_DEPTH_ATTACHMENT,
                                                    gl::GL_TEXTURE_2D,
                                                    imgID,
                                                    0);
                    }
                    else
                    {
                        gl::glFramebufferTexture2D( gl::GL_FRAMEBUFFER,
                                                    gl::GL_COLOR_ATTACHMENT0 + i,
                                                    gl::GL_TEXTURE_2D,
                                                    imgID, 0);
                        ++i;

                    }                                                                          // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
                }

                if (gl::glCheckFramebufferStatus(gl::GL_FRAMEBUFFER) != gl::GL_FRAMEBUFFER_COMPLETE)
                    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

                _glNode.isInit = true;
                gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
            }
        }
    #endif
    }

    void operator()(FrameGraph & G)
    {
        for(auto & x : _execOrder)
        {
            auto & N = G.getNodes().at(x);
            if( std::holds_alternative<RenderPassNode>(N))
            {
                auto &R = _renderers.at(x);
                Frame F;
#if 0
                F.frameBuffer      = _nodes.at(x).framebuffer;
                F.inputAttachments = _nodes.at(x).inputAttachments;
                F.width            = _nodes.at(x).width;
                F.height           = _nodes.at(x).height;
#endif
                R(F);
            }
        }
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
