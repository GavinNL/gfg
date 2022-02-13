#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <unordered_set>
#include <spdlog/spdlog.h>


struct RenderPassNode
{
    std::string name;
    std::string type;
    std::vector<std::string> inputNodes;  // input render targets
    std::vector<std::string> outputNodes; // output render targets

    RenderPassNode& setOutputs(std::vector<std::string> const & outputTargets)
    {
        for(auto & i : outputTargets)
        {
            insertOutputNode(i);
        }
        return *this;
    }

    RenderPassNode& setInputs(std::vector<std::string> const & inputTargets)
    {
        for(auto & i : inputTargets)
        {
            insertInputNode(i);
        }
        return *this;
    }

    RenderPassNode& insertOutputNode(std::string nodeName)
    {
        outputNodes.push_back(nodeName);
        return *this;
    }
    RenderPassNode& insertInputNode(std::string nodeName)
    {
        inputNodes.push_back(nodeName);
        return *this;
    }
};

struct RenderTargetNode
{
    std::string              name;
    std::string              writer;  // only one render pass can write to a render target
    std::vector<std::string> readers; // multiple render passes can read from this render target

    std::string imageResource;
};



using node = std::variant<RenderPassNode,RenderTargetNode>;

struct FrameGraph
{
#define BE(A) std::begin(A),std::end(A)
protected:
    std::vector<std::string> findEndNodes() const
    {
        // end node will always be a RenderTarget

        std::vector<std::string> endNodes;
        for(auto & [name, n] : nodes)
        {
            if(std::holds_alternative<RenderTargetNode>(n))
            {
                auto & N = std::get<RenderTargetNode>(n); // should always be a render pass node
                if(N.readers.size() == 0)
                    endNodes.push_back(name);
            }
        }
        return endNodes;
    }


    void _recursePushBack(std::string const & name, std::vector<std::string> & order) const
    {
        auto & n = nodes.at(name);
        order.push_back(name);

        if(std::holds_alternative<RenderPassNode>(n))
        {
            auto & N = std::get<RenderPassNode>(n); // should always be a render pass node
            for(auto & in : N.inputNodes)
            {
                _recursePushBack(in, order);
            }
        }
        else if(std::holds_alternative<RenderTargetNode>(n))
        {
            auto & N = std::get<RenderTargetNode>(n); // should always be a render pass node
            assert(!N.writer.empty());
            _recursePushBack(N.writer, order);
        }

    }
    std::vector<std::string> findExecutionOrder() const
    {
        auto endNodes = findEndNodes();
        std::vector<std::string> order;
        for(auto & name : endNodes)
        {
            _recursePushBack(name, order);
        }

        std::reverse(BE(order));
        std::unordered_set<std::string> seen;
        auto it = std::remove_if(BE(order),[&seen](auto & x)
        {
           return seen.insert(x).second == false;
        });
        order.erase(it, order.end());
        return order;
    }


    // returns the name of the render target which
    // has an image but is no longer being used
    std::string findRenderTargetThatIsNotBeingUsed(std::map<std::string, int32_t> & renderTargetUsageCount)
    {
        for(auto & [name, count] : renderTargetUsageCount)
        {
            if(count == 0)
            {
                return name;
            }
        }
        return "";
    }

