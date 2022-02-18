#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "frameGraph/frameGraph.h"
#include "frameGraph/openGLGraph.h"

FrameGraph getFrameGraph()
{
    FrameGraph G;

#if 1
    G.createRenderPass("ShadowPass")
     .output("SM", {});

    G.createRenderPass("geometryPass")
     .input("SM")
     .output("C1", FrameGraphFormat::R8G8B8A8_UNORM);

    G.createRenderPass("HBlur1")
     .input("C1")
     .output("B1h", FrameGraphFormat::R8G8B8A8_UNORM);

    G.createRenderPass("VBlur1")
     .input("B1h")
     .output("B1v",FrameGraphFormat::R8G8B8A8_UNORM);

    G.createRenderPass("Final")
     .input("C1")
     .input("B1v")
     .output("F", FrameGraphFormat::R8G8B8A8_UNORM);
#else
    G.createRenderPass("G", {"C1"});


    G.createRenderPass("H", {"C2"}, {"C1"});

    G.createRenderPass("V", {"C3"}, {"C2"});

    G.createRenderPass("P", {"C4","C7"}, {"C1","C3"});

    G.createRenderPass("Q", {"C5"}, {"C4"});

    G.createRenderPass("R", {"C6"}, {"C5", "C7"});
#endif
    G.allocateImages();

    //auto order = G.findExecutionOrder();
    //spdlog::info("Order: {}", fmt::join(BE(order),","));
    //for(auto & n : G.nodes)
    //{
    //    if( std::holds_alternative<RenderTargetNode>(n.second) )
    //    {
    //        spdlog::info("{} is using image: {}", n.first, std::get<RenderTargetNode>(n.second).imageResource.name);
    //    }
    //}



    //OpenGLGraph vg;
    //vg(G);
    G.execute();
    return G;
    //return 0;
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
    "#version 130\n"
    "in vec2 i_position;\n"
    "in vec4 i_color;\n"
    "out vec4 v_color;\n"
    "uniform mat4 u_projection_matrix;\n"
    "void main() {\n"
    "    v_color = i_color;\n"
    "    gl_Position = u_projection_matrix * vec4( i_position, 0.0, 1.0 );\n"
    "}\n";

static const char * fragment_shader =
    "#version 130\n"
    "in vec4 v_color;\n"
    "out vec4 o_color;\n"
    "void main() {\n"
    "    o_color = v_color;\n"
    "}\n";

typedef enum t_attrib_id
{
    attrib_position,
    attrib_color
} t_attrib_id;

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

int main( int argc, char * argv[] )
{
    // Assume context creation using GLFW
    glbinding::initialize([](const char * name)
    {
        return reinterpret_cast<glbinding::ProcAddress>(SDL_GL_GetProcAddress(name));
    }, false);

    SDL_Init( SDL_INIT_VIDEO );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

    static const int width = 800;
    static const int height = 600;

    SDL_Window * window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
    SDL_GLContext context = SDL_GL_CreateContext( window );
    (void)context;

    gl::glDebugMessageCallback( MessageCallback, 0 );

    gl::GLuint vs, fs, program;


    vs = gl::glCreateShader( gl::GL_VERTEX_SHADER );
    fs = gl::glCreateShader( gl::GL_FRAGMENT_SHADER );

    int length = strlen( vertex_shader );
    gl::glShaderSource( vs, 1, ( const gl::GLchar ** )&vertex_shader, &length );
    gl::glCompileShader( vs );

    gl::GLboolean status;
    gl::glGetShaderiv( vs, gl::GL_COMPILE_STATUS, &status );
    if( status == gl::GL_FALSE )
    {
        fprintf( stderr, "vertex shader compilation failed\n" );
        return 1;
    }

    length = strlen( fragment_shader );
    gl::glShaderSource( fs, 1, ( const gl::GLchar ** )&fragment_shader, &length );
    gl::glCompileShader( fs );

    gl::glGetShaderiv( fs, gl::GL_COMPILE_STATUS, &status );
    if( status == gl::GL_FALSE )
    {
        fprintf( stderr, "fragment shader compilation failed\n" );
        return 1;
    }

    program = gl::glCreateProgram();
    gl::glAttachShader( program, vs );
    gl::glAttachShader( program, fs );

    gl::glBindAttribLocation( program, attrib_position, "i_position" );
    gl::glBindAttribLocation( program, attrib_color, "i_color" );
    gl::glLinkProgram( program );

    gl::glUseProgram( program );

    gl::glDisable( gl::GL_DEPTH_TEST );
    gl::glClearColor( 0.5, 0.0, 0.0, 0.0 );
    gl::glViewport( 0, 0, width, height );

    gl::GLuint vao, vbo;

    gl::glGenVertexArrays( 1, &vao );
    gl::glGenBuffers( 1, &vbo );
    gl::glBindVertexArray( vao );
    gl::glBindBuffer( gl::GL_ARRAY_BUFFER, vbo );

    gl::glEnableVertexAttribArray( attrib_position );
    gl::glEnableVertexAttribArray( attrib_color );

    gl::glVertexAttribPointer( attrib_color, 4, gl::GL_FLOAT, gl::GL_FALSE, sizeof( float ) * 6, 0 );
    gl::glVertexAttribPointer( attrib_position, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof( float ) * 6, ( void * )(4 * sizeof(float)) );

    const gl::GLfloat g_vertex_buffer_data[] = {
    /*  R, G, B, A, X, Y  */
        1, 0, 0, 1, 0, 0,
        0, 1, 0, 1, width, 0,
        0, 0, 1, 1, width, height,

        1, 0, 0, 1, 0, 0,
        0, 0, 1, 1, width, height,
        1, 1, 1, 1, 0, height
    };

    gl::glBufferData( gl::GL_ARRAY_BUFFER, sizeof( g_vertex_buffer_data ), g_vertex_buffer_data, gl::GL_STATIC_DRAW );

    t_mat4x4 projection_matrix;
    mat4x4_ortho( projection_matrix, 0.0f, (float)width, (float)height, 0.0f, 0.0f, 100.0f );
    gl::glUniformMatrix4fv( gl::glGetUniformLocation( program, "u_projection_matrix" ), 1, gl::GL_FALSE, projection_matrix );


    auto G = getFrameGraph();

    OpenGLGraph VG;
    VG.initGraphResources(G);

    bool quit=false;
    while( !quit )
    {
        gl::glClear( gl::GL_COLOR_BUFFER_BIT );

        SDL_Event event;
        while( SDL_PollEvent( &event ) )
        {
            switch( event.type )
            {
                case SDL_KEYUP:
                    if( event.key.keysym.sym == SDLK_ESCAPE )
                        return 0;
                    break;
                case SDL_QUIT:
                    quit = true;
            }
        }

        gl::glBindVertexArray( vao );
        gl::glDrawArrays( gl::GL_TRIANGLES, 0, 6 );

        SDL_GL_SwapWindow( window );
        SDL_Delay( 1 );
    }

    VG.releaseGraphResources(G);
    SDL_GL_DeleteContext( context );
    SDL_DestroyWindow( window );
    SDL_Quit();

    return 0;
}
