#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "frameGraph/frameGraph.h"
#include "frameGraph/executors/OpenGLExecutor.h"

#include <gul/MeshPrimitive.h>
#include <gul/math/Transform.h>

FrameGraph getFrameGraph()
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

/*
    Minimal SDL2 + OpenGL3 example.

    Author: https://github.com/koute

    This file is in the public domain; you can do whatever you want with it.
    In case the concept of public domain doesn't exist in your jurisdiction
    you can also use this code under the terms of Creative Commons CC0 license,
    either version 1.0 or (at your option) any later version; for details see:
        http://creativecommons.org/publicdomain/zero/1.0/

    This software is distributed without any warranty whatsoever.

    Compile and run with: gcc opengl3_hello.c `sdl2-config --libs --cflags` -lGL -Wall && ./a.out
*/

#define GL_GLEXT_PROTOTYPES
#include <glbinding/glbinding.h>
#include <glbinding-aux/ContextInfo.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include <SDL.h>
#include <stdio.h>
//#include <glbinding/gl/gl.h>

typedef float t_mat4x4[16];

static inline void mat4x4_ortho( t_mat4x4 out, float left, float right, float bottom, float top, float znear, float zfar )
{
    #define T(a, b) (a * 4 + b)

    out[T(0,0)] = 2.0f / (right - left);
    out[T(0,1)] = 0.0f;
    out[T(0,2)] = 0.0f;
    out[T(0,3)] = 0.0f;

    out[T(1,1)] = 2.0f / (top - bottom);
    out[T(1,0)] = 0.0f;
    out[T(1,2)] = 0.0f;
    out[T(1,3)] = 0.0f;

    out[T(2,2)] = -2.0f / (zfar - znear);
    out[T(2,0)] = 0.0f;
    out[T(2,1)] = 0.0f;
    out[T(2,3)] = 0.0f;

    out[T(3,0)] = -(right + left) / (right - left);
    out[T(3,1)] = -(top + bottom) / (top - bottom);
    out[T(3,2)] = -(zfar + znear) / (zfar - znear);
    out[T(3,3)] = 1.0f;

    #undef T
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

    float _coefs[9] = float[](0.02853226260337099, 0.06723453549491201, 0.1240093299792275, 0.1790438646174162, 0.2023600146101466, 0.1790438646174162, 0.1240093299792275, 0.06723453549491201, 0.02853226260337099);

    void main() {
        vec2 v = filterDirection;//vec2(0.01);

        vec4 c0 = vec4(0.0f);
        for(int i=0;i<9;i++)
        {
            c0 += _coefs[i] * texture(in_Attachment_0, v_TexCoord_0 + v*float(i-4) );
        }

        c0.a = 1.0f;
        o_color = c0;
    }
)foo";


void MessageCallback( gl::GLenum source,
                      gl::GLenum type,
                      gl::GLuint id,
                      gl::GLenum severity,
                      gl::GLsizei length,
                      const gl::GLchar* message,
                      const void* userParam )
{
    if( type == gl::GL_DEBUG_TYPE_ERROR)
    {
        std::cerr << fmt::format("ERROR: Type: {:x}  ID: {:x} {}", type, id, message) << std::endl;
    }

    (void)severity;
    (void)source;
    (void)type;
  (void)source;
  (void)id;
  (void)length;
  (void)userParam;
}

gl::GLuint compileShader(char const * _vertex_shader, char const * _fragment_shader)
{
    gl::GLuint VS, FS, program;


    VS = gl::glCreateShader( gl::GL_VERTEX_SHADER );
    FS = gl::glCreateShader( gl::GL_FRAGMENT_SHADER );

    int length = strlen( _vertex_shader );
    gl::glShaderSource( VS, 1, ( const gl::GLchar ** )&_vertex_shader, &length );
    gl::glCompileShader( VS );

    gl::GLboolean status;
    gl::glGetShaderiv( VS, gl::GL_COMPILE_STATUS, &status );
    if( status == gl::GL_FALSE )
    {
        fprintf( stderr, "vertex shader compilation failed\n" );
        return 1;
    }

    length = strlen( _fragment_shader );
    gl::glShaderSource( FS, 1, ( const gl::GLchar ** )&_fragment_shader, &length );
    gl::glCompileShader( FS );

    gl::glGetShaderiv( FS, gl::GL_COMPILE_STATUS, &status );
    if( status == gl::GL_FALSE )
    {
        fprintf( stderr, "fragment shader compilation failed\n" );
        return 1;
    }

    program = gl::glCreateProgram();
    gl::glAttachShader( program, VS );
    gl::glAttachShader( program, FS );

    //gl::glBindAttribLocation( program, attrib_position, "i_position" );
    //gl::glBindAttribLocation( program, attrib_color, "i_color" );
    gl::glLinkProgram( program );

    return program;
}


struct Mesh
{
    gl::GLuint vao;
    gl::GLuint vertexBuffer;
    gl::GLuint indexBuffer;

