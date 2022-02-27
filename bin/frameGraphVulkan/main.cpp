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
R"foo(#version 430
layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_TexCoord_0;

out vec4 v_color;
out vec2 v_TexCoord_0;

uniform mat4 u_projection_matrix;

void main() {
    v_color      = vec4(i_TexCoord_0,0,1);
    v_TexCoord_0 = i_TexCoord_0;
    gl_Position  = u_projection_matrix * vec4( i_position, 1.0 );
};
)foo";



static const char * fragment_shader =
R"foo(#version 430
    in vec4 v_color;
    in vec2 v_TexCoord_0;

    out vec4 o_color;

    void main() {
        o_color = v_color;
    }
)foo";


static const char * fragment_shader_presentImage =
R"foo(#version 430
    in vec4 v_color;
    in vec2 v_TexCoord_0;

    uniform sampler2D in_Attachment_0;
    uniform sampler2D in_Attachment_1;

    out vec4 o_color;

    void main() {
        vec4 c0 = texture(in_Attachment_0, v_TexCoord_0);
        vec4 c1 = texture(in_Attachment_1, v_TexCoord_0);

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
    VkBuffer          buffer;
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

    void draw()
    {
        //gl::glBindVertexArray( vao );
        //gl::glDrawElements(gl::GL_TRIANGLES, indexCount, gl::GL_UNSIGNED_INT, nullptr );
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

    auto     totalBufferByteSize = M.calculateDeviceSize();

    outMesh.buffer = _createBuffer(allocator,
                                   totalBufferByteSize,
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                   VMA_MEMORY_USAGE_CPU_TO_GPU); // i dont feel like doing GPU transfers for
                                                                 // this example

    void * mapped = nullptr;
    vmaMapMemory(allocator, outMesh.buffer.allocation, &mapped);

    auto offsets = M.copySequential(mapped);

    vmaUnmapMemory(allocator, outMesh.buffer.allocation);

    outMesh.indexCount  = M.indexCount();
    outMesh.vertexCount = M.vertexCount();

    return outMesh;
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
    // POI:
    FrameGraphExecutor_Vulkan FGE;
    FGE.init(allocator, window->getDevice());

    auto G = getFrameGraphTwoPassBlur();

    uint32_t _width = 1024;
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


        frame.beginCommandBuffer();
            frame.clearColor = {{1.f,0.f,0.f,0.f}};
            frame.beginRenderPass( frame.commandBuffer );

            // record to frame.commandbuffer

            frame.endRenderPass(frame.commandBuffer);
        frame.endCommandBuffer();

        window->submitFrame(frame);

        // Present the frame after you have recorded
        // the command buffer;
        window->presentFrame(frame);
        window->waitForPresent();
    }

    FGE.releaseGraphResources(G);

    vmaDestroyAllocator(allocator);

    // delete the window to destroy all objects
    // that were created.
    window->destroy();
    delete window;

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
