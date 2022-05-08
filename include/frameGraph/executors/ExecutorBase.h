#ifndef GNL_FRAME_GRAPH_EXECUTOR_BASE_H
#define GNL_FRAME_GRAPH_EXECUTOR_BASE_H

#include <vector>
#include <string>
#include "../frameGraph.h"

namespace gfg
{
struct ExecutorBase
{
    /**
     * @brief generateImage
     * @param imageName
     * @param format
     * @param width
     * @param height
     *
     * This function is used to generate an image that will be rendered to.
     * It will be identified by imageName.
     */
    virtual void generateImage(std::string const & imageName, FrameGraphFormat format, uint32_t width, uint32_t height) = 0;

    /**
     * @brief destroyImage
     * @param imageName
     *
     * destroyImage will be called when we need to destroy the image.
     * It is up to the implementation to determine whether to actually destroy
     * the image or keep the image in cache for whatever reason.
     * DestroyImage will be called when the framegraph changes size and
     * the images need to resized
     */
    virtual void destroyImage(std::string const & imageName) = 0;

    /**
     * @brief buildFrameBuffer
     * @param renderPassName
     * @param outputTargetImages
     * @param inputSampledImages
     *
     * This function is called when a framebuffer needs to be built.
     * The outputTargetImages are the names of the images that should be used
     * for rendering to.
     *
     * inputSampled images are the list of images that are going to be sampled
     * from.
     *
     */
    virtual void buildFrameBuffer(std::string const & renderPassName, std::vector<std::string> const & outputTargetImages, std::vector<std::string> const & inputSampledImages) = 0;

    /**
     * @brief destroyFrameBuffer
     * @param renderPassName
     *
     * This function is called when the framebuffer needs to be destroyed. It is
     * usually called before buildFrameBuffer( ).
     *
     * If the framebuffer is already destroyed. This function should return gracefully.
     *
     */
    virtual void destroyFrameBuffer(std::string const & renderPassName) = 0;


    /**
     * @brief preResize
     *
     * This function is called before any resizing occurs.
     */
    virtual void preResize() = 0;

    /**
     * @brief postResize
     *
     * This function is called after all the resizing has occured. Ie; all images
     * have been destroyed and recreated. All framebuffers have been destroyed
     * and recreated.
     */
    virtual void postResize() = 0;


    /**
     * @brief resize
     * @param G
     * @param width
     * @param height
     *
     * Resizes the framgraph. This should be called whenever
     * your window changes size.
     */
    void resize(FrameGraph &G, uint32_t width, uint32_t height)
    {
        m_execOrder = G.findExecutionOrder();

        preResize();

        m_windowWidth = width;
        m_windowHeight = height;

        // Second, go through all the images that need to be created
        // and create/recreate them.
        for (auto &[name, imgDef] : G.getImages())
        {
            auto iDef         = imgDef;

            if (iDef.width * iDef.height == 0)
            {
                iDef.width  = width;
                iDef.height = height;
            }
            if(imgDef.resizable)
            {
                destroyImage(name);
            }
            generateImage(name, iDef.format, iDef.width, iDef.height);

        }

        for (auto &name : m_execOrder)
        {
            auto &Nv = G.getNodes().at(name);

            if (std::holds_alternative<RenderPassNode>(Nv))
            {
                auto &N       = std::get<RenderPassNode>(Nv);

                std::vector<std::string> outputTargetNames;
                std::vector<std::string> inputSampledImageNames;

                for (auto r : N.outputRenderTargets)
                {
                    auto &RTN = std::get<RenderTargetNode>(G.getNodes().at(r.name));
                    outputTargetNames.push_back(RTN.imageResource.name);
                }
                for (auto r : N.inputSampledRenderTargets)
                {
                    auto &RTN = std::get<RenderTargetNode>(G.getNodes().at(r.name));
                    inputSampledImageNames.push_back(RTN.imageResource.name);
                }

                destroyFrameBuffer(name);
                buildFrameBuffer(name, outputTargetNames, inputSampledImageNames);
            }

        }

        postResize();
    }

protected:
    std::vector<std::string> m_execOrder;
    uint32_t m_windowWidth  = 0;
    uint32_t m_windowHeight = 0;
};
}

#endif
