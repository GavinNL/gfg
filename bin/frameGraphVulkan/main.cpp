#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "frameGraph/frameGraph.h"
#include "frameGraph/executors/VulkanExecutor.h"

#include <gul/MeshPrimitive.h>
#include <gul/math/Transform.h>

#include <vkw/VKWVulkanWindow.h>
#include <vkw/Adapters/SDLVulkanWindowAdapter.h>


#include <vkb/DescriptorSetLayoutCache.h>
#include <vkb/ImageSamplerCache.h>
#include <vkb/PipelineManager.h>
#include <GLSLCompiler.h>

//#include <vkb/SPIRV_DescriptorSetLayoutGenerator.h>

vkb::DescriptorLayoutCache g_layoutCache;

FrameGraph getFrameGraphTwoPassBlur()
{
    FrameGraph G;

    G.createRenderPass("geometryPass")
     .output("C1", FrameGraphFormat::R8G8B8A8_UNORM)
     .output("D1", FrameGraphFormat::D32_SFLOAT);

    G.createRenderPass("HBlur1")
     .setExtent(256,256)
     .input("C1")
     .output("B1h", FrameGraphFormat::R8G8B8A8_UNORM);

    G.createRenderPass("VBlur1")
     .setExtent(256,256)
     .input("B1h")
     .output("B1v",FrameGraphFormat::R8G8B8A8_UNORM);

    G.createRenderPass("Final")
     .input("B1v")
     .input("C1")
   //  .output("F", FrameGraphFormat::R8G8B8A8_UNORM)
     ;

    G.finalize();

    return G;
}

FrameGraph getFrameGraphSimpleDeferred()
{
    FrameGraph G;

    G.createRenderPass("geometryPass")
     .output("C1", FrameGraphFormat::R8G8B8A8_UNORM)
     .output("D1", FrameGraphFormat::D32_SFLOAT)
            ;

    G.createRenderPass("Final")
     .input("C1")
     .input("D1")
     ;

    G.finalize();

    return G;
}

FrameGraph getFrameGraphGeometryOnly()
{
    FrameGraph G;

    G.createRenderPass("geometryPass"); // no outputs, a framebuffer/renderpass(vk) will not be created
                                        // for this. This is assuming that the rendering in this pass
                                        // will be rendering to the swapchain/presentable surface

    G.finalize();

    return G;
}


static const char * vertex_shader =
R"foo(#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_TexCoord_0;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_TexCoord_0;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(push_constant) uniform PushConsts
{
    mat4 u_projection_matrix;
} _pc;

void main()
{
    v_color      = vec4(i_TexCoord_0,0,1);
    v_TexCoord_0 = i_TexCoord_0;
    gl_Position  = _pc.u_projection_matrix * vec4( i_position, 1.0 );
}
)foo";



static const char * fragment_shader =
R"foo(#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_TexCoord_0;

layout(location = 0) out vec4 o_color;

void main() {
    o_color = v_color;
}
)foo";


static const char * fragment_shader_presentImage =
R"foo(#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_TexCoord_0;

layout(location = 0) out vec4 o_color;

layout (set = 0, binding = 0) uniform sampler2D u_Attachment[10];



void main() {
    vec4 c0 = texture( u_Attachment[0], v_TexCoord_0);
    vec4 c1 = texture( u_Attachment[0], v_TexCoord_0);

    o_color = mix(c0,c1,0.0f);
}
)foo";

static const char * fragment_shader_blur =
R"foo(#version 430
    in vec4 v_color;
    in vec2 v_TexCoord_0;

    uniform sampler2D in_Attachment_0;
    uniform vec2 filterDirection;

    out vec4 o_color;

    //float _coefs[9] = float[](0.02853226260337099, 0.06723453549491201, 0.1240093299792275, 0.1790438646174162, 0.2023600146101466, 0.1790438646174162, 0.1240093299792275, 0.06723453549491201, 0.02853226260337099);
//    float _coefs[13] = float[](0.0024055085674964125, 0.009255297393309877, 0.02786684424768963, 0.06566651775036977, 0.12111723251079276, 0.17486827308986305, 0.1976406528809569, 0.17486827308986305, 0.12111723251079276, 0.06566651775036977, 0.02786684424768963, 0.009255297393309877, 0.0024055085674964125);
    float _coefs[13] = float[](0.05158219732758756, 0.08695578132125481, 0.12548561145470394, 0.15502055040385468, 0.16394105419415744, 0.14841941523103785, 0.11502576640056936, 0.0763131810584986, 0.04334093146518183, 0.02107074185604522, 0.008768693979940428, 0.003123618015387646, 0.0009524572917807333);

    int size=13;

    void main() {
        vec2 v = filterDirection;//vec2(0.01);

        vec4 c0 = vec4(0.0f);
        for(int i=0;i<size;i++)
        {
            c0 += _coefs[i] * texture(in_Attachment_0, v_TexCoord_0 + v*float(i-size/2) );
        }

        c0.a = 1.0f;
        o_color = c0;
    }
)foo";




