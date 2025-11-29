#include "renderer/detail/vulkan/VkObjectMaps.h"

VkPresentModeKHR VkObjectMaps::GetPresentMode(eRenderSwapInterval interval) {
    switch (interval) {
    case RENDER_SWAP_INTERVAL_IMMEDIATE:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    case RENDER_SWAP_INTERVAL_VSYNC:
        return VK_PRESENT_MODE_FIFO_KHR;
    case RENDER_SWAP_INTERVAL_TRIPLE_BUFFERING:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    case RENDER_SWAP_INTERVAL_ADAPTIVE_SYNC: // This does not exist on Vulkan.
        return VK_PRESENT_MODE_MAX_ENUM_KHR;
    }
}

VkBlendFactor VkObjectMaps::GetBlendFactor(eRenderBlendFactor factor) {
    switch (factor) {
    case RENDER_BLEND_FACTOR_ZERO:
        return VK_BLEND_FACTOR_ZERO;
    case RENDER_BLEND_FACTOR_ONE:
        return VK_BLEND_FACTOR_ONE;
    case RENDER_BLEND_FACTOR_SRC_COLOR:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case RENDER_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case RENDER_BLEND_FACTOR_DST_COLOR:
        return VK_BLEND_FACTOR_DST_COLOR;
    case RENDER_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case RENDER_BLEND_FACTOR_SRC_ALPHA:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case RENDER_BLEND_FACTOR_DST_ALPHA:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case RENDER_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case RENDER_BLEND_FACTOR_CONST_COLOR:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case RENDER_BLEND_FACTOR_ONE_MINUS_CONST_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case RENDER_BLEND_FACTOR_CONST_ALPHA:
        return VK_BLEND_FACTOR_CONSTANT_ALPHA;
    case RENDER_BLEND_FACTOR_ONE_MINUS_CONST_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    case RENDER_BLEND_FACTOR_SRC_ALPHA_SATURATE:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case RENDER_BLEND_FACTOR_SRC1_COLOR:
        return VK_BLEND_FACTOR_SRC1_COLOR;
    case RENDER_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
    case RENDER_BLEND_FACTOR_SRC1_ALPHA:
        return VK_BLEND_FACTOR_SRC1_ALPHA;
    case RENDER_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    }
}

VkPolygonMode VkObjectMaps::GetPolygonMode(eRenderPolygonMode mode) {
    switch (mode) {
    case RENDER_POLYGON_MODE_FILL:
        return VK_POLYGON_MODE_FILL;
    case RENDER_POLYGON_MODE_LINE:
        return VK_POLYGON_MODE_LINE;
    case RENDER_POLYGON_MODE_POINT:
        return VK_POLYGON_MODE_POINT;
    }
}

VkCullModeFlags VkObjectMaps::GetCullMode(eRenderCullMode mode) {
    switch (mode) {
    case RENDER_CULL_MODE_NONE:
        return VK_CULL_MODE_NONE;
    case RENDER_CULL_MODE_FRONT:
        return VK_CULL_MODE_FRONT_BIT;
    case RENDER_CULL_MODE_BACK:
        return VK_CULL_MODE_BACK_BIT;
    case RENDER_CULL_MODE_FRONT_AND_BACK:
        return VK_CULL_MODE_FRONT_AND_BACK;
    }
}

VkFormat VkObjectMaps::GetTypeFormat(eRenderType type) {
    switch (type) {
    case RENDER_TYPE_VEC2:
        return VK_FORMAT_R32G32_SFLOAT;
    case RENDER_TYPE_VEC3:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case RENDER_TYPE_VEC4:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case RENDER_TYPE_MAT2:
        return VK_FORMAT_UNDEFINED; // TODO figure out this one
    case RENDER_TYPE_MAT3:
        return VK_FORMAT_UNDEFINED; // TODO figure out this one
    case RENDER_TYPE_MAT4:
        return VK_FORMAT_UNDEFINED; // TODO figure out this one
    case RENDER_TYPE_FLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case RENDER_TYPE_DOUBLE:
        return VK_FORMAT_R64_SFLOAT;
    case RENDER_TYPE_INT:
        return VK_FORMAT_R32_SINT;
    case RENDER_TYPE_UINT:
        return VK_FORMAT_R32_UINT;
    }
}

VkSharingMode VkObjectMaps::GetSharingMode(eGPUBufferSharingMode mode) {
    switch (mode) {
    case GPU_BUFFER_SHARING_MODE_EXCLUSIVE:
        return VK_SHARING_MODE_EXCLUSIVE;
    case GPU_BUFFER_SHARING_MODE_CONCURRENT:
        return VK_SHARING_MODE_CONCURRENT;
    }
}
