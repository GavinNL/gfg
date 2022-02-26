#include <catch2/catch.hpp>
#include <frameGraph/frameGraph.h>

SCENARIO("test")
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


    auto & geometryPass = std::get<RenderPassNode>(G.getNodes().at("geometryPass"));
    auto & Final = std::get<RenderPassNode>(G.getNodes().at("Final"));
    auto & C1 = std::get<RenderTargetNode>(G.getNodes().at("C1"));
    auto & D1 = std::get<RenderTargetNode>(G.getNodes().at("D1"));

    //C1.imageResource.
    //rC1.inputRenderTargets.at(0).name
}
