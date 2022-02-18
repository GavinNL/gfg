#ifndef GNL_FRAME_GRAPH_OPENGL_H
#define GNL_FRAME_GRAPH_OPENGL_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "frameGraph.h"


//#include <glbinding/glbinding.h>
//#include <glbinding-aux/ContextInfo.h>
#include <glbinding/gl/gl.h>
//#include <glbinding/glbinding.h>

struct OpenGLGraph
{
    struct GLNodeInfo
    {
        bool isInit = false;
    };

    struct GLImageInfo
    {
        gl::GLuint textureID;
    };

    std::map<std::string, GLNodeInfo>  _nodes;
    std::map<std::string, GLImageInfo> _imageNames;

    static gl::GLenum _getInternalFormatFromDef(ImageDefinition const & def)
    {
        switch(def.format)
        {
        case FrameGraphFormat::UNDEFINED: return static_cast<gl::GLenum>(0);
            break;
        case FrameGraphFormat::R8_UNORM:    return gl::GL_R8;
            break;
        case FrameGraphFormat::R8_SNORM:    return gl::GL_R8_SNORM;
            break;
        case FrameGraphFormat::R8_UINT:    return gl::GL_R8UI;
            break;
        case FrameGraphFormat::R8_SINT:   return gl::GL_R8I;
            break;
        case FrameGraphFormat::R8G8_UNORM:   return gl::GL_RG8;
            break;
        case FrameGraphFormat::R8G8_SNORM:   return gl::GL_RG8_SNORM;
            break;
        case FrameGraphFormat::R8G8_UINT:   return gl::GL_RG8UI;
            break;
        case FrameGraphFormat::R8G8_SINT:  return gl::GL_RG8I;
            break;
        case FrameGraphFormat::R8G8B8_UNORM:  return gl::GL_RGB8;
            break;
        case FrameGraphFormat::R8G8B8_SNORM: return gl::GL_RGB8_SNORM;
            break;
        case FrameGraphFormat::R8G8B8_UINT: return gl::GL_RGB8UI;
            break;
        case FrameGraphFormat::R8G8B8_SINT: return gl::GL_RGB8I;
            break;
        case FrameGraphFormat::R8G8B8A8_UNORM: return gl::GL_RGBA8;
            break;
        case FrameGraphFormat::R8G8B8A8_SNORM: return gl::GL_RGBA8_SNORM;
            break;
        case FrameGraphFormat::R8G8B8A8_UINT: return gl::GL_RGBA8UI;
            break;
        case FrameGraphFormat::R8G8B8A8_SINT: return gl::GL_RGBA8I;
            break;
        case FrameGraphFormat::R16_UNORM: return gl::GL_R16;
            break;
        case FrameGraphFormat::R16_SNORM: return gl::GL_R16_SNORM;
            break;
        case FrameGraphFormat::R16_UINT: return gl::GL_R16UI;
            break;
        case FrameGraphFormat::R16_SINT: return gl::GL_R16I;
            break;
        case FrameGraphFormat::R16_SFLOAT: return gl::GL_R16F;
            break;
        case FrameGraphFormat::R16G16_UNORM: return gl::GL_RG16;
            break;
        case FrameGraphFormat::R16G16_SNORM: return gl::GL_RG16_SNORM;
            break;
        case FrameGraphFormat::R16G16_UINT: return gl::GL_RG16UI;
            break;
        case FrameGraphFormat::R16G16_SINT: return gl::GL_RG16I;
            break;
        case FrameGraphFormat::R16G16_SFLOAT: return gl::GL_RG16F;
            break;
        case FrameGraphFormat::R16G16B16_UNORM: return gl::GL_RGB16;
            break;
        case FrameGraphFormat::R16G16B16_SNORM: return gl::GL_RGB16_SNORM;
            break;
        case FrameGraphFormat::R16G16B16_UINT: return gl::GL_RGB16UI;
            break;
        case FrameGraphFormat::R16G16B16_SINT: return gl::GL_RGB16I;
            break;
        case FrameGraphFormat::R16G16B16_SFLOAT: return gl::GL_RGB16F;
            break;
        case FrameGraphFormat::R16G16B16A16_UNORM: return gl::GL_RGBA16;
            break;
        case FrameGraphFormat::R16G16B16A16_SNORM: return gl::GL_RGBA16_SNORM;
            break;
        case FrameGraphFormat::R16G16B16A16_UINT: return gl::GL_RGBA16UI;
            break;
        case FrameGraphFormat::R16G16B16A16_SINT: return gl::GL_RGBA16I;
            break;
        case FrameGraphFormat::R16G16B16A16_SFLOAT: return gl::GL_RGBA16F;
            break;
        case FrameGraphFormat::R32_UINT: return gl::GL_R32UI;
            break;
        case FrameGraphFormat::R32_SINT: return gl::GL_R32I;
            break;
        case FrameGraphFormat::R32_SFLOAT: return gl::GL_R32F;
            break;
        case FrameGraphFormat::R32G32_UINT: return gl::GL_RG32UI;
            break;
        case FrameGraphFormat::R32G32_SINT: return gl::GL_RG32I;
            break;
        case FrameGraphFormat::R32G32_SFLOAT: return gl::GL_RG32F;
            break;
        case FrameGraphFormat::R32G32B32_UINT: return gl::GL_RGB32UI;
            break;
        case FrameGraphFormat::R32G32B32_SINT: return gl::GL_RGB32I;
            break;
        case FrameGraphFormat::R32G32B32_SFLOAT: return gl::GL_RGB32F;
            break;
        case FrameGraphFormat::R32G32B32A32_UINT: return gl::GL_RGBA32UI;
            break;
        case FrameGraphFormat::R32G32B32A32_SINT: return gl::GL_RGBA32I;
            break;
        case FrameGraphFormat::R32G32B32A32_SFLOAT: return gl::GL_RGBA32F;
            break;
        case FrameGraphFormat::D32_SFLOAT: return gl::GL_DEPTH_COMPONENT32F;
            break;
        case FrameGraphFormat::D24_UNORM_S8_UINT: return gl::GL_DEPTH24_STENCIL8;
            break;
        case FrameGraphFormat::D32_SFLOAT_S8_UINT: return gl::GL_DEPTH32F_STENCIL8;
            break;
        case FrameGraphFormat::MAX_ENUM:
            break;
        }
        return{};
    }