    uint32_t vertexCount= 0 ;
    uint32_t indexCount=0;

    void draw()
    {
        gl::glBindVertexArray( vao );
        gl::glDrawElements(gl::GL_TRIANGLES, indexCount, gl::GL_UNSIGNED_INT, nullptr );
    }
};


Mesh CreateOpenGLMesh(gul::MeshPrimitive const & M)
{
    Mesh outMesh;

    gl::GLuint vao, vertexBuffer, indexBuffer;

    gl::glGenVertexArrays( 1, &vao );
    gl::glGenBuffers( 1, &vertexBuffer );
    gl::glGenBuffers( 1, &indexBuffer );

    outMesh.vertexBuffer = vertexBuffer;
    outMesh.indexBuffer = indexBuffer;
    outMesh.vao = vao;

    gl::glBindVertexArray( vao );
    gl::glBindBuffer( gl::GL_ARRAY_BUFFER, vertexBuffer );
    gl::glBindBuffer( gl::GL_ELEMENT_ARRAY_BUFFER, indexBuffer );

    uint32_t attrIndex=0;
    uint64_t offset = 0;
    uint32_t stride = M.calculateInterleavedStride();
    auto totalBufferByteSize = M.calculateInterleavedBufferSize();

    for(auto & V : { &M.POSITION,
                     &M.NORMAL,
                     &M.TANGENT,
                     &M.TEXCOORD_0,
                     &M.TEXCOORD_1,
                     &M.COLOR_0,
                     &M.JOINTS_0,
                     &M.WEIGHTS_0})
    {
        auto count = gul::VertexAttributeCount(*V);
        if(count)
        {
            gl::glEnableVertexAttribArray( attrIndex );
            void * offset_v;
            std::memcpy(&offset_v, &offset, sizeof(offset));
            gl::glVertexAttribPointer( attrIndex ,
                                       gul::VertexAttributeNumComponents(*V),
                                       gl::GL_FLOAT,
                                       gl::GL_FALSE,
                                       stride,
                                       offset_v );
            offset += gul::VertexAttributeSizeOf(*V);
            ++attrIndex;
        }
    }

    {
        //auto M = gul::Imposter(1.0f);
        std::vector<char> data(totalBufferByteSize);
        {
            auto size = M.copyVertexAttributesInterleaved(data.data());
            gl::glBufferData( gl::GL_ARRAY_BUFFER, size, data.data(), gl::GL_STATIC_DRAW );
        }
        {
            auto size = M.copyIndex(data.data());
            gl::glBufferData( gl::GL_ELEMENT_ARRAY_BUFFER, size, data.data(), gl::GL_STATIC_DRAW );
        }
        outMesh.indexCount = M.indexCount();
    }
    gl::glBindVertexArray(0);
    return outMesh;
}


int main( int argc, char * argv[] )
{
    // Assume context creation using GLFW
    glbinding::initialize([](const char * name)
    {
        return reinterpret_cast<glbinding::ProcAddress>(SDL_GL_GetProcAddress(name));
    }, false);

    SDL_Init( SDL_INIT_VIDEO );
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

    static const int width = 800;
    static const int height = 600;

    SDL_Window * window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
    SDL_GLContext context = SDL_GL_CreateContext( window );
    (void)context;

    gl::glDebugMessageCallback( MessageCallback, 0 );

    auto modelShader = compileShader(vertex_shader, fragment_shader);
    auto imposterShader = compileShader(vertex_shader, fragment_shader_presentImage);
    auto blurShader = compileShader(vertex_shader, fragment_shader_blur);


    auto Bmesh = gul::Box(1.0f);
    auto Imesh = gul::Imposter(1.0f);

    auto boxMeshMesh = CreateOpenGLMesh(Bmesh);
    auto imposterMesh = CreateOpenGLMesh(Imesh);


    //auto G = getFrameGraphGeometryOnly();
    //auto G = getFrameGraphSimpleDeferred();
    auto G = getFrameGraph();

    OpenGLGraph VG;
    gul::Transform objT;

    VG.setRenderer("geometryPass", [&](OpenGLGraph::Frame & F)
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
    VG.setRenderer("HBlur1", [&](OpenGLGraph::Frame & F)
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
    VG.setRenderer("VBlur1", [&](OpenGLGraph::Frame & F)
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
    VG.setRenderer("Final", [&](OpenGLGraph::Frame & F)
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


    VG.initGraphResources(G);
    VG.resize(G, width,height);

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
                        VG.resize(G, event.window.data1,event.window.data2);
                    }
                    break;
                case SDL_QUIT:
                    quit = true;
            }
        }

        VG(G);

        SDL_GL_SwapWindow( window );
        SDL_Delay( 1 );
    }

    VG.releaseGraphResources(G);
    SDL_GL_DeleteContext( context );
    SDL_DestroyWindow( window );
    SDL_Quit();

    return 0;
}
