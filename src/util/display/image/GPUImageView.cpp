#include "util/display/image/GPUImageView.h"

#include "util/display/RenderSystem.h"
#include "util/display/image/GPUImage.h"
#include "util/display/vulkan/VkObjectMaps.h"
#include "util/display/vulkan/VkResultHandler.h"

void GPUImageView::Build() {
    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_image->m_handler,
        .viewType = VkObjectMaps::GetImageViewType(m_image->m_type),
        .format = VkObjectMaps::GetColorFormat(m_image->m_format),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, // TODO change this
            .baseMipLevel = 0,
            .levelCount = m_image->m_size.m_mipmapLevel,
            .baseArrayLayer = 0,
            .layerCount = m_image->m_size.m_layers,
        }
    };

    VkResult result = vkCreateImageView(RenderSystem::sDevice, &info, VK_NULL_HANDLE, &m_handler);
    VkResultHandler::CheckResult(result, "Failed to create image view!");
}

void GPUImageView::Delete() {
    vkDestroyImageView(RenderSystem::sDevice, m_handler, VK_NULL_HANDLE);
}
