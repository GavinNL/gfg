#ifndef GNL_FRAME_GRAPH_H
#define GNL_FRAME_GRAPH_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <unordered_set>
#include <spdlog/spdlog.h>


/**
 * @brief The FrameGraphFormat enum
 *
 * This enum is copied from VkFormat
 */
enum class FrameGraphFormat
{
    UNDEFINED = 0,
    //R4G4_UNORM_PACK8 = 1,
    //R4G4B4A4_UNORM_PACK16 = 2,
    //B4G4R4A4_UNORM_PACK16 = 3,
    //R5G6B5_UNORM_PACK16 = 4,
    //B5G6R5_UNORM_PACK16 = 5,
    //R5G5B5A1_UNORM_PACK16 = 6,
    //B5G5R5A1_UNORM_PACK16 = 7,
    //A1R5G5B5_UNORM_PACK16 = 8,
    R8_UNORM = 9,
    R8_SNORM = 10,
    //R8_USCALED = 11,
    //R8_SSCALED = 12,
    R8_UINT = 13,
    R8_SINT = 14,
    //R8_SRGB = 15,
    R8G8_UNORM = 16,
    R8G8_SNORM = 17,
    //R8G8_USCALED = 18,
    //R8G8_SSCALED = 19,
    R8G8_UINT = 20,
    R8G8_SINT = 21,
    //R8G8_SRGB = 22,
    R8G8B8_UNORM = 23,
    R8G8B8_SNORM = 24,
    //R8G8B8_USCALED = 25,
    //R8G8B8_SSCALED = 26,
    R8G8B8_UINT = 27,
    R8G8B8_SINT = 28,
    //R8G8B8_SRGB = 29,
    //B8G8R8_UNORM = 30,
    //B8G8R8_SNORM = 31,
    //B8G8R8_USCALED = 32,
    //B8G8R8_SSCALED = 33,
    //B8G8R8_UINT = 34,
    //B8G8R8_SINT = 35,
    //B8G8R8_SRGB = 36,
    R8G8B8A8_UNORM = 37,
    R8G8B8A8_SNORM = 38,
    //R8G8B8A8_USCALED = 39,
    //R8G8B8A8_SSCALED = 40,
    R8G8B8A8_UINT = 41,
    R8G8B8A8_SINT = 42,
    //R8G8B8A8_SRGB = 43,
    //B8G8R8A8_UNORM = 44,
    //B8G8R8A8_SNORM = 45,
    //B8G8R8A8_USCALED = 46,
    //B8G8R8A8_SSCALED = 47,
    //B8G8R8A8_UINT = 48,
    //B8G8R8A8_SINT = 49,
    //B8G8R8A8_SRGB = 50,
    //A8B8G8R8_UNORM_PACK32 = 51,
    //A8B8G8R8_SNORM_PACK32 = 52,
    //A8B8G8R8_USCALED_PACK32 = 53,
    //A8B8G8R8_SSCALED_PACK32 = 54,
    //A8B8G8R8_UINT_PACK32 = 55,
    //A8B8G8R8_SINT_PACK32 = 56,
    //A8B8G8R8_SRGB_PACK32 = 57,
    //A2R10G10B10_UNORM_PACK32 = 58,
    //A2R10G10B10_SNORM_PACK32 = 59,
    //A2R10G10B10_USCALED_PACK32 = 60,
    //A2R10G10B10_SSCALED_PACK32 = 61,
    //A2R10G10B10_UINT_PACK32 = 62,
    //A2R10G10B10_SINT_PACK32 = 63,
    //A2B10G10R10_UNORM_PACK32 = 64,
    //A2B10G10R10_SNORM_PACK32 = 65,
    //A2B10G10R10_USCALED_PACK32 = 66,
    //A2B10G10R10_SSCALED_PACK32 = 67,
    //A2B10G10R10_UINT_PACK32 = 68,
    //A2B10G10R10_SINT_PACK32 = 69,
    R16_UNORM = 70,
    R16_SNORM = 71,
    //R16_USCALED = 72,
    //R16_SSCALED = 73,
    R16_UINT = 74,
    R16_SINT = 75,
    R16_SFLOAT = 76,
    R16G16_UNORM = 77,
    R16G16_SNORM = 78,
    //R16G16_USCALED = 79,
    //R16G16_SSCALED = 80,
    R16G16_UINT = 81,
    R16G16_SINT = 82,
    R16G16_SFLOAT = 83,
    R16G16B16_UNORM = 84,
    R16G16B16_SNORM = 85,
    //R16G16B16_USCALED = 86,
    //R16G16B16_SSCALED = 87,
    R16G16B16_UINT = 88,
    R16G16B16_SINT = 89,
    R16G16B16_SFLOAT = 90,
    R16G16B16A16_UNORM = 91,
    R16G16B16A16_SNORM = 92,
    //R16G16B16A16_USCALED = 93,
    //R16G16B16A16_SSCALED = 94,
    R16G16B16A16_UINT = 95,
    R16G16B16A16_SINT = 96,
    R16G16B16A16_SFLOAT = 97,
    R32_UINT = 98,
    R32_SINT = 99,
    R32_SFLOAT = 100,
    R32G32_UINT = 101,
    R32G32_SINT = 102,
    R32G32_SFLOAT = 103,
    R32G32B32_UINT = 104,
    R32G32B32_SINT = 105,
    R32G32B32_SFLOAT = 106,
    R32G32B32A32_UINT = 107,
    R32G32B32A32_SINT = 108,
    R32G32B32A32_SFLOAT = 109,
    //R64_UINT = 110,
    //R64_SINT = 111,
    //R64_SFLOAT = 112,
    //R64G64_UINT = 113,
    //R64G64_SINT = 114,
    //R64G64_SFLOAT = 115,
    //R64G64B64_UINT = 116,
    //R64G64B64_SINT = 117,
    //R64G64B64_SFLOAT = 118,
    //R64G64B64A64_UINT = 119,
    //R64G64B64A64_SINT = 120,
    //R64G64B64A64_SFLOAT = 121,
    //B10G11R11_UFLOAT_PACK32 = 122,
    //E5B9G9R9_UFLOAT_PACK32 = 123,
    //D16_UNORM = 124,
    //X8_D24_UNORM_PACK32 = 125,
    D32_SFLOAT = 126,
    //S8_UINT = 127,
    //D16_UNORM_S8_UINT = 128,
    D24_UNORM_S8_UINT = 129,
    D32_SFLOAT_S8_UINT = 130,
    //BC1_RGB_UNORM_BLOCK = 131,
    //BC1_RGB_SRGB_BLOCK = 132,
    //BC1_RGBA_UNORM_BLOCK = 133,
    //BC1_RGBA_SRGB_BLOCK = 134,
    //BC2_UNORM_BLOCK = 135,
    //BC2_SRGB_BLOCK = 136,
    //BC3_UNORM_BLOCK = 137,
    //BC3_SRGB_BLOCK = 138,
    //BC4_UNORM_BLOCK = 139,
    //BC4_SNORM_BLOCK = 140,
    //BC5_UNORM_BLOCK = 141,
    //BC5_SNORM_BLOCK = 142,
    //BC6H_UFLOAT_BLOCK = 143,
    //BC6H_SFLOAT_BLOCK = 144,
    //BC7_UNORM_BLOCK = 145,
    //BC7_SRGB_BLOCK = 146,
    //ETC2_R8G8B8_UNORM_BLOCK = 147,
    //ETC2_R8G8B8_SRGB_BLOCK = 148,
    //ETC2_R8G8B8A1_UNORM_BLOCK = 149,
    //ETC2_R8G8B8A1_SRGB_BLOCK = 150,
    //ETC2_R8G8B8A8_UNORM_BLOCK = 151,
    //ETC2_R8G8B8A8_SRGB_BLOCK = 152,
    //EAC_R11_UNORM_BLOCK = 153,
    //EAC_R11_SNORM_BLOCK = 154,
    //EAC_R11G11_UNORM_BLOCK = 155,
    //EAC_R11G11_SNORM_BLOCK = 156,
    //ASTC_4x4_UNORM_BLOCK = 157,
    //ASTC_4x4_SRGB_BLOCK = 158,
    //ASTC_5x4_UNORM_BLOCK = 159,
    //ASTC_5x4_SRGB_BLOCK = 160,
    //ASTC_5x5_UNORM_BLOCK = 161,
    //ASTC_5x5_SRGB_BLOCK = 162,
    //ASTC_6x5_UNORM_BLOCK = 163,
    //ASTC_6x5_SRGB_BLOCK = 164,
    //ASTC_6x6_UNORM_BLOCK = 165,
    //ASTC_6x6_SRGB_BLOCK = 166,
    //ASTC_8x5_UNORM_BLOCK = 167,
    //ASTC_8x5_SRGB_BLOCK = 168,
    //ASTC_8x6_UNORM_BLOCK = 169,
    //ASTC_8x6_SRGB_BLOCK = 170,
    //ASTC_8x8_UNORM_BLOCK = 171,
    //ASTC_8x8_SRGB_BLOCK = 172,
    //ASTC_10x5_UNORM_BLOCK = 173,
    //ASTC_10x5_SRGB_BLOCK = 174,
    //ASTC_10x6_UNORM_BLOCK = 175,
    //ASTC_10x6_SRGB_BLOCK = 176,
    //ASTC_10x8_UNORM_BLOCK = 177,
    //ASTC_10x8_SRGB_BLOCK = 178,
    //ASTC_10x10_UNORM_BLOCK = 179,
    //ASTC_10x10_SRGB_BLOCK = 180,
    //ASTC_12x10_UNORM_BLOCK = 181,
    //ASTC_12x10_SRGB_BLOCK = 182,
    //ASTC_12x12_UNORM_BLOCK = 183,
    //ASTC_12x12_SRGB_BLOCK = 184,
    //G8B8G8R8_422_UNORM = 1000156000,
    //B8G8R8G8_422_UNORM = 1000156001,
    //G8_B8_R8_3PLANE_420_UNORM = 1000156002,
    //G8_B8R8_2PLANE_420_UNORM = 1000156003,
    //G8_B8_R8_3PLANE_422_UNORM = 1000156004,
    //G8_B8R8_2PLANE_422_UNORM = 1000156005,
    //G8_B8_R8_3PLANE_444_UNORM = 1000156006,
    //R10X6_UNORM_PACK16 = 1000156007,
    //R10X6G10X6_UNORM_2PACK16 = 1000156008,
    //R10X6G10X6B10X6A10X6_UNORM_4PACK16 = 1000156009,
    //G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 = 1000156010,
    //B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 = 1000156011,
    //G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012,
    //G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 = 1000156013,
    //G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014,
    //G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 = 1000156015,
    //G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016,
    //R12X4_UNORM_PACK16 = 1000156017,
    //R12X4G12X4_UNORM_2PACK16 = 1000156018,
    //R12X4G12X4B12X4A12X4_UNORM_4PACK16 = 1000156019,
    //G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 = 1000156020,
    //B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 = 1000156021,
    //G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022,
    //G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 = 1000156023,
    //G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024,
    //G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 = 1000156025,
    //G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026,
    //G16B16G16R16_422_UNORM = 1000156027,
    //B16G16R16G16_422_UNORM = 1000156028,
    //G16_B16_R16_3PLANE_420_UNORM = 1000156029,
    //G16_B16R16_2PLANE_420_UNORM = 1000156030,
    //G16_B16_R16_3PLANE_422_UNORM = 1000156031,
    //G16_B16R16_2PLANE_422_UNORM = 1000156032,
    //G16_B16_R16_3PLANE_444_UNORM = 1000156033,
    //PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000,
    //PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001,
    //PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002,
    //PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003,
    //PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004,
    //PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005,
    //PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006,
    //PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007,
    //ASTC_4x4_SFLOAT_BLOCK_EXT = 1000066000,
    //ASTC_5x4_SFLOAT_BLOCK_EXT = 1000066001,
    //ASTC_5x5_SFLOAT_BLOCK_EXT = 1000066002,
    //ASTC_6x5_SFLOAT_BLOCK_EXT = 1000066003,
    //ASTC_6x6_SFLOAT_BLOCK_EXT = 1000066004,
    //ASTC_8x5_SFLOAT_BLOCK_EXT = 1000066005,
    //ASTC_8x6_SFLOAT_BLOCK_EXT = 1000066006,
    //ASTC_8x8_SFLOAT_BLOCK_EXT = 1000066007,
    //ASTC_10x5_SFLOAT_BLOCK_EXT = 1000066008,
    //ASTC_10x6_SFLOAT_BLOCK_EXT = 1000066009,
    //ASTC_10x8_SFLOAT_BLOCK_EXT = 1000066010,
    //ASTC_10x10_SFLOAT_BLOCK_EXT = 1000066011,
    //ASTC_12x10_SFLOAT_BLOCK_EXT = 1000066012,
    //ASTC_12x12_SFLOAT_BLOCK_EXT = 1000066013,
    //G8_B8R8_2PLANE_444_UNORM_EXT = 1000330000,
    //G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT = 1000330001,
    //G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT = 1000330002,
    //G16_B16R16_2PLANE_444_UNORM_EXT = 1000330003,
    //A4R4G4B4_UNORM_PACK16_EXT = 1000340000,
    //A4B4G4R4_UNORM_PACK16_EXT = 1000340001,
    //G8B8G8R8_422_UNORM_KHR = G8B8G8R8_422_UNORM,
    //B8G8R8G8_422_UNORM_KHR = B8G8R8G8_422_UNORM,
    //G8_B8_R8_3PLANE_420_UNORM_KHR = G8_B8_R8_3PLANE_420_UNORM,
    //G8_B8R8_2PLANE_420_UNORM_KHR = G8_B8R8_2PLANE_420_UNORM,
    //G8_B8_R8_3PLANE_422_UNORM_KHR = G8_B8_R8_3PLANE_422_UNORM,
    //G8_B8R8_2PLANE_422_UNORM_KHR = G8_B8R8_2PLANE_422_UNORM,
    //G8_B8_R8_3PLANE_444_UNORM_KHR = G8_B8_R8_3PLANE_444_UNORM,
    //R10X6_UNORM_PACK16_KHR = R10X6_UNORM_PACK16,
    //R10X6G10X6_UNORM_2PACK16_KHR = R10X6G10X6_UNORM_2PACK16,
    //R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR = R10X6G10X6B10X6A10X6_UNORM_4PACK16,
    //G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR = G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
    //B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR = B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
    //G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR = G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
    //G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR = G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
    //G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR = G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
    //G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR = G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
    //G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR = G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
    //R12X4_UNORM_PACK16_KHR = R12X4_UNORM_PACK16,
    //R12X4G12X4_UNORM_2PACK16_KHR = R12X4G12X4_UNORM_2PACK16,
    //R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR = R12X4G12X4B12X4A12X4_UNORM_4PACK16,
    //G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR = G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
    //B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR = B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
    //G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR = G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
    //G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR = G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
    //G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR = G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
    //G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR = G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
    //G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR = G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
    //G16B16G16R16_422_UNORM_KHR = G16B16G16R16_422_UNORM,
    //B16G16R16G16_422_UNORM_KHR = B16G16R16G16_422_UNORM,
    //G16_B16_R16_3PLANE_420_UNORM_KHR = G16_B16_R16_3PLANE_420_UNORM,
    //G16_B16R16_2PLANE_420_UNORM_KHR = G16_B16R16_2PLANE_420_UNORM,
    //G16_B16_R16_3PLANE_422_UNORM_KHR = G16_B16_R16_3PLANE_422_UNORM,
    //G16_B16R16_2PLANE_422_UNORM_KHR = G16_B16R16_2PLANE_422_UNORM,
    //G16_B16_R16_3PLANE_444_UNORM_KHR = G16_B16_R16_3PLANE_444_UNORM,
    MAX_ENUM = 0x7FFFFFFF
} ;