    static gl::GLenum _getFormatFromDef(ImageDefinition const & def)
    {
        switch(_getInternalFormatFromDef(def))
        {
            case gl::GL_R8: return	gl::GL_RED;
            case gl::GL_R8_SNORM: return	gl::GL_RED;
            case gl::GL_R16: return	gl::GL_RED;
            case gl::GL_R16_SNORM: return	gl::GL_RED;
            case gl::GL_RG8: return	gl::GL_RG;
            case gl::GL_RG8_SNORM: return	gl::GL_RG;
            case gl::GL_RG16: return	gl::GL_RG;
            case gl::GL_RG16_SNORM: return	gl::GL_RG;
            case gl::GL_R3_G3_B2: return	gl::GL_RGB;
            case gl::GL_RGB4: return	gl::GL_RGB;
            case gl::GL_RGB5: return	gl::GL_RGB;
            case gl::GL_RGB8: return	gl::GL_RGB;
            case gl::GL_RGB8_SNORM: return	gl::GL_RGB;
            case gl::GL_RGB10: return	gl::GL_RGB;
            case gl::GL_RGB12: return	gl::GL_RGB;
            case gl::GL_RGB16_SNORM: return	gl::GL_RGB;
            case gl::GL_RGBA2: return	gl::GL_RGB;
            case gl::GL_RGBA4: return	gl::GL_RGB;
            case gl::GL_RGB5_A1: return	gl::GL_RGBA;
            case gl::GL_RGBA8: return	gl::GL_RGBA;
            case gl::GL_RGBA8_SNORM: return	gl::GL_RGBA;
            case gl::GL_RGB10_A2: return	gl::GL_RGBA;
            case gl::GL_RGB10_A2UI: return	gl::GL_RGBA;
            case gl::GL_RGBA12: return	gl::GL_RGBA;
            case gl::GL_RGBA16: return	gl::GL_RGBA;
            case gl::GL_SRGB8: return	gl::GL_RGB;
            case gl::GL_SRGB8_ALPHA8: return	gl::GL_RGBA;
            case gl::GL_R16F: return	gl::GL_RED;
            case gl::GL_RG16F: return	gl::GL_RG;
            case gl::GL_RGB16F: return	gl::GL_RGB;
            case gl::GL_RGBA16F: return	gl::GL_RGBA;
            case gl::GL_R32F: return	gl::GL_RED;
            case gl::GL_RG32F: return	gl::GL_RG;
            case gl::GL_RGB32F: return	gl::GL_RGB;
            case gl::GL_RGBA32F: return	gl::GL_RGBA;
            case gl::GL_R11F_G11F_B10F: return	gl::GL_RGB;
            case gl::GL_RGB9_E5: return	gl::GL_RGB;
            case gl::GL_R8I: return	gl::GL_RED;
            case gl::GL_R8UI: return	gl::GL_RED;
            case gl::GL_R16I: return	gl::GL_RED;
            case gl::GL_R16UI: return	gl::GL_RED;
            case gl::GL_R32I: return	gl::GL_RED;
            case gl::GL_R32UI: return	gl::GL_RED;
            case gl::GL_RG8I: return	gl::GL_RG;
            case gl::GL_RG8UI: return	gl::GL_RG;
            case gl::GL_RG16I: return	gl::GL_RG;
            case gl::GL_RG16UI: return	gl::GL_RG;
            case gl::GL_RG32I: return	gl::GL_RG;
            case gl::GL_RG32UI: return	gl::GL_RG;
            case gl::GL_RGB8I: return	gl::GL_RGB;
            case gl::GL_RGB8UI: return	gl::GL_RGB;
            case gl::GL_RGB16I: return	gl::GL_RGB;
            case gl::GL_RGB16UI: return	gl::GL_RGB;
            case gl::GL_RGB32I: return	gl::GL_RGB;
            case gl::GL_RGB32UI: return	gl::GL_RGB;
            case gl::GL_RGBA8I: return	gl::GL_RGBA;
            case gl::GL_RGBA8UI: return	gl::GL_RGBA;
            case gl::GL_RGBA16I: return	gl::GL_RGBA;
            case gl::GL_RGBA16UI: return	gl::GL_RGBA;
            case gl::GL_RGBA32I: return	gl::GL_RGBA;
            case gl::GL_RGBA32UI: return	gl::GL_RGBA;
            default:
                return {};
        }
    }

