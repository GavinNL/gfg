#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <vulkan/vulkan.h>
#include <unordered_set>
#include <spdlog/spdlog.h>


struct RenderTargetDefinition
{
    std::string name;
    VkFormat    format = VK_FORMAT_UNDEFINED;
    uint32_t    width  = 0;
    uint32_t    height = 0;
};

struct ImageDefinition
{
    std::string name;
    VkFormat    format = VK_FORMAT_UNDEFINED;
    uint32_t    width  = 0;
    uint32_t    height = 0;
};


struct RenderPassNode
{
    std::string name;

    std::vector<RenderTargetDefinition> inputRenderTargets;  // input render targets
    std::vector<RenderTargetDefinition> outputRenderTargets; // output render targets

    RenderPassNode& input(std::string name)
    {
        inputRenderTargets.push_back({name});
        return *this;
    }
    RenderPassNode& output(std::string name, VkFormat format=VK_FORMAT_UNDEFINED)
    {
        outputRenderTargets.push_back({name,format});
        return *this;
    }
};

struct RenderTargetNode
{
    std::string              name;
    std::string              writer;  // only one render pass can write to a render target
    std::vector<std::string> readers; // multiple render passes can read from this render target

    RenderTargetDefinition   imageResource;
    //std::string imageResource;
};



using node = std::variant<RenderPassNode,RenderTargetNode>;

struct FrameGraph
{

#define BE(A) std::begin(A),std::end(A)
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
            for(auto & in : N.inputRenderTargets)
            {
                _recursePushBack(in.name, order);
            }
        }
        else if(std::holds_alternative<RenderTargetNode>(n))
        {
            auto & N = std::get<RenderTargetNode>(n); // should always be a render pass node
            assert(!N.writer.empty());
            _recursePushBack(N.writer, order);
        }

    }

    // returns the name of the render target which
    // has an image but is no longer being used
    std::string findImageThatIsNotBeingUsed(std::map<std::string, int32_t> & renderTargetUsageCount, RenderTargetDefinition const & def)
    {
        for(auto & [name, count] : renderTargetUsageCount)
        {
            if(count == 0)
            {
                auto & N = std::get<RenderTargetNode>(nodes.at(name));
                //auto & NN = std::get<RenderTargetNode>()
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
            for(auto & o : N.outputRenderTargets)
            {
                if( images.count(o.name) == 0)
                {
                    auto & nI = images[o.name];
                    nI.name   = o.name;
                    nI.writer = name;
                }
            }
        }

        // Tell each image which render pass is reading from it
        for(auto & n : nodes)
        {
            auto & N = std::get<RenderPassNode>(n.second);
            for(auto & in : N.inputRenderTargets)
            {
                images.at(in.name).readers.push_back(n.first);
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
        std::map<std::string, int32_t> imageUseCount;

        auto _print = [&]()
        {
            for(auto & [rtName, count] : imageUseCount)
            {
                spdlog::info("{} : {}   {}", rtName, count, std::get<RenderTargetNode>(nodes.at(rtName)).imageResource.name);
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
                for(auto & n : N.outputRenderTargets)
                {
                    imageUseCount[n.name]++;
                }
                for(auto & n : N.inputRenderTargets)
                {
                    imageUseCount[n.name]++;
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

            for(auto & outTarget : N.outputRenderTargets)
            {
                auto & outRenderTarget = std::get<RenderTargetNode>(nodes.at(outTarget.name));
                auto imageThatIsNotBeingUsed = findImageThatIsNotBeingUsed(imageUseCount, outTarget);

                if(imageThatIsNotBeingUsed.empty()) // no available image
                {
                    // generate new image
                    auto imageName = fmt::format("{}_img", outTarget.name);
                    outRenderTarget.imageResource.name   = imageName;
                    outRenderTarget.imageResource.format = outTarget.format;

                    ImageDefinition imgDef;
                    imgDef.name   = imageName;
                    imgDef.format = outTarget.format;
                    imgDef.width  = outTarget.width;
                    imgDef.height = outTarget.height;

                    m_images[imageName] = imgDef;
                }
                else
                {
                    outRenderTarget.imageResource.name = std::get<RenderTargetNode>(nodes.at(imageThatIsNotBeingUsed)).imageResource.name;
                    imageUseCount.at(imageThatIsNotBeingUsed)++;
                }
            }

            _print();

            for(auto & outTarget : N.outputRenderTargets)
            {
                //imageUseCount[nodes.at(outputName).imageResource]--;
                imageUseCount.at(outTarget.name)--;
            }
            for(auto & inTarget : N.inputRenderTargets)
            {
                //imageUseCount[nodes.at(inputName).imageResource]--;
                imageUseCount.at(inTarget.name)--;
            }
        }
    }


    //void printGraph() const
    //{
    //    for(auto & [name, N] : nodes)
    //    {
    //        auto nn =
    //        fmt::format("({}) --> {} --> {}",
    //                       fmt::join(N.inputRenderTargets.begin(), N.inputRenderTargets.end(), ","),
    //                       name,
    //                       fmt::join(N.outputRenderTargets.begin(), N.outputRenderTargets.end(), ",")
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

    std::map< std::string, ImageDefinition> m_images;
    std::map<std::string, node> nodes;
};

int main(int argc, char ** argv)
{
    FrameGraph G;

#if 1
    G.createRenderPass("ShadowPass")
     .output("SM", VK_FORMAT_D32_SFLOAT);

    G.createRenderPass("geometryPass")
     .input("SM")
     .output("C1", VK_FORMAT_R8G8B8A8_UNORM);

    G.createRenderPass("HBlur1")
     .input("C1")
     .output("B1h", VK_FORMAT_R8G8B8A8_UNORM);

    G.createRenderPass("VBlur1")
     .input("B1h")
     .output("B1v",VK_FORMAT_R8G8B8A8_UNORM);

    G.createRenderPass("Final")
     .input("C1")
     .input("B1v")
     .output("F", VK_FORMAT_R8G8B8A8_UNORM);
#else
    G.createRenderPass("G", {"C1"});


    G.createRenderPass("H", {"C2"}, {"C1"});

    G.createRenderPass("V", {"C3"}, {"C2"});

    G.createRenderPass("P", {"C4","C7"}, {"C1","C3"});

    G.createRenderPass("Q", {"C5"}, {"C4"});

    G.createRenderPass("R", {"C6"}, {"C5", "C7"});
#endif
    G.allocateImages();

    auto order = G.findExecutionOrder();
    spdlog::info("Order: {}", fmt::join(BE(order),","));
    for(auto & n : G.nodes)
    {
        if( std::holds_alternative<RenderTargetNode>(n.second) )
        {
            spdlog::info("{} is using image: {}", n.first, std::get<RenderTargetNode>(n.second).imageResource.name);
        }
    }
    return 0;
}
