#pragma once

#include "util/display/pipeline/Type.h"
#include "util/Constants.h"
#include <vulkan/vulkan.h>

// Functions for getting various Vulkan objects from render enums.
class VkObjectMaps final {
public:
    static VkPresentModeKHR GetPresentMode(eRenderSwapInterval interval);
    static VkBlendFactor GetBlendFactor(eRenderBlendFactor factor);
    static VkPolygonMode GetPolygonMode(eRenderPolygonMode mode);
    static VkCullModeFlags GetCullMode(eRenderCullMode mode);
    static VkFormat GetTypeFormat(eRenderType type);
    static VkSharingMode GetSharingMode(eRenderSharingMode mode);
    static VkIndexType GetIndexType(eRenderType type);
    static VkImageType GetImageType(eRenderImageType type);
    static VkImageViewType GetImageViewType(eRenderImageType type);
    static VkFormat GetColorFormat(eRenderColorFormat format);
    static VkImageUsageFlags GetImageUsage(eRenderImageUsage usage);
    static VkImageLayout GetImageLayout(eRenderImageLayout layout);
    static VkImageTiling GetImageTiling(eRenderImageTiling tiling);
    static VkSampleCountFlagBits GetSampleCount(eRenderSampleCount count);
    static VkFilter GetFilter(eRenderFilter filter);
    static VkSamplerAddressMode GetSamplerAddressMode(eRenderWrapMode mode);
};