    void generateImages()
    {
        std::map<std::string, RenderTargetNode> images;

        // first generate all the output images
        // go through each of the nodes and create the output
        // render target descriptions
        for(auto & [name,n] : nodes)
        {
            auto & N = std::get<RenderPassNode>(n);
            for(auto & o : N.outputNodes)
            {
                if( images.count(o) == 0)
                {
                    auto & nI = images[o];
                    nI.name   = o;
                    nI.writer = name;
                }
            }
        }

        // Tell each image which render pass is reading from it
        for(auto & n : nodes)
        {
            auto & N = std::get<RenderPassNode>(n.second);
            for(auto & in : N.inputNodes)
            {
                images.at(in).readers.push_back(n.first);
            }
        }

        // copy all the image nodes into
        // node list
        for(auto & [n,i] : images)
        {
            nodes[n] = i;
        }
    }

public:
    void allocateImages()
    {
        for(auto it=nodes.begin();it!=nodes.end();)
        {
            if( std::holds_alternative<RenderTargetNode>(it->second) )
            {
                it = nodes.erase(it);
            }
            else
            {
                it++;
            }
        }
        generateImages();

        auto order = findExecutionOrder();

        spdlog::info("Execute Order: {}", fmt::join(BE(order),","));
        //std::map<std::string, int32_t> imageUseCount;
        std::map<std::string, int32_t> renderTargetUseCount;

        auto _print = [&]()
        {
            for(auto & [rtName, count] : renderTargetUseCount)
            {
                spdlog::info("{} : {}   {}", rtName, count, std::get<RenderTargetNode>(nodes.at(rtName)).imageResource);
            }
            spdlog::info("----");

        };
        _print();


        for(auto & name : order)
        {
            auto & n = nodes.at(name);
            if( std::holds_alternative<RenderPassNode>(n) )
            {
                auto & N = std::get<RenderPassNode>(n);
                for(auto & n : N.outputNodes)
                {
                    renderTargetUseCount[n]++;
                }
                for(auto & n : N.inputNodes)
                {
                    renderTargetUseCount[n]++;
                }
            }
        }

        for(auto & name : order)
        {
            auto & _n = nodes.at(name);
            if( !std::holds_alternative<RenderPassNode>(_n))
                continue;
            auto & N = std::get<RenderPassNode>(_n);

            spdlog::info("Pass Name: {}", name);

            for(auto & outputName : N.outputNodes)
            {
                auto & outRenderTarget = std::get<RenderTargetNode>(nodes.at(outputName));
                auto renderTargetThatIsNotBeingUsed = findRenderTargetThatIsNotBeingUsed(renderTargetUseCount);

                if(renderTargetThatIsNotBeingUsed.empty()) // no available image
                {
                    // generate new image
                    outRenderTarget.imageResource = fmt::format("{}_img", outputName);
                }
                else
                {
                    outRenderTarget.imageResource = std::get<RenderTargetNode>(nodes.at(renderTargetThatIsNotBeingUsed)).imageResource;
                    renderTargetUseCount.at(renderTargetThatIsNotBeingUsed)++;
                }
            }

            _print();

            for(auto & outputName : N.outputNodes)
            {
                //imageUseCount[nodes.at(outputName).imageResource]--;
                renderTargetUseCount.at(outputName)--;
            }
            for(auto & inputName : N.inputNodes)
            {
                //imageUseCount[nodes.at(inputName).imageResource]--;
                renderTargetUseCount.at(inputName)--;
            }
        }
    }


    //void printGraph() const
    //{
    //    for(auto & [name, N] : nodes)
    //    {
    //        auto nn =
    //        fmt::format("({}) --> {} --> {}",
    //                       fmt::join(N.inputNodes.begin(), N.inputNodes.end(), ","),
    //                       name,
    //                       fmt::join(N.outputNodes.begin(), N.outputNodes.end(), ",")
    //                    );
    //        spdlog::info("{}", nn);
    //    }
    //}

    RenderPassNode& createRenderPass(std::string const & name)
    {
        RenderPassNode RPN;
        RPN.name = name;
        nodes[name] = RPN;
        return std::get<RenderPassNode>(nodes[name]);
    }

    RenderPassNode& createRenderPass(std::string const & name,
                           std::vector<std::string> const & outputTargets,
                           std::vector<std::string> const & inputTargets = {})
    {
        auto & N = createRenderPass(name);
        N.setInputs(inputTargets);
        N.setOutputs(outputTargets);
//        for(auto & o :outputTargets)
//        {
//            N.insertOutputNode(o);
//        }
//        for(auto & i : inputTargets)
//        {
//            N.insertInputNode(i);
//        }

        return N;
    }


    struct Image
    {
        std::string name;
        uint32_t width  = 0;
        uint32_t height = 0;
    };

    std::map< std::string, Image> m_images;
    std::map<std::string, node> nodes;
};

int main(int argc, char ** argv)
{
    FrameGraph G;

    G.createRenderPass("G", {"C1"});

    G.createRenderPass("H", {"C2"}, {"C1"});

    G.createRenderPass("V", {"C3"}, {"C2"});

    G.createRenderPass("P", {"C4","C7"}, {"C1","C3"});

    G.createRenderPass("Q", {"C5"}, {"C4"});

    G.createRenderPass("R", {"C6"}, {"C5", "C7"});

//    G.generateImages();
//
//    G.printGraph();

//    auto endNodes = G.findEndNodes();
//    spdlog::info("End Nodes: {}", fmt::format("{}", fmt::join(BE(endNodes), ",")));
//
//    auto order = G.findExecutionOrder();
//    spdlog::info("Execution Order: {}", fmt::format("{}", fmt::join(BE(order), ",")));

    G.allocateImages();

    for(auto & n : G.nodes)
    {
        if( std::holds_alternative<RenderTargetNode>(n.second) )
        {
            spdlog::info("{} is using image: {}", n.first, std::get<RenderTargetNode>(n.second).imageResource);
        }
    }
    return 0;
}