inline bool isDepth(FrameGraphFormat f)
{
    if( f == FrameGraphFormat::D32_SFLOAT ||
        f == FrameGraphFormat::D24_UNORM_S8_UINT ||
        f == FrameGraphFormat::D32_SFLOAT_S8_UINT)
    {
        return true;
    }
    return false;
}
struct RenderTargetDefinition
{
    std::string      name;
    FrameGraphFormat format;
    //uint32_t         width  = 0;
    //uint32_t         height = 0;
};

struct ImageDefinition
{
    std::string      name;
    FrameGraphFormat format;
    uint32_t         width     = 0;
    uint32_t         height    = 0;
    bool             resizable = true;
};

struct FrameBase
{
    // The dimensions of the framebuffer image
    uint32_t imageWidth       = 0;
    uint32_t imageHeight      = 0;

    // the renderable width/height
    // this value may be different from imageWidth and imageHeight
    // These values will be less than the values above when you
    // set the renderpass to only reallocate when the
    // image size increases
    uint32_t renderableWidth  = 0;
    uint32_t renderableHeight = 0;

    uint32_t windowWidth  = 0;
    uint32_t windowHeight = 0;

    bool     resizable        = false;
};

struct RenderPassNode
{
    std::string name;

    std::vector<RenderTargetDefinition> inputSampledRenderTargets;  // input render targets
    std::vector<RenderTargetDefinition> outputRenderTargets; // output render targets
    uint32_t                            width  = 0; // if zer0, use swapchain's size
    uint32_t                            height = 0;