struct Buffer
{
    VkBuffer          buffer = VK_NULL_HANDLE;
    VkDeviceSize      byteSize=0;
    VkDeviceSize      requestedSize=0;
    VmaAllocation     allocation = nullptr;
    VmaAllocationInfo allocInfo;
};

struct Mesh
{
    Buffer buffer;

    uint32_t vertexCount = 0;
    uint32_t indexCount  = 0;
    uint32_t indexByteOffset = 0;

    void destroy(VmaAllocator allocator)
    {
        vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
    }
    void draw(VkCommandBuffer cmd)
    {
        if(indexCount)
            vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);
        else
            vkCmdDraw(cmd, vertexCount,1,0,0);
        //gl::glBindVertexArray( vao );
        //gl::glDrawElements(gl::GL_TRIANGLES, indexCount, gl::GL_UNSIGNED_INT, nullptr );
    }
    void bind(VkCommandBuffer cmd, uint32_t bindingNumber, VkDeviceSize offset)
    {
        vkCmdBindVertexBuffers(cmd, bindingNumber, 1, &buffer.buffer, &offset);
        vkCmdBindIndexBuffer(cmd, buffer.buffer, indexByteOffset, VK_INDEX_TYPE_UINT32);
    }
};

static Buffer _createBuffer(VmaAllocator allocator,
                          size_t bytes,
                          VkBufferUsageFlags usage,
                          VmaMemoryUsage memUsage)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size  = bytes;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocCInfo = {};
    allocCInfo.usage = memUsage;
    allocCInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT; // can always set this bit,
                                                         // vma will not allow device local
                                                         // memory to be mapped

    VkBuffer          buffer;
    VmaAllocation     allocation = nullptr;
    VmaAllocationInfo allocInfo;

    auto result = vmaCreateBuffer(allocator, &bufferInfo, &allocCInfo, &buffer, &allocation, &allocInfo);

    if( result != VK_SUCCESS)
    {
       throw std::runtime_error( "Error allocating VMA Buffer");
    }

    Buffer Buf;
    Buf.byteSize = bytes;

    {
        Buf.buffer        = buffer;
        Buf.allocation    = allocation;
        Buf.allocInfo     = allocInfo;
        Buf.requestedSize = bytes;
    }
    return Buf;
}

Mesh CreateOpenGLMesh(gul::MeshPrimitive const & M, VmaAllocator allocator)
{
    Mesh outMesh;

    auto     totalBufferByteSize = 2*M.calculateDeviceSize();

    outMesh.buffer = _createBuffer(allocator,
                                   totalBufferByteSize,
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                   VMA_MEMORY_USAGE_CPU_TO_GPU); // i dont feel like doing GPU transfers for
                                                                 // this example

    void * mapped = nullptr;
    vmaMapMemory(allocator, outMesh.buffer.allocation, &mapped);

#if 1
    auto offsets = M.copyVertexAttributesInterleaved(mapped);
    M.copyIndex(static_cast<uint8_t*>(mapped)+offsets);
    outMesh.indexByteOffset = offsets;
#else
    auto offsets = M.copySequential(mapped);
    outMesh.indexByteOffset = offsets.back();
#endif

    vmaUnmapMemory(allocator, outMesh.buffer.allocation);

    outMesh.indexCount  = M.indexCount();
    outMesh.vertexCount = M.vertexCount();

    return outMesh;
}


struct Pipeline
{
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    void bind(VkCommandBuffer cmd)
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }
    void pushConstants(VkCommandBuffer cmd, uint32_t size, void * value)
    {
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, size, value);
    }
    void destroy(VkDevice device)
    {
        if (pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
        }
        if (layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, layout, nullptr);
            layout = VK_NULL_HANDLE;
        }
    }
};


VkShaderModule createShader(VkDevice device, char const * glslCOde, EShLanguage lang)
{
    VkShaderModuleCreateInfo ci = {};
    gnl::GLSLCompiler compiler;

    auto spv = compiler.compile(glslCOde,lang);

    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = static_cast<uint32_t>(spv.size() * 4);
    ci.pCode    = spv.data();

    VkShaderModule sh = {};
    auto result = vkCreateShaderModule(device, &ci, nullptr, &sh);
    if(result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed at compiling shader");
    }
    return sh;
}


