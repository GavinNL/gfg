#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_set>
#include <spdlog/spdlog.h>


struct Node
{
    std::string name;
    std::string type;
    std::vector<std::string> inputNodes;
    std::vector<std::string> outputNodes;

    std::string imageResource;

    Node& setOutputs(std::vector<std::string> const & outputTargets)
    {
        for(auto & i : outputTargets)
        {
            insertOutputNode(i);
        }
        return *this;
    }

    Node& setInputs(std::vector<std::string> const & inputTargets)
    {
        for(auto & i : inputTargets)
        {
            insertInputNode(i);
        }
        return *this;
    }

    Node& insertOutputNode(std::string nodeName)
    {
        outputNodes.push_back(nodeName);
        return *this;
    }
    Node& insertInputNode(std::string nodeName)
    {
        inputNodes.push_back(nodeName);
        return *this;
    }
};

struct FrameGraph
{
#define BE(A) std::begin(A),std::end(A)
    Node& insertNode(std::string name, std::string type)
    {
        if(nodes.count(name))
            throw std::runtime_error("Already exists");

        auto & G = nodes[name];
        G.name = name;
        G.type = type;
        return G;
    }

protected:
    std::vector<std::string> findEndNodes() const
    {
        std::vector<std::string> endNodes;
        for(auto & [name, N] : nodes)
        {
            if(N.outputNodes.size() == 0)
                endNodes.push_back(name);
        }
        return endNodes;
    }


    void _recursePushBack(std::string const & name, std::vector<std::string> & order) const
    {
        auto & n = nodes.at(name);
        order.push_back(name);
        for(auto & in : n.inputNodes)
        {
            _recursePushBack(in, order);
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
        std::map<std::string, Node> images;

        // first generate all the output images
        // go through each of the nodes and create the output
        // render target descriptions
        for(auto & n : nodes)
        {
            for(auto & o : n.second.outputNodes)
            {
                if( images.count(o) == 0)
                {
                    auto & nI = images[o];
                    nI.name = o;
                    nI.type = "renderTarget";
                    nI.inputNodes.push_back(n.first);
                }
            }
        }

        for(auto & n : nodes)
        {
            for(auto & in : n.second.inputNodes)
            {
                images.at(in).outputNodes.push_back(n.first);
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
            if(it->second.type == "renderTarget")
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

        //std::map<std::string, int32_t> imageUseCount;
        std::map<std::string, int32_t> renderTargetUseCount;

        auto _print = [&]()
        {
            for(auto & [rtName, count] : renderTargetUseCount)
            {
                spdlog::info("{} : {}", rtName, count);
            }
            spdlog::info("----");

        };
        _print();


        for(auto & name : order)
        {
            auto & N = nodes.at(name);
            if(N.type == "renderPass")
            {
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
            auto & N = nodes.at(name);
            if(N.type == "renderPass")
            {
                spdlog::info("Pass Name: {}", name);

                for(auto & outputName : N.outputNodes)
                {
                    auto & outRenderTarget = nodes.at(outputName);
                    auto renderTargetThatIsNotBeingUsed = findRenderTargetThatIsNotBeingUsed(renderTargetUseCount);

                    if(renderTargetThatIsNotBeingUsed.empty()) // no available image
                    {
                        // generate new image
                        outRenderTarget.imageResource = fmt::format("{}_img", outputName);
                        //imageUseCount[outRenderTarget.imageResource] = static_cast<int32_t>(outRenderTarget.outputNodes.size()) + 1;
                    }
                    else
                    {
                        outRenderTarget.imageResource = nodes.at(renderTargetThatIsNotBeingUsed).imageResource;
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
    }


    void printGraph() const
    {
        for(auto & [name, N] : nodes)
        {
            auto nn =
            fmt::format("({}) --> {} --> {}",
                           fmt::join(N.inputNodes.begin(), N.inputNodes.end(), ","),
                           name,
                           fmt::join(N.outputNodes.begin(), N.outputNodes.end(), ",")
                        );
            spdlog::info("{}", nn);
        }
    }

    Node& createRenderPass(std::string const & name)
    {
        return insertNode(name, "renderPass");
    }

    Node& createRenderPass(std::string const & name,
                           std::vector<std::string> const & outputTargets,
                           std::vector<std::string> const & inputTargets = {})
    {
        auto & N = insertNode(name, "renderPass");
        N.setInputs(inputTargets);
        N.setOutputs(outputTargets);
        for(auto & o :outputTargets)
        {
            N.insertOutputNode(o);
        }
        for(auto & i : inputTargets)
        {
            N.insertInputNode(i);
        }

        return N;
    }


    struct Image
    {
        std::string name;
        uint32_t width  = 0;
        uint32_t height = 0;
    };

    std::map< std::string, Image> m_images;
    std::map<std::string, Node> nodes;
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
        if( n.second.type == "renderTarget")
        {
            spdlog::info("{} is using image: {}", n.first, n.second.imageResource);
        }
    }
    return 0;
}
