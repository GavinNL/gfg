#ifndef GNL_FRAME_GRAPH_OPENGL_H
#define GNL_FRAME_GRAPH_OPENGL_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "../frameGraph.h"


//#include <glbinding/glbinding.h>
//#include <glbinding-aux/ContextInfo.h>
#include <glbinding/gl/gl.h>
//#include <glbinding/glbinding.h>

struct OpenGLGraph
{
    struct Frame {
        gl::GLuint              frameBuffer = 0;
        std::vector<gl::GLuint> inputAttachments;
        uint32_t                width  = 0;
        uint32_t                height = 0;
    };

    struct GLNodeInfo {
        bool                    isInit      = false;
        gl::GLuint              framebuffer = 0;
        std::vector<gl::GLuint> inputAttachments;
        uint32_t                width  = 0;
        uint32_t                height = 0;
    };

    struct GLImageInfo {
        gl::GLuint textureID = 0;
        uint32_t   width     = 0;
        uint32_t   height    = 0;
        bool       resizable = true;
    };

    std::map<std::string, GLNodeInfo>                   _nodes;
    std::map<std::string, GLImageInfo>                  _imageNames;
    std::map<std::string, std::function<void(Frame &)>> _renderers;
    std::vector<std::string>                            _execOrder;
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
            auto &n = G.nodes.at(x);
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
            auto &n = G.nodes.at(x);
            if (std::holds_alternative<RenderPassNode>(n))
            {
                auto &N = std::get<RenderPassNode>(n);

                _nodes[x].isInit = false;
                //
                // Destroy the images/framebuffers/renderpasses
            }
        }

        for (auto &[name, img] : _imageNames)
        {
            gl::glDeleteTextures(1, &img.textureID);
            img.textureID = 0;
        }
    }

    void resize(FrameGraph &G, uint32_t width, uint32_t height)
    {
        _execOrder = G.findExecutionOrder();
        // first go through all the images that have already been
        // created and destroy the ones that are not resizable
        for(auto it = _imageNames.begin(); it!=_imageNames.end();)
        {
            auto &img = it->second;
            if(img.resizable)
            {
                if(img.textureID)
                {
                    gl::glDeleteTextures(1, &img.textureID);
                    img.textureID = 0;
                    it = _imageNames.erase(it);
                    spdlog::info("Image Deleted: {}", it->first);
                    continue;
                }
            }
            ++it;
        }

        // Second, go through all the images that need to be created
        // and create/recreate them.
        for (auto &[name, imgDef] : G.m_images)
        {
            bool multisampled = false;
            int  samples      = 1;
            bool resizable    = false;
            auto iDef = imgDef;
            if (iDef.width * iDef.height == 0)
            {
                iDef.width  = width;
                iDef.height = height;
                resizable = true;
            }
            if(_imageNames.count(name) == 0)
            {
                _imageNames[name].textureID = _createFramebufferTexture(iDef);
                _imageNames[name].width     = iDef.width;
                _imageNames[name].height    = iDef.height;
                _imageNames[name].resizable = resizable;
                spdlog::info("Image Created: {}   {}x{}", name, iDef.width, iDef.height);
            }


            spdlog::info("Texture2D created: {}", name);
        }

        for (auto &name : _execOrder)
        {
            auto &Nv = G.nodes.at(name);

            if (std::holds_alternative<RenderPassNode>(Nv))
            {
                auto &N       = std::get<RenderPassNode>(Nv);
                auto &_glNode = this->_nodes[name];

                for (auto r : N.inputRenderTargets)
                {
                    auto &RTN = std::get<RenderTargetNode>(G.nodes.at(r.name));

                    auto  imgName = RTN.imageResource.name;
                    auto &imgDef  = G.m_images.at(imgName);
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
                    auto &RTN = std::get<RenderTargetNode>(G.nodes.at(r.name));

                    auto  imgName = RTN.imageResource.name;
                    auto &imgDef  = G.m_images.at(imgName);
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
    }

    void operator()(FrameGraph & G)
    {
        for(auto & x : _execOrder)
        {
            auto & N = G.nodes.at(x);
            if( std::holds_alternative<RenderPassNode>(N))
            {
                auto &R = _renderers.at(x);
                Frame F;
                F.frameBuffer      = _nodes.at(x).framebuffer;
                F.inputAttachments = _nodes.at(x).inputAttachments;
                F.width            = _nodes.at(x).width;
                F.height           = _nodes.at(x).height;
                R(F);
            }
        }
    }

    void setRenderer(std::string const& renderPassName, std::function<void(Frame&)> f)
    {
        _renderers[renderPassName] = f;
    }



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
            case gl::GL_DEPTH_COMPONENT32F: return gl::GL_DEPTH_COMPONENT;
            case gl::GL_DEPTH24_STENCIL8: return gl::GL_DEPTH_COMPONENT;
            case gl::GL_DEPTH32F_STENCIL8: return gl::GL_DEPTH_COMPONENT;
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

            spdlog::info("Image Created: {}x{}", width, height);
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
            spdlog::info("Image Created: {}x{}", width, height);
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

};


#endif
