#include "util/display/vulkan/VkObjectMaps.h"

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
    case RENDER_TYPE_INT16_T:
    case RENDER_TYPE_UINT16_T:
        return VK_FORMAT_UNDEFINED;
    }
}

VkSharingMode VkObjectMaps::GetSharingMode(eRenderSharingMode mode) {
    switch (mode) {
    case RENDER_SHARING_MODE_EXCLUSIVE:
        return VK_SHARING_MODE_EXCLUSIVE;
    case RENDER_SHARING_MODE_CONCURRENT:
        return VK_SHARING_MODE_CONCURRENT;
    }
}

VkIndexType VkObjectMaps::GetIndexType(eRenderType type) {
    switch (type) {
    case RENDER_TYPE_VEC2:
    case RENDER_TYPE_VEC3:
    case RENDER_TYPE_VEC4:
    case RENDER_TYPE_MAT2:
    case RENDER_TYPE_MAT3:
    case RENDER_TYPE_MAT4:
    case RENDER_TYPE_FLOAT:
    case RENDER_TYPE_DOUBLE:
    case RENDER_TYPE_INT:
    case RENDER_TYPE_UINT:
    case RENDER_TYPE_INT16_T:
        return VK_INDEX_TYPE_NONE_KHR;
    case RENDER_TYPE_UINT16_T:
        return VK_INDEX_TYPE_UINT16;
    }
}

VkImageType VkObjectMaps::GetImageType(eRenderImageType type) {
    switch (type) {
    case RENDER_IMAGE_TYPE_1D:
        return VK_IMAGE_TYPE_1D;
    case RENDER_IMAGE_TYPE_2D:
        return VK_IMAGE_TYPE_2D;
    case RENDER_IMAGE_TYPE_3D:
        return VK_IMAGE_TYPE_3D;
    }
}

VkImageViewType VkObjectMaps::GetImageViewType(eRenderImageType type) {
    switch (type) {
    case RENDER_IMAGE_TYPE_1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case RENDER_IMAGE_TYPE_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case RENDER_IMAGE_TYPE_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    }
}

VkFormat VkObjectMaps::GetColorFormat(eRenderColorFormat format) {
    switch (format) {
    case RENDER_COLOR_FORMAT_RGBA:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case RENDER_COLOR_FORMAT_RGB:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case RENDER_COLOR_FORMAT_ARGB:
        return VK_FORMAT_UNDEFINED; // TODO figure this out.
    }
}

VkImageUsageFlags VkObjectMaps::GetImageUsage(eRenderImageUsage usage) {
    switch (usage) {
    case RENDER_IMAGE_USAGE_SAMPLED:
        return VK_IMAGE_USAGE_SAMPLED_BIT;
    case RENDER_IMAGE_USAGE_STORAGE:
        return VK_IMAGE_USAGE_STORAGE_BIT;
    case RENDER_IMAGE_USAGE_COLOR_ATTACHMENT:
        return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    case RENDER_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT:
        return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    case RENDER_IMAGE_USAGE_INPUT_ATTACHMENT:
        return VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    case RENDER_IMAGE_USAGE_TRANSIENT_ATTACHMENT:
        return VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    case RENDER_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT:
        return VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
    case RENDER_IMAGE_USAGE_FRAGMENT_DENSITY_MAP:
        return VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
    case RENDER_IMAGE_USAGE_VIDEO_DECODE_DST:
        return VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
    case RENDER_IMAGE_USAGE_VIDEO_DECODE_DPB:
        return VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
    case RENDER_IMAGE_USAGE_VIDEO_ENCODE_SRC:
        return VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR;
    case RENDER_IMAGE_USAGE_VIDEO_ENCODE_DPB:
        return VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR;
    }
}

VkImageLayout VkObjectMaps::GetImageLayout(eRenderImageLayout layout) {
    switch (layout) {
    case RENDER_IMAGE_LAYOUT_UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case RENDER_IMAGE_LAYOUT_GENERAL:
        return VK_IMAGE_LAYOUT_GENERAL;
    case RENDER_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RENDER_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    case RENDER_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case RENDER_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case RENDER_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case RENDER_IMAGE_LAYOUT_PREINITIALIZED:
        return VK_IMAGE_LAYOUT_PREINITIALIZED;
    }
}

VkImageTiling VkObjectMaps::GetImageTiling(eRenderImageTiling tiling) {
    switch (tiling) {
    case RENDER_IMAGE_TILING_OPTIMAL:
        return VK_IMAGE_TILING_OPTIMAL;
    case RENDER_IMAGE_TILING_LINEAR:
        return VK_IMAGE_TILING_LINEAR;
    }
}

VkSampleCountFlagBits VkObjectMaps::GetSampleCount(eRenderSampleCount count) {
    switch (count) {
    case RENDER_SAMPLE_COUNT_1:
        return VK_SAMPLE_COUNT_1_BIT;
    case RENDER_SAMPLE_COUNT_2:
        return VK_SAMPLE_COUNT_2_BIT;
    case RENDER_SAMPLE_COUNT_4:
        return VK_SAMPLE_COUNT_4_BIT;
    case RENDER_SAMPLE_COUNT_8:
        return VK_SAMPLE_COUNT_8_BIT;
    case RENDER_SAMPLE_COUNT_16:
        return VK_SAMPLE_COUNT_16_BIT;
    case RENDER_SAMPLE_COUNT_32:
        return VK_SAMPLE_COUNT_32_BIT;
    case RENDER_SAMPLE_COUNT_64:
        return VK_SAMPLE_COUNT_64_BIT;
    }
}

VkFilter VkObjectMaps::GetFilter(eRenderFilter filter) {
    switch (filter) {
    case RENDER_FILTER_LINEAR:
        return VK_FILTER_LINEAR;
    case RENDER_FILTER_NEAREST:
        return VK_FILTER_NEAREST;
    }
}

VkSamplerAddressMode VkObjectMaps::GetSamplerAddressMode(eRenderWrapMode mode) {
    switch (mode) {
    case RENDER_WRAP_MODE_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case RENDER_WRAP_MODE_MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case RENDER_WRAP_MODE_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case RENDER_WRAP_MODE_MIRROR_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    case RENDER_WRAP_MODE_CLAMP_TO_BORDER:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    }
}