    RenderPassNode& input(std::string name)
    {
        inputSampledRenderTargets.push_back({name});
        return *this;
    }
    RenderPassNode& output(std::string name, FrameGraphFormat format=FrameGraphFormat::UNDEFINED)
    {
        outputRenderTargets.push_back({name,format});
        return *this;
    }
    RenderPassNode& setExtent(uint32_t _width, uint32_t _height)
    {
        width = _width;
        height = _height;
        return *this;
    }
};



struct RenderTargetNode
{
    std::string              name;
    std::string              writer;  // only one render pass can write to a render target
    std::vector<std::string> readers; // multiple render passes can read from this render target

    RenderTargetDefinition   imageResource;
};

using node_v = std::variant<RenderPassNode,RenderTargetNode>;

struct FrameGraph
{

#define BE(A) std::begin(A),std::end(A)

    /**
     * @brief findExecutionOrder
     * @return
     *
     * Finds the execution order of the graph. This includes
     * ALL nodes. This means it will include the render pass nodes
     * and the render target nodes
     */
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

    /**
     * @brief createRenderPass
     * @param name
     * @return
     *
     * Create a renderPass and return the reference to it.
     */
    RenderPassNode& createRenderPass(std::string const & name)
    {
        RenderPassNode RPN;
        RPN.name = name;
        m_nodes[name] = RPN;
        return std::get<RenderPassNode>(m_nodes[name]);
    }