    static gl::GLuint _createFramebufferTexture(ImageDefinition const & def)
    {
        bool multisampled=false;
        gl::GLuint outID;

        auto textureTarget = multisampled ? gl::GL_TEXTURE_2D_MULTISAMPLE : gl::GL_TEXTURE_2D;

        int  samples = 1;
        auto width   = def.width;
        auto height  = def.height;

        gl::glCreateTextures( textureTarget, 1, &outID);
        gl::glBindTexture(textureTarget, outID);
        if (multisampled)
        {
            auto internalFormat = _getInternalFormatFromDef(def);

            glTexImage2DMultisample(gl::GL_TEXTURE_2D_MULTISAMPLE,
                                    samples,
                                    internalFormat,
                                    static_cast<gl::GLsizei>(width),
                                    static_cast<gl::GLsizei>(height),
                                    gl::GL_FALSE);
        }
        else
        {
            auto internalFormat = _getInternalFormatFromDef(def);
            auto format = _getFormatFromDef(def);
            gl::glTexImage2D(gl::GL_TEXTURE_2D,
                             0,
                             internalFormat,
                             static_cast<gl::GLsizei>(width),
                             static_cast<gl::GLsizei>(height),
                             0,
                             format,
                             gl::GL_UNSIGNED_BYTE,
                             nullptr);

            gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR);
            gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);
            gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_R, gl::GL_CLAMP_TO_EDGE);
            gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_CLAMP_TO_EDGE);
            gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_CLAMP_TO_EDGE);
        }
        gl::glBindTexture(textureTarget, 0);
        return outID;
    }

    /**
     * @brief init
     * @param G
     *
     */
    void initGraphResources(FrameGraph & G)
    {
        for(auto & [name, imgDef] : G.m_images)
        {
            bool multisampled=false;
            int samples = 1;

            _imageNames[name].textureID = _createFramebufferTexture(imgDef);
        }

        auto order = G.findExecutionOrder();
        for(auto & x : order)
        {
            auto & n = G.nodes.at(x);
            if( std::holds_alternative<RenderPassNode>(n) )
            {
                auto & N = std::get<RenderPassNode>(n);

                for(auto & oT : N.outputRenderTargets)
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

    void releaseGraphResources(FrameGraph & G)
    {
        auto order = G.findExecutionOrder();
        std::reverse(order.begin(), order.end());
        for(auto & x : order)
        {
            auto & n = G.nodes.at(x);
            if( std::holds_alternative<RenderPassNode>(n) )
            {
                auto & N = std::get<RenderPassNode>(n);

                _nodes[x].isInit = false;
                //
                // Destroy the images/framebuffers/renderpasses

            }
        }

        for(auto & [name, img] : _imageNames)
        {
            gl::glDeleteTextures( 1, &img.textureID);
            img.textureID = 0;
        }
    }

    void operator()(FrameGraph & G)
    {
        auto order = G.findExecutionOrder();

        for(auto & x : order)
        {
            auto & n = G.nodes.at(x);
            if( std::holds_alternative<RenderPassNode>(n) )
            {
                auto & N = std::get<RenderPassNode>(n);

                Frame F;

                if(N.func)
                {
                    N.func(F);
                }
            }
        }

        for(auto & x : order)
        {
            auto & n = G.nodes.at(x);
            if( std::holds_alternative<RenderPassNode>(n) )
            {
                auto & N = std::get<RenderPassNode>(n);

                Frame F;

                if(N.func)
                {
                    N.func(F);
                }
            }
        }
    }
};


#endif