// get the descriptor set that will be used for input samplers
//
VkDescriptorSetLayout getInputSamplerSet(VkDevice device)
{
    // This Descriptor Set Layout needs to match the one that is epxecte
    vkb::DescriptorLayoutInfo layoutInfo;
    layoutInfo.setDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, FrameGraphExecutor_Vulkan::maxInputTextures, VK_SHADER_STAGE_FRAGMENT_BIT);
    auto layout = g_layoutCache.createDescriptorSetLayout(layoutInfo);
    return layout;
}

Pipeline createPipeline(VkDevice device,
                        char const * _vertex_shader,
                        char const * _fragment_shader,
                        VkRenderPass rp,
                        VkDescriptorSetLayout inputSamplerLayout)
{
    vkb::GraphicsPipelineCreateInfo ci;

    ci.vertexShader   = createShader(device,  _vertex_shader, EShLangVertex);
    ci.fragmentShader = createShader(device, _fragment_shader,EShLangFragment);
    ci.renderPass     = rp;

    //                    format                      shaderLoc, binding, offset, stride
    ci.setVertexAttribute(VK_FORMAT_R32G32B32_SFLOAT, 0, 0, 0              , 8*sizeof(float));
    ci.setVertexAttribute(VK_FORMAT_R32G32B32_SFLOAT, 1, 0, 3*sizeof(float), 8*sizeof(float));
    ci.setVertexAttribute(VK_FORMAT_R32G32_SFLOAT   , 2, 0, 6*sizeof(float), 8*sizeof(float));

    ci.enableDepthTest = true;
    ci.enableDepthWrite = true;

    ci.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    ci.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    {
        VkPipelineLayoutCreateInfo plci = {};

        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        VkPushConstantRange ranges;
        ranges.offset            = 0;
        ranges.size              = 128;
        ranges.stageFlags        = VK_SHADER_STAGE_VERTEX_BIT;
        plci.pPushConstantRanges = &ranges;
        plci.pushConstantRangeCount = 1;

        // This Descriptor Set Layout needs to match the one that is epxecte
        plci.setLayoutCount = inputSamplerLayout == VK_NULL_HANDLE ? 0 : 1;
        plci.pSetLayouts    = &inputSamplerLayout;

        VkPipelineLayout pipelineLayout = {};
        auto result = vkCreatePipelineLayout(device, &plci, nullptr, &pipelineLayout);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed at compiling pipeline");
        }
        ci.pipelineLayout = pipelineLayout;
    }

    auto pipeline = ci.create([&](auto & C)
    {
        VkPipeline p;
        auto result = vkCreateGraphicsPipelines(device, nullptr, 1, &C, nullptr, &p);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed at compiling pipeline");
        }
        return p;
    });


    vkDestroyShaderModule(device, ci.vertexShader, nullptr);
    vkDestroyShaderModule(device, ci.fragmentShader, nullptr);

    Pipeline L;
    L.pipeline = pipeline;
    L.layout = ci.pipelineLayout;
    return L;
}

// callback function for validation layers
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanReportFunc(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)
{
    printf("VULKAN VALIDATION: [%s] %s\n", layerPrefix, msg);

    return VK_FALSE;
}

#if defined(__WIN32__)
int SDL_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    // This needs to be called first to initialize SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // create a default window and initialize all vulkan
    // objects.
    auto window = new vkw::VKWVulkanWindow();
    auto sdl_window = new vkw::SDLVulkanWindowAdapter();

    // 1. create the window and set the adapater
    sdl_window->createWindow("Title", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024,768);
    window->setWindowAdapater(sdl_window);


    // 2. Create the Instance
    vkw::VKWVulkanWindow::InstanceInitilizationInfo2 instanceInfo;
    instanceInfo.debugCallback = &VulkanReportFunc;
    instanceInfo.vulkanVersion = VK_MAKE_VERSION(1, 5, 0);
    instanceInfo.enabledExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    window->createVulkanInstance(instanceInfo);

    // 3. Create the surface
    vkw::VKWVulkanWindow::SurfaceInitilizationInfo2 surfaceInfo;
    surfaceInfo.depthFormat          = VK_FORMAT_D32_SFLOAT_S8_UINT;
    surfaceInfo.presentMode          = VK_PRESENT_MODE_FIFO_KHR;
    surfaceInfo.additionalImageCount = 1;// how many additional swapchain images should we create ( total = min_images + additionalImageCount
    window->createVulkanSurface(surfaceInfo);

    // 4. Create the device
    //    and add additional extensions that we want to enable
    vkw::VKWVulkanWindow::DeviceInitilizationInfo2 deviceInfo;
