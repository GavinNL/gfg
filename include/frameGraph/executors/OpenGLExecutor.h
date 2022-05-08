#ifndef GNL_FRAME_GRAPH_OPENGL_H
#define GNL_FRAME_GRAPH_OPENGL_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include "../frameGraph.h"
#include "ExecutorBase.h"

#include <glbinding/gl/gl.h>

namespace gfg
{
struct FrameGraphExecutor_OpenGL : public ExecutorBase
{
    struct Frame : public FrameBase
    {
        gl::GLuint              frameBuffer = 0;
        std::vector<gl::GLuint> inputAttachments;

        void bindFramebuffer()
        {
            gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, frameBuffer);
        }
        void bindInputTextures(uint32_t firstTextureIndex)
        {
            for(uint32_t i=0;i<inputAttachments.size();i++)
            {
                gl::glActiveTexture(gl::GL_TEXTURE0 + firstTextureIndex + i); // activate the texture unit first before binding texture
                gl::glBindTexture(gl::GL_TEXTURE_2D, inputAttachments[i]);
            }
        }
    };


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
    void init()
    {

    }

    void destroy()
    {
        for(auto & x : _nodes)
        {
            gl::glDeleteFramebuffers(1, &x.second.framebuffer);
            x.second.framebuffer = 0;
        }
        for(auto & x : _imageNames)
        {
            gl::glDeleteTextures(1, &x.second.textureID);
            x.second.textureID = 0;
        }
        _imageNames.clear();
        _nodes.clear();
    }


    void operator()(FrameGraph & G)
    {
        for(auto & x : m_execOrder)
        {
            auto & N = G.getNodes().at(x);
            if( std::holds_alternative<RenderPassNode>(N))
            {
                auto &R = _renderers.at(x);
                Frame F;
                auto & node = _nodes.at(x);
                F.frameBuffer      = node.framebuffer;
                F.inputAttachments = node.inputAttachments;
                F.imageWidth       = node.width;
                F.imageHeight      = node.height;
                F.renderableWidth  = node.width;
                F.renderableHeight = node.height;

                if(node.outputAttachments.size() == 0)
                {
                    F.imageWidth       = m_windowWidth;
                    F.imageHeight      = m_windowHeight;
                    F.renderableWidth  = m_windowWidth;
                    F.renderableHeight = m_windowHeight;
                    F.windowWidth      = m_windowWidth;
                    F.windowHeight     = m_windowHeight;
                }
                R(F);
            }
        }
    }



protected:

    static gl::GLenum _getInternalFormatFromDef(FrameGraphFormat format)
    {
        switch(format)
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

    static gl::GLenum _getFormatFromDef(FrameGraphFormat format)
    {
        switch(_getInternalFormatFromDef(format))
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

    static gl::GLuint _createFramebufferTexture(FrameGraphFormat format, uint32_t width, uint32_t height)
    {
        bool multisampled=false;
        gl::GLuint outID;

        auto textureTarget = multisampled ? gl::GL_TEXTURE_2D_MULTISAMPLE : gl::GL_TEXTURE_2D;

        int  samples = 1;
        //auto width   = def.width;
        //auto height  = def.height;

        gl::glCreateTextures( textureTarget, 1, &outID);
        gl::glBindTexture(textureTarget, outID);
        if (multisampled)
        {
            auto internalFormat = _getInternalFormatFromDef(format);

            GFG_INFO("Image Created: {}x{}", width, height);
            glTexImage2DMultisample(gl::GL_TEXTURE_2D_MULTISAMPLE,
                                    samples,
                                    internalFormat,
                                    static_cast<gl::GLsizei>(width),
                                    static_cast<gl::GLsizei>(height),
                                    gl::GL_FALSE);
        }
        else
        {
            auto internalFormat = _getInternalFormatFromDef(format);
            auto glFormat = _getFormatFromDef(format);
            GFG_INFO("Image Created: {}x{}", width, height);
            gl::glTexImage2D(gl::GL_TEXTURE_2D,
                             0,
                             internalFormat,
                             static_cast<gl::GLsizei>(width),
                             static_cast<gl::GLsizei>(height),
                             0,
                             glFormat,
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


    // ExecutorBase interface
public:
    void generateImage(const std::string &imageName, FrameGraphFormat format, uint32_t width, uint32_t height)
    {
        if(_imageNames.count(imageName) != 0)
            return;

        _imageNames[imageName].textureID = _createFramebufferTexture(format, width, height);
        _imageNames[imageName].width     = width;
        _imageNames[imageName].height    = height;
        _imageNames[imageName].format    = format;
    }
    void destroyImage(const std::string &imageName)
    {
        if(_imageNames.count(imageName) == 0)
            return;

        auto & img = _imageNames.at(imageName);
        if(img.textureID)
        {
            gl::glDeleteTextures(1, &img.textureID);
            img.textureID = 0;
            GFG_INFO("Image Deleted: {}", imageName);
        }
        _imageNames.erase(imageName);
    }
    void buildFrameBuffer(const std::string &renderPassName, const std::vector<std::string> &outputTargetImages, const std::vector<std::string> &inputSampledImages)
    {
        auto & _glNode = _nodes[renderPassName];
        if(outputTargetImages.size() == 0)
        {
            return;
        }
        if(_glNode.framebuffer==0)
        {
            gl::GLuint framebuffer;
            gl::glGenFramebuffers(1, &framebuffer);
            _glNode.framebuffer = framebuffer;
        }
        gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, _glNode.framebuffer );

        uint32_t i = 0;

        _glNode.outputAttachments.clear();
        for (auto imgName : outputTargetImages)
        {
            //auto &RTN = std::get<RenderTargetNode>(G.getNodes().at(r.name));
            //auto  imgName = RTN.imageResource.name;
            auto &imgDef  =  _imageNames.at(imgName);
            auto  imgID   = _imageNames.at(imgName).textureID;


            _glNode.width  = imgDef.width;
            _glNode.height = imgDef.height;
            gl::glBindTexture(gl::GL_TEXTURE_2D, imgID);

            if( isDepth(imgDef.format) )
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

            }
            _glNode.outputAttachments.push_back(imgID);
            // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
        }

        if (gl::glCheckFramebufferStatus(gl::GL_FRAMEBUFFER) != gl::GL_FRAMEBUFFER_COMPLETE)
        {
            GFG_ERROR("Framebuffer for, {}, is not complete!", renderPassName);
        }

        for(auto imgName : inputSampledImages)
        {
            auto &imgDef = _imageNames.at(imgName);
            auto  imgID  = _imageNames.at(imgName).textureID;
            _glNode.inputAttachments.push_back(imgID);
        }

        _glNode.isInit = true;
        gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);

    }

    void destroyFrameBuffer(const std::string &renderPassName)
    {

    }
    void preResize()
    {

    }
    void postResize()
    {

    }

    struct GLNodeInfo {
        bool                    isInit      = false;
        gl::GLuint              framebuffer = 0;
        std::vector<gl::GLuint> inputAttachments;
        std::vector<gl::GLuint> outputAttachments;
        uint32_t                width  = 0;
        uint32_t                height = 0;
    };

    struct GLImageInfo {
        gl::GLuint textureID = 0;
        uint32_t   width     = 0;
        uint32_t   height    = 0;
        bool       resizable = true;
        FrameGraphFormat format;
    };

    std::map<std::string, GLNodeInfo>                   _nodes;
    std::map<std::string, GLImageInfo>                  _imageNames;
    std::map<std::string, std::function<void(Frame &)>> _renderers;
};
}

#endif