    /**
     * @brief finalize
     *
     * Call this function after the graph has been generated.
     * This will determine the order the render passes should
     * be executed in as well as determine how many images
     * should be created and which ones should be reused
     */
    void finalize()
    {
        for(auto it=m_nodes.begin();it!=m_nodes.end();)
        {
            if( std::holds_alternative<RenderTargetNode>(it->second) )
            {
                it = m_nodes.erase(it);
            }
            else
            {
                it++;
            }
        }
        generateImages();

        auto order = findExecutionOrder();

        spdlog::info("Execute Order: {}", fmt::join(BE(order),","));

        std::map<std::string, int32_t> imageUseCount;

        auto _print = [&]()
        {
            for(auto & [rtName, count] : imageUseCount)
            {
                spdlog::info("{} : {}   {}", rtName, count, std::get<RenderTargetNode>(m_nodes.at(rtName)).imageResource.name);
            }
            spdlog::info("----");

        };
        _print();


        for(auto & name : order)
        {
            auto & n = m_nodes.at(name);
            if( std::holds_alternative<RenderPassNode>(n) )
            {
                auto & N = std::get<RenderPassNode>(n);
                for(auto & n : N.outputRenderTargets)
                {
                    imageUseCount[n.name]++;
                }
                for(auto & n : N.inputSampledRenderTargets)
                {
                    imageUseCount[n.name]++;
                }
            }
        }

        for(auto & name : order)
        {
            auto & _n = m_nodes.at(name);
            if( !std::holds_alternative<RenderPassNode>(_n))
                continue;
            auto & N = std::get<RenderPassNode>(_n);

            spdlog::info("Pass Name: {}", name);

            for(auto & outTarget : N.outputRenderTargets)
            {
                auto & outRenderTarget = std::get<RenderTargetNode>(m_nodes.at(outTarget.name));
                auto imageThatIsNotBeingUsed = _findImageThatIsNotBeingUsed(imageUseCount, N, outTarget);

                if(imageThatIsNotBeingUsed.empty()) // no available image
                {
                    // generate new image
                    auto imageName = fmt::format("{}_img", outTarget.name);
                    outRenderTarget.imageResource.name   = imageName;
                    outRenderTarget.imageResource.format = outTarget.format;

                    ImageDefinition imgDef;
                    imgDef.name   = imageName;
                    imgDef.format = outTarget.format;
                    imgDef.width  = N.width;
                    imgDef.height = N.height;

                    m_images[imageName] = imgDef;
                }
                else
                {
                    outRenderTarget.imageResource.name = std::get<RenderTargetNode>(m_nodes.at(imageThatIsNotBeingUsed)).imageResource.name;
                    imageUseCount.at(imageThatIsNotBeingUsed)++;
                }
            }

            _print();

            for(auto & outTarget : N.outputRenderTargets)
            {
                imageUseCount.at(outTarget.name)--;
            }
            for(auto & inTarget : N.inputSampledRenderTargets)
            {
                imageUseCount.at(inTarget.name)--;
            }
        }
    }