#if 0
    // set to zero if you want to VKW to choose
    // a device for you
    deviceInfo.deviceID = 0;
#else
    for(auto &d : window->getAvailablePhysicalDevices())
    {
        if(d.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            deviceInfo.deviceID = d.deviceID;
            break;
        }
    }
#endif
    deviceInfo.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    deviceInfo.deviceExtensions.push_back(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);

    // enable a new extended feature
    VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexState = {};
    dynamicVertexState.sType                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
    dynamicVertexState.vertexInputDynamicState  = true;
    deviceInfo.enabledFeatures12.pNext          = &dynamicVertexState;

    window->createVulkanDevice(deviceInfo);

    glslang::InitializeProcess();

    VmaAllocator allocator = {};

    {
        VmaAllocatorCreateInfo aci = {};
        aci.physicalDevice   = window->getPhysicalDevice();
        aci.device           = window->getDevice();
        aci.instance         = window->getInstance();
        aci.vulkanApiVersion = 0;
        auto result = vmaCreateAllocator(&aci, &allocator);
        if( result != VK_SUCCESS)
        {
            throw std::runtime_error("Error creator allocator");
        }
    }


    g_layoutCache.init(window->getDevice());

    auto mesh = [&]()
    {
        auto M = gul::Box(1,1,1);
        return CreateOpenGLMesh(M, allocator);
    }();

    auto imposterMesh = [&]()
    {
        auto M = gul::Imposter();
        return CreateOpenGLMesh(M, allocator);
    }();
    // POI:
    FrameGraphExecutor_Vulkan FGE;
    FGE.init(allocator, window->getDevice());

    //auto G = getFrameGraphGeometryOnly();
    //auto G = getFrameGraphTwoPassBlur();
    auto G = getFrameGraphSimpleDeferred();

    Pipeline geometryPipeline;
    Pipeline presentPipeline;

    gul::Transform objT;

    FGE.setRenderer("geometryPass", [&](FrameGraphExecutor_Vulkan::Frame & F) mutable
    {
        spdlog::info("geometryPass");

        // during the first run
        if(geometryPipeline.pipeline == VK_NULL_HANDLE)
        {
            geometryPipeline = createPipeline(window->getDevice(),
                                              vertex_shader,
                                              fragment_shader,
                                              F.renderPass,
                                              F.inputAttachmentSetLayout // this will be VK_NULL_HANDLE is render pass node doesn't take any input samplers
                                              );
        }

        F.beginRenderPass();
#if 1
            geometryPipeline.bind(F.commandBuffer);

            gul::Transform cameraT;

            cameraT.position = {0,0,5};
            cameraT.lookat({0,0,0},{0,1,0});
            auto cameraProjectionMatrix = glm::perspective( glm::radians(45.f), static_cast<float>(F.renderableWidth)/static_cast<float>(F.renderableHeight), 0.1f, 100.f);
            auto cameraViewMatrix = cameraT.getViewMatrix();

            VkViewport vp = {0, 0,static_cast<float>(F.imageWidth), static_cast<float>(F.imageHeight), 0.0f,1.0f};
            VkRect2D sc = { {0, 0}, {F.imageWidth, F.imageHeight}};
            vkCmdSetViewport(F.commandBuffer, 0, 1, &vp);
            vkCmdSetScissor(F.commandBuffer,0,1,&sc);

            // For each object
            {
                objT.rotateGlobal({0,1,1}, 0.01f);

                auto matrix = cameraProjectionMatrix * cameraViewMatrix * objT.getMatrix();

                mesh.bind(F.commandBuffer, 0, 0);
                geometryPipeline.pushConstants(F.commandBuffer, sizeof(glm::mat4), &matrix);
                mesh.draw(F.commandBuffer);
            }
#endif
        F.endRenderPass();

        //VkImageMemoryBarrier image_barrier = {
        //    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        //    .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        //    .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        //    .oldLayout = from_layout,
        //    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        //    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        //    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        //    .image = essentials->images[image_index],
        //    .subresourceRange = {
        //        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        //        .baseMipLevel = 0,
        //        .levelCount = 1,
        //        .baseArrayLayer = 0,
        //        .layerCount = 1,
        //    },
        //};
         vkCmdPipelineBarrier(F.commandBuffer,
                 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                 0,			/* no flags */
                 0, NULL,		/* no memory barriers */
                 0, NULL,		/* no buffer barriers */
                 0, NULL);	/* our image transition */
#if 0
        //=============================================================
        // Bind the frame buffer for this pass and make sure that
        // each input attachment is bound to some texture unit
        //=============================================================
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, F.frameBuffer);
        for(uint32_t i=0;i<F.inputAttachments.size();i++)
        {
            gl::glActiveTexture(gl::GL_TEXTURE0 + i); // activate the texture unit first before binding texture
            gl::glBindTexture(gl::GL_TEXTURE_2D, F.inputAttachments[i]);
        }
        //=============================================================
        gl::glUseProgram( modelShader );
        gl::glEnable( gl::GL_DEPTH_TEST );
        gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gl::glViewport( 0, 0, F.width, F.height );
        gl::glClear( gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

        gul::Transform cameraT;

        cameraT.position = {0,0,5};
        cameraT.lookat({0,0,0},{0,1,0});
        auto cameraProjectionMatrix = glm::perspective( glm::radians(45.f), static_cast<float>(width)/static_cast<float>(height), 0.1f, 100.f);
        auto cameraViewMatrix = cameraT.getViewMatrix();

        // For each object
        {
            objT.rotateGlobal({0,1,1}, 0.01f);

            auto matrix = cameraProjectionMatrix * cameraViewMatrix * objT.getMatrix();
            gl::glUniformMatrix4fv( gl::glGetUniformLocation( modelShader, "u_projection_matrix" ), 1, gl::GL_FALSE, &matrix[0][0] );
            boxMeshMesh.draw();
        }
#endif
    });
    FGE.setRenderer("HBlur1", [&](FrameGraphExecutor_Vulkan::Frame & F)
    {
        #if 0
        //=============================================================
        // Bind the frame buffer for this pass and make sure that
        // each input attachment is bound to some texture unit
        //=============================================================
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, F.frameBuffer);

        for(uint32_t i=0;i<F.inputAttachments.size();i++)
        {
            gl::glActiveTexture( gl::GL_TEXTURE0+i ); // activate the texture unit first before binding texture
            gl::glBindTexture(gl::GL_TEXTURE_2D, F.inputAttachments[i]);
        }
        //=============================================================
        gl::glUseProgram( blurShader );

        gl::glUniform1i(gl::glGetUniformLocation(blurShader, "in_Attachment_0"), 0);
        gl::glDisable( gl::GL_DEPTH_TEST );
        gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gl::glViewport( 0, 0, F.width, F.height );  // not managed by the frame graph. need window width/height
        gl::glClear( gl::GL_COLOR_BUFFER_BIT);


        auto M = glm::scale(glm::identity<glm::mat4>(), {1.f,1.f,1.0f});
        gl::glUniformMatrix4fv( gl::glGetUniformLocation( blurShader, "u_projection_matrix" ), 1, gl::GL_FALSE, &M[0][0] );
        auto filterDirectionLocation = gl::glGetUniformLocation( blurShader, "filterDirection" );

        glm::vec2 dir = glm::vec2(1.f, 0.0f) / glm::vec2(F.width, F.height);
        gl::glUniform2f( filterDirectionLocation, dir[0], dir[1]);
        imposterMesh.draw();
#endif
    });
    FGE.setRenderer("VBlur1", [&](FrameGraphExecutor_Vulkan::Frame & F)
    {
        #if 0
        //=============================================================
        // Bind the frame buffer for this pass and make sure that
        // each input attachment is bound to some texture unit
        //=============================================================
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, F.frameBuffer);

        for(uint32_t i=0;i<F.inputAttachments.size();i++)
        {
            gl::glActiveTexture( gl::GL_TEXTURE0+i ); // activate the texture unit first before binding texture
            gl::glBindTexture(gl::GL_TEXTURE_2D, F.inputAttachments[i]);
        }
        //=============================================================
        gl::glUseProgram( blurShader );

        gl::glUniform1i(gl::glGetUniformLocation(blurShader, "in_Attachment_0"), 0);
        gl::glDisable( gl::GL_DEPTH_TEST );
        gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gl::glViewport( 0, 0, F.width, F.height );  // not managed by the frame graph. need window width/height
        gl::glClear( gl::GL_COLOR_BUFFER_BIT);


        auto M = glm::scale(glm::identity<glm::mat4>(), {1.f,1.f,1.0f});
        gl::glUniformMatrix4fv( gl::glGetUniformLocation( blurShader, "u_projection_matrix" ), 1, gl::GL_FALSE, &M[0][0] );
        auto filterDirectionLocation = gl::glGetUniformLocation( blurShader, "filterDirection" );
        glm::vec2 dir = glm::vec2(0.f, 1.0f) / glm::vec2(F.width, F.height);
        gl::glUniform2f( filterDirectionLocation, dir[0], dir[1]);
        imposterMesh.draw();
#endif
    });
    FGE.setRenderer("Final", [&](FrameGraphExecutor_Vulkan::Frame & F)
    {
        F.clearValue[0].color.float32[0] = 1.0f;
        F.clearValue[0].color.float32[3] = 1.0f;

        if(presentPipeline.pipeline == VK_NULL_HANDLE)
        {
            presentPipeline = createPipeline(window->getDevice(),
                                             vertex_shader, fragment_shader_presentImage,
                                             F.renderPass, F.inputAttachmentSetLayout);
        }
        F.beginRenderPass();
            presentPipeline.bind(F.commandBuffer);

            VkViewport vp = {0, 0,static_cast<float>(F.imageWidth), static_cast<float>(F.imageHeight), 0.0f,1.0f};
            VkRect2D sc = { {0, 0}, {F.imageWidth, F.imageHeight}};
            vkCmdSetViewport(F.commandBuffer, 0, 1, &vp);
            vkCmdSetScissor(F.commandBuffer,0,1,&sc);

            imposterMesh.bind(F.commandBuffer,0,0);
            vkCmdBindDescriptorSets(F.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentPipeline.layout, 0, 1, &F.inputAttachmentSet,0,nullptr);
            glm::mat4 M(1.0f);
            presentPipeline.pushConstants(F.commandBuffer, sizeof(M), &M);
            imposterMesh.draw(F.commandBuffer);
        F.endRenderPass();
#if 0
        //=============================================================
        // Bind the frame buffer for this pass and make sure that
        // each input attachment is bound to some texture unit
        //=============================================================
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, F.frameBuffer);

        for(uint32_t i=0;i<F.inputAttachments.size();i++)
        {
            gl::glActiveTexture( gl::GL_TEXTURE0+i ); // activate the texture unit first before binding texture
            gl::glBindTexture(gl::GL_TEXTURE_2D, F.inputAttachments[i]);
        }
        //=============================================================
        gl::glUseProgram( imposterShader );

        gl::glUniform1i(gl::glGetUniformLocation(imposterShader, "in_Attachment_0"), 0);
        gl::glUniform1i(gl::glGetUniformLocation(imposterShader, "in_Attachment_1"), 1);
        gl::glDisable( gl::GL_DEPTH_TEST );
        gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gl::glViewport( 0, 0, F.width, F.height );  // not managed by the frame graph. need window width/height
        gl::glClear( gl::GL_COLOR_BUFFER_BIT);



        auto M = glm::scale(glm::identity<glm::mat4>(), {1.0f,1.0f,1.0f});
        gl::glUniformMatrix4fv( gl::glGetUniformLocation( imposterShader, "u_projection_matrix" ), 1, gl::GL_FALSE, &M[0][0] );
        imposterMesh.draw();
#endif
    });

    uint32_t _width  = 1024;
    uint32_t _height = 768;
    FGE.resize(G, _width,_height);

    bool running=true;
    while(running)
    {
        SDL_Event event;
        bool resize=false;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
                    /*&& event.window.windowID == SDL_GetWindowID( window->getSDLWindow()) */ )
            {
                resize=true;
                _width = event.window.data1;
                _height= event.window.data2;
                spdlog::info("Resized to: {}x{}", _width, _height);
            }
        }
        if( resize )
        {
            // If the window has changed size. we need to rebuild the swapchain
            // and any other textures (depth texture)
            window->rebuildSwapchain();
            FGE.resize(G, _width, _height);
        }

        // Get the next available frame.
        // the Frame struct is simply a POD containing
        // all the information that you need to record a command buffer
        auto frame = window->acquireNextFrame();


        FrameGraphExecutor_Vulkan::RenderInfo Ri;
        Ri.commandBuffer        = frame.commandBuffer;
        Ri.swapchainFrameBuffer = frame.framebuffer;
        Ri.swapchainRenderPass  = frame.renderPass;
        Ri.swapchainWidth       = frame.swapchainSize.width;
        Ri.swapchainHeight      = frame.swapchainSize.height;
        Ri.swapchainImage       = frame.swapchainImageView;
        Ri.swapchainDepthImage  = frame.depthImageView;

        frame.beginCommandBuffer();
            //frame.clearColor = {{1.f,0.f,0.f,0.f}};
            //frame.beginRenderPass( frame.commandBuffer );

            // record to frame.commandbuffer
            FGE(G, Ri);

            //frame.endRenderPass(frame.commandBuffer);
        frame.endCommandBuffer();

        window->submitFrame(frame);

        // Present the frame after you have recorded
        // the command buffer;
        window->presentFrame(frame);
        window->waitForPresent();
    }

    FGE.releaseGraphResources(G);

    mesh.destroy(allocator);

    vmaDestroyAllocator(allocator);

    geometryPipeline.destroy(window->getDevice());
    presentPipeline.destroy(window->getDevice());

    g_layoutCache.destroy();

    // delete the window to destroy all objects
    // that were created.
    window->destroy();
    delete window;

    glslang::FinalizeProcess();

    SDL_Quit();
    return 0;
}

