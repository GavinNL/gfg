# GFG - Gavin's Frame Graph

This library allows you to define a Render Frame Graph, and then use a Executor (either openGL or Vulkan) to run the frame graph.

The Executor will manage generating all the internal data such as images/framebuffers/renderPasses/etc

No dependencies outside of OpenGL/Vulkan. Only the examples use external libraries.

## External Libraries for Examples

The examples use some of my helper libraries. They also use other third party libraries which are handled
by the Conan Package Manager

 * [GVU](https://gitlab.com/gavinNL/gvu) - to help manage creating vulkan primitives
 * [GUL](https://gitlab.com/gavinNL/gul) - For a Mesh class 
 * [VKW](https://gitlab.com/gavinNL/vkw) - To set up a vulkan window
 * GLM - OpenGL/Vulkan Math  
 
# Build the Example

```bash
cd source_folder
mkdir build && cd build

# install any dependencies for the examples
conan install .. -s compiler.libcxx=libstdc++11

cmake .. -DCMAKE_MODULE_PATH=$PWD

cmake --build .
```


# Example: Two-Pass Gaussian Blur

There are two examples provided in the bin folder. These examples demonstrate
setting up a 2-pass gaussian blur frame graph

 * [OpenGL Example](bin/frameGraphOpenGL/main.cpp)
 * [Vulkan Example](bin/frameGraphVulkan/main.cpp)

 
Below are some of the main ideas to demonstrate how the library works

We first need to define how the graph looks

```cpp
gfg::FrameGraph G;

// This render pass will be used to render geometry to the 
// colour (C1) and depth buffer (D1)
G.createRenderPass("geometryPass")
    .output("C1", FrameGraphFormat::R8G8B8A8_UNORM)
    .output("D1", FrameGraphFormat::D32_SFLOAT);

// we will then run a horizontal blur pass. We will
// sample from C1 and write to B1h buffer
G.createRenderPass("HBlur1")
    .setExtent(256,256)
    .input("C1")
    .output("B1h", FrameGraphFormat::R8G8B8A8_UNORM);

// then run a vertical blur pass by sampling from B1h and
// writing to B1v
G.createRenderPass("VBlur1")
    .setExtent(256,256)
    .input("B1h")
    .output("B1v",FrameGraphFormat::R8G8B8A8_UNORM);

// finally we will read in the original colour C1
// and the final blurr B1v
// and write to the swapchain/window
G.createRenderPass("Final")
    .input("B1v")
    .input("C1")
    ;

G.finalize();
```

We can then use one of the executors to run the frame graph


## OpenGL Executor

To use the OpenGL executor, you can instantiate an object and set the render functions for
each pass.

```cpp
gfg::FrameGraphExecutor_OpenGL framegraphExecutor;

framegraphExecutor.setRenderer("geometryPass", [&](FrameGraphExecutor_OpenGL::Frame & F)
{
    F.bindFramebuffer();
    F.bindInputTextures(0); // 

    gl::glUseProgram( modelShader );
    gl::glEnable( gl::GL_DEPTH_TEST );
    gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
    gl::glViewport( 0, 0, F.renderableWidth, F.renderableHeight);
    gl::glClear( gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

    //
        // for each mesh:
        //    for each material:
        //       draw() 
    //
});

framegraphExecutor.setRenderer("HBlur1", [&](FrameGraphExecutor_OpenGL::Frame & F)
{
    F.bindFramebuffer();
    F.bindInputTextures(0);

    gl::glUseProgram( blurShader );

    // set the in_Attachment_0 texture in the shader to read from
    // GL_TEXTURE0
    gl::glUniform1i(gl::glGetUniformLocation(blurShader, "in_Attachment_0"), 0);
    gl::glDisable( gl::GL_DEPTH_TEST );
    gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
    gl::glViewport( 0, 0, F.renderableWidth, F.renderableHeight);
    gl::glClear( gl::GL_COLOR_BUFFER_BIT);

    // set which direction we want to perform the blur in
    auto filterDirectionLocation = gl::glGetUniformLocation( blurShader, "filterDirection" );
    glm::vec2 dir = glm::vec2(1.f, 0.0f) / glm::vec2(F.renderableWidth, F.renderableHeight);
    gl::glUniform2f( filterDirectionLocation, dir[0], dir[1]);

    // draw full screen quad
});


framegraphExecutor.setRenderer("VBlur1", [&](FrameGraphExecutor_OpenGL::Frame & F)
{
    F.bindFramebuffer();
    F.bindInputTextures(0);

    gl::glUseProgram( blurShader );

    // set the in_Attachment_0 texture in the shader to read from
    // GL_TEXTURE0
    gl::glUniform1i(gl::glGetUniformLocation(blurShader, "in_Attachment_0"), 0);
    gl::glDisable( gl::GL_DEPTH_TEST );
    gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
    gl::glViewport( 0, 0, F.renderableWidth, F.renderableHeight);
    gl::glClear( gl::GL_COLOR_BUFFER_BIT);

    // set which direction we want to perform the blur in
    auto filterDirectionLocation = gl::glGetUniformLocation( blurShader, "filterDirection" );
    glm::vec2 dir = glm::vec2(0.f, 1.0f) / glm::vec2(F.renderableWidth, F.renderableHeight);
    gl::glUniform2f( filterDirectionLocation, dir[0], dir[1]);

    // draw full screen quad
});

framegraphExecutor.setRenderer("Final", [&](FrameGraphExecutor_OpenGL::Frame & F)
{
    //=============================================================
    // Bind the frame buffer for this pass and make sure that
    // each input attachment is bound to some texture unit
    //=============================================================
    F.bindFramebuffer();
    F.bindInputTextures(0);
    //=============================================================

    // use a simple full screen quad shader to render a texture
    gl::glUseProgram( imposterShader );
    gl::glUniform1i(gl::glGetUniformLocation(imposterShader, "in_Attachment_0"), 0);  
    gl::glUniform1i(gl::glGetUniformLocation(imposterShader, "in_Attachment_1"), 1);  

    gl::glDisable( gl::GL_DEPTH_TEST );
    gl::glClearColor( 0.0, 0.0, 0.0, 0.0 );
    gl::glViewport( 0, 0, F.renderableWidth, F.renderableHeight);  
    gl::glClear( gl::GL_COLOR_BUFFER_BIT);

    // draw full screen quad
});
```

Now in your main loop, you simply have to call the following function:


```cpp

framegraphExecutor.init();
framegraphExecutor.resize(G, windowWidth, windowHeight);
while(mainLoop)
{
    if(window_has_resized)
    {
        framegraphExecutor.resize(G, newWidth, newHeight);
    }

    framegraphExecutor(G);
}

```


# Vulkan Executor

The vulkan executor is a little more complex than openGL. 
The Vulkan Frame provides you with some vulkan primitives to help you get started. 
It provides you with a RenderPass and a DescriptorSetLayout for the input sampled images,
which can be used to generate your pipelines

The input sampled images are handled by a single descriptor set with contains an array of 10 images. 
Your fragment shader should looks something like the following:

```glsl
layout (set = 0, binding = 0) uniform sampler2D u_Attachment[10];
```



```cpp
VkPipeline geometryPipeline = VK_NULL_HANDLE;
VkPipeline filterPipeline   = VK_NULL_HANDLE;

FGE.setRenderer("geometryPass", [&](FrameGraphExecutor_Vulkan::Frame & F) mutable
{
    // during the first run
    if(geometryPipeline == VK_NULL_HANDLE)
    {
        // generate geometryPipeline using F.renderPass and F.inputAttachmentSetLayout
        // you'd probably want to use dynamic viewport/scissors for your pipelines
    }

    F.beginRenderPass();

        vkCmdBindPipeline(F.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometryPipeline);
        VkViewport vp = {0, 0,static_cast<float>(F.imageWidth), static_cast<float>(F.imageHeight), 0.0f,1.0f};
        VkRect2D sc = { {0, 0}, {F.imageWidth, F.imageHeight}};
        vkCmdSetViewport(F.commandBuffer, 0, 1, &vp);
        vkCmdSetScissor(F.commandBuffer,0,1,&sc);

        // draw objects
    F.endRenderPass();

    // do a full barrier syncronization. probably not the best option
    F.fullBarrier();
});

FGE.setRenderer("HBlur1", [&](FrameGraphExecutor_Vulkan::Frame & F)
{
    F.clearValue[0].color.float32[0] = 1.0f;
    F.clearValue[0].color.float32[3] = 1.0f;

    if(filterPipeline == VK_NULL_HANDLE)
    {
        // generate filterPipeline using F.renderPass and F.inputAttachmentSetLayout
    }

    F.beginRenderPass();

        vkCmdBindPipeline(F.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geometryPipeline);
        VkViewport vp = {0, 0,static_cast<float>(F.imageWidth), static_cast<float>(F.imageHeight), 0.0f,1.0f};
        VkRect2D sc = { {0, 0}, {F.imageWidth, F.imageHeight}};
        vkCmdSetViewport(F.commandBuffer, 0, 1, &vp);
        vkCmdSetScissor(F.commandBuffer,0,1,&sc);

        // bind the descriptor set which holds all the input sampled images
        vkCmdBindDescriptorSets(F.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, filterPipelineLayout, 0, 1, &F.inputAttachmentSet,0,nullptr);

        // draw objects
    F.endRenderPass();

    // do a full barrier syncronization. probably not the best option
    F.fullBarrier();
});

FGE.setRenderer("VBlur1", [&](FrameGraphExecutor_Vulkan::Frame & F)
{
    // same as HBlur with minor changes
});

FGE.setRenderer("Final", [&](FrameGraphExecutor_Vulkan::Frame & F)
{
    F.clearValue[0].color.float32[0] = 1.0f;
    F.clearValue[0].color.float32[3] = 1.0f;

    if(presentPipeline == VK_NULL_HANDLE)
    {
        // generate presentPipeline using F.renderPass and F.inputAttachmentSetLayout
    }

    F.beginRenderPass();
        vkCmdBindPipeline(F.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentPipeline);

        VkViewport vp = {0, 0,static_cast<float>(F.imageWidth), static_cast<float>(F.imageHeight), 0.0f,1.0f};
        VkRect2D sc = { {0, 0}, {F.imageWidth, F.imageHeight}};
        vkCmdSetViewport(F.commandBuffer, 0, 1, &vp);
        vkCmdSetScissor(F.commandBuffer , 0, 1, &sc);

        vkCmdBindDescriptorSets(F.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentPipelineLayout, 0, 1, &F.inputAttachmentSet,0,nullptr);

        // draw fullscreen quad
    F.endRenderPass();

});
```

Executing the vulkan framegraph is a bit more complicated than in OpenGL because you have to deal with the swapchains and
it's images/framebuffers.

It was decided that the swapchain should be handled externally from the framegraph. Your window system or rendering framework
should generate the following to be used 

 * swapchain
 * swapchain imageViews
 * swapchain Framebuffers
 * swapchain renderpass;

```cpp

framegraphExecutor.init();
framegraphExecutor.resize(G, windowWidth, windowHeight);

while(mainLoop)
{
    if(window_has_resized)
    {
        framegraphExecutor.resize(G, newWidth, newHeight);
    }

    FrameGraphExecutor_Vulkan::RenderInfo Ri;
    Ri.commandBuffer        = commandBuffer;          // the command buffer to use for this frame. it will be passed to all the renderers
    Ri.swapchainFrameBuffer = swapchainFramebuffer;   // the framebuffer of the swapchain
    Ri.swapchainRenderPass  = swapchainRenderPass;    // the renderpass compatiable with the swapchain
    Ri.swapchainWidth       = swapchainSize.width;    // size of the swapchain
    Ri.swapchainHeight      = swapchainSize.height; 
    Ri.swapchainImage       = swapchainImageView;     // which image are we rendering to for this frame
    Ri.swapchainDepthImage  = swapchaindepthImageView; // which depth image we are using for rendering to the swapchain

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

        framegraphExecutor(G, Ri);

    vkEndCommandBuffer(commandBuffer);

    // submit your frame
}

```