    auto const & getImages() const
    {
        return m_images;
    }
    auto const & getNodes() const
    {
        return m_nodes;
    }
protected:

    // finds all nodes that do not have outputs
    std::vector<std::string> findEndNodes() const
    {
        // end node will always be a RenderTarget

        std::vector<std::string> endNodes;
        for(auto & [name, n] : m_nodes)
        {
            if(std::holds_alternative<RenderTargetNode>(n))
            {
                auto & N = std::get<RenderTargetNode>(n); // should always be a render pass node
                if(N.readers.size() == 0)
                    endNodes.push_back(name);
            }
            else
            {
                auto & N = std::get<RenderPassNode>(n); // should always be a render pass node
                if(N.outputRenderTargets.size() == 0)
                    endNodes.push_back(name);
            }


        }
        return endNodes;
    }


    void _recursePushBack(std::string const & name, std::vector<std::string> & order) const
    {
        auto & n = m_nodes.at(name);
        order.push_back(name);

        if(std::holds_alternative<RenderPassNode>(n))
        {
            auto & N = std::get<RenderPassNode>(n); // should always be a render pass node
            for(auto & in : N.inputSampledRenderTargets)
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
    std::string _findImageThatIsNotBeingUsed(std::map<std::string, int32_t> & renderTargetUsageCount,
                                            RenderPassNode const & node,
                                            RenderTargetDefinition const & def)
    {
        for(auto & [name, count] : renderTargetUsageCount)
        {
            if(count == 0) // image isn't being used
            {
                auto & N = std::get<RenderTargetNode>(m_nodes.at(name));

                auto & I = m_images.at(N.imageResource.name);
                if( std::tie(I.format,I.height,I.width) == std::tie(def.format,node.width,node.height) )
                {
                    return name;
                }
                //return name;
            }
        }
        return "";
    }

    // generate all the image nodes based on
    // the render target outputs.
    void generateImages()
    {
        std::map<std::string, RenderTargetNode> images;

        // first generate all the output images
        // go through each of the nodes and create the output
        // render target descriptions
        for(auto & [name,n] : m_nodes)
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

        // Tell each of new new images we created
        // which render pass is reading from it
        for(auto & n : m_nodes)
        {
            auto & N = std::get<RenderPassNode>(n.second);
            for(auto & in : N.inputSampledRenderTargets)
            {
                images.at(in.name).readers.push_back(n.first);
            }
        }

        // copy all the image nodes into
        // node list
        for(auto & [n,i] : images)
        {
            m_nodes[n] = i;
        }
    }

//public:

    std::map< std::string, ImageDefinition> m_images;
    std::map<std::string, node_v>           m_nodes;
#undef BE
};

#endif