#if 0
int main( int argc, char * argv[] )
{
    // Assume context creation using GLFW
    glbinding::initialize([](const char * name)
    {
        return reinterpret_cast<glbinding::ProcAddress>(SDL_GL_GetProcAddress(name));
    }, false);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

    static const int width  = 800;
    static const int height = 600;

    SDL_Window * window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
    SDL_GLContext context = SDL_GL_CreateContext( window );
    (void)context;

    gl::glDebugMessageCallback( MessageCallback, 0 );

    auto modelShader    = compileShader(vertex_shader, fragment_shader);
    auto imposterShader = compileShader(vertex_shader, fragment_shader_presentImage);
    auto blurShader     = compileShader(vertex_shader, fragment_shader_blur);

    auto Bmesh = gul::Box(1.0f);
    auto Imesh = gul::Imposter(1.0f);

    auto boxMeshMesh  = CreateOpenGLMesh(Bmesh);
    auto imposterMesh = CreateOpenGLMesh(Imesh);

    //auto G = getFrameGraphGeometryOnly();
    //auto G = getFrameGraphSimpleDeferred();
    auto G = getFrameGraphTwoPassBlur();

    FrameGraphExecutor_OpenGL framegraphExecutor;
    gul::Transform objT;

    framegraphExecutor.setRenderer("geometryPass", [&](FrameGraphExecutor_OpenGL::Frame & F)
    {
        //=============================================================
        // Bind the frame buffer for this pass and make sure that
        // each input attachment is bound to some texture unit
        //=============================================================
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, F.frameBuffer);
        for(uint32_t i=0;i<F.inputAttachments.size();i++)
        {
            gl::glActiveTexture(gl::GL_TEXTURE0 + i); // activate the texture unit first before binding texture
            gl::glBindTexture(gl::GL_TEXTURE_2D, F.inputAttachments[i]);
        }
        //=============================================================
        gl::glUseProgram( modelShader );
        gl::glEnable( gl::GL_DEPTH_TEST );
        gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gl::glViewport( 0, 0, F.width, F.height );
        gl::glClear( gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

        gul::Transform cameraT;

        cameraT.position = {0,0,5};
        cameraT.lookat({0,0,0},{0,1,0});
        auto cameraProjectionMatrix = glm::perspective( glm::radians(45.f), static_cast<float>(width)/static_cast<float>(height), 0.1f, 100.f);
        auto cameraViewMatrix = cameraT.getViewMatrix();

        // For each object
        {
            objT.rotateGlobal({0,1,1}, 0.01f);

            auto matrix = cameraProjectionMatrix * cameraViewMatrix * objT.getMatrix();
            gl::glUniformMatrix4fv( gl::glGetUniformLocation( modelShader, "u_projection_matrix" ), 1, gl::GL_FALSE, &matrix[0][0] );
            boxMeshMesh.draw();
        }
    });
    framegraphExecutor.setRenderer("HBlur1", [&](FrameGraphExecutor_OpenGL::Frame & F)
    {
        //=============================================================
        // Bind the frame buffer for this pass and make sure that
        // each input attachment is bound to some texture unit
        //=============================================================
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, F.frameBuffer);

        for(uint32_t i=0;i<F.inputAttachments.size();i++)
        {
            gl::glActiveTexture( gl::GL_TEXTURE0+i ); // activate the texture unit first before binding texture
            gl::glBindTexture(gl::GL_TEXTURE_2D, F.inputAttachments[i]);
        }
        //=============================================================
        gl::glUseProgram( blurShader );

        gl::glUniform1i(gl::glGetUniformLocation(blurShader, "in_Attachment_0"), 0);
        gl::glDisable( gl::GL_DEPTH_TEST );
        gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gl::glViewport( 0, 0, F.width, F.height );  // not managed by the frame graph. need window width/height
        gl::glClear( gl::GL_COLOR_BUFFER_BIT);


        auto M = glm::scale(glm::identity<glm::mat4>(), {1.f,1.f,1.0f});
        gl::glUniformMatrix4fv( gl::glGetUniformLocation( blurShader, "u_projection_matrix" ), 1, gl::GL_FALSE, &M[0][0] );
        auto filterDirectionLocation = gl::glGetUniformLocation( blurShader, "filterDirection" );

        glm::vec2 dir = glm::vec2(1.f, 0.0f) / glm::vec2(F.width, F.height);
        gl::glUniform2f( filterDirectionLocation, dir[0], dir[1]);
        imposterMesh.draw();
    });
    framegraphExecutor.setRenderer("VBlur1", [&](FrameGraphExecutor_OpenGL::Frame & F)
    {
        //=============================================================
        // Bind the frame buffer for this pass and make sure that
        // each input attachment is bound to some texture unit
        //=============================================================
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, F.frameBuffer);

        for(uint32_t i=0;i<F.inputAttachments.size();i++)
        {
            gl::glActiveTexture( gl::GL_TEXTURE0+i ); // activate the texture unit first before binding texture
            gl::glBindTexture(gl::GL_TEXTURE_2D, F.inputAttachments[i]);
        }
        //=============================================================
        gl::glUseProgram( blurShader );

        gl::glUniform1i(gl::glGetUniformLocation(blurShader, "in_Attachment_0"), 0);
        gl::glDisable( gl::GL_DEPTH_TEST );
        gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gl::glViewport( 0, 0, F.width, F.height );  // not managed by the frame graph. need window width/height
        gl::glClear( gl::GL_COLOR_BUFFER_BIT);


        auto M = glm::scale(glm::identity<glm::mat4>(), {1.f,1.f,1.0f});
        gl::glUniformMatrix4fv( gl::glGetUniformLocation( blurShader, "u_projection_matrix" ), 1, gl::GL_FALSE, &M[0][0] );
        auto filterDirectionLocation = gl::glGetUniformLocation( blurShader, "filterDirection" );
        glm::vec2 dir = glm::vec2(0.f, 1.0f) / glm::vec2(F.width, F.height);
        gl::glUniform2f( filterDirectionLocation, dir[0], dir[1]);
        imposterMesh.draw();
    });
    framegraphExecutor.setRenderer("Final", [&](FrameGraphExecutor_OpenGL::Frame & F)
    {
        //=============================================================
        // Bind the frame buffer for this pass and make sure that
        // each input attachment is bound to some texture unit
        //=============================================================
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, F.frameBuffer);

        for(uint32_t i=0;i<F.inputAttachments.size();i++)
        {
            gl::glActiveTexture( gl::GL_TEXTURE0+i ); // activate the texture unit first before binding texture
            gl::glBindTexture(gl::GL_TEXTURE_2D, F.inputAttachments[i]);
        }
        //=============================================================
        gl::glUseProgram( imposterShader );

        gl::glUniform1i(gl::glGetUniformLocation(imposterShader, "in_Attachment_0"), 0);
        gl::glUniform1i(gl::glGetUniformLocation(imposterShader, "in_Attachment_1"), 1);
        gl::glDisable( gl::GL_DEPTH_TEST );
        gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
        gl::glViewport( 0, 0, F.width, F.height );  // not managed by the frame graph. need window width/height
        gl::glClear( gl::GL_COLOR_BUFFER_BIT);



        auto M = glm::scale(glm::identity<glm::mat4>(), {1.0f,1.0f,1.0f});
        gl::glUniformMatrix4fv( gl::glGetUniformLocation( imposterShader, "u_projection_matrix" ), 1, gl::GL_FALSE, &M[0][0] );
        imposterMesh.draw();
    });


    framegraphExecutor.initGraphResources(G);
    framegraphExecutor.resize(G, width,height);

    bool quit=false;
    while( !quit )
    {
        SDL_Event event;
        while( SDL_PollEvent( &event ) )
        {
            switch( event.type )
            {
                case SDL_KEYUP:
                    if( event.key.keysym.sym == SDLK_ESCAPE )
                        return 0;
                    break;
                case SDL_WINDOWEVENT:
                    if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        // POI: Resize the executor, at this stage
                        // any previous images might be destroyed
                        framegraphExecutor.resize(G, event.window.data1,event.window.data2);
                    }
                    break;
                case SDL_QUIT:
                    quit = true;
            }
        }

        framegraphExecutor(G);

        SDL_GL_SwapWindow( window );
        SDL_Delay( 1 );
    }

    framegraphExecutor.releaseGraphResources(G);
    SDL_GL_DeleteContext( context );
    SDL_DestroyWindow( window );
    SDL_Quit();

    return 0;
}

#endif

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <vkw/VKWVulkanWindow.inl>
