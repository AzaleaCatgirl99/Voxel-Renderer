#pragma once

#include "util/display/buffer/GPUBuffer.h"
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
    static VkSharingMode GetSharingMode(eGPUBufferSharingMode mode);
};
