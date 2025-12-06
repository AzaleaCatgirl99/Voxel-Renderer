#include "util/display/image/GPUImageSampler.h"

#include "util/display/RenderSystem.h"
#include "util/display/vulkan/VkObjectMaps.h"
#include "util/display/vulkan/VkResultHandler.h"

void GPUImageSampler::Build() {
    VkSamplerCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VkObjectMaps::GetFilter(m_magFilter),
        .minFilter = VkObjectMaps::GetFilter(m_minFilter),
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VkObjectMaps::GetSamplerAddressMode(m_wrap),
        .addressModeV = VkObjectMaps::GetSamplerAddressMode(m_wrap),
        .addressModeW = VkObjectMaps::GetSamplerAddressMode(m_wrap),
        .mipLodBias = 0.0f,
        .anisotropyEnable = m_anisotropy > 0.0f ? VK_TRUE : VK_FALSE,
        .maxAnisotropy = m_anisotropy,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = static_cast<float>(m_mipmap)
    };

    VkResult result = vkCreateSampler(RenderSystem::sDevice, &info, VK_NULL_HANDLE, &m_handler);
    VkResultHandler::CheckResult(result, "Failed to create image sampler!");
}

void GPUImageSampler::Delete() {
    vkDestroySampler(RenderSystem::sDevice, m_handler, VK_NULL_HANDLE);
}
