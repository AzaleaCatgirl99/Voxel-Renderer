#include "util/display/image/GPUImage.h"

#include "util/display/buffer/GPUBuffer.h"
#include "util/display/vulkan/VkObjectMaps.h"
#include "util/display/vulkan/VkResultHandler.h"
#include "util/display/RenderSystem.h"
#include "util/BufferedImage.h"

void GPUImage::Build() {
    VkImageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = 0,
        .imageType = VkObjectMaps::GetImageType(m_type),
        .format = VkObjectMaps::GetColorFormat(m_format),
        .extent = {
            .width = m_size.m_width,
            .height = m_size.m_height,
            .depth = m_size.m_depth,
        },
        .mipLevels = m_size.m_mipmapLevel,
        .arrayLayers = m_size.m_layers,
        .samples = VkObjectMaps::GetSampleCount(m_samples),
        .tiling = VkObjectMaps::GetImageTiling(m_tiling),
        .usage = m_usages,
        .sharingMode = VkObjectMaps::GetSharingMode(m_sharingMode),
        .initialLayout = VkObjectMaps::GetImageLayout(m_layout)
    };

    VkResult result = vkCreateImage(RenderSystem::sDevice, &info, VK_NULL_HANDLE, &m_handler);
    VkResultHandler::CheckResult(result, "Failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(RenderSystem::sDevice, m_handler, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = RenderSystem::GetGPU()->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    result = vkAllocateMemory(RenderSystem::sDevice, &allocInfo, VK_NULL_HANDLE, &m_memory);
    VkResultHandler::CheckResult(result, "Failed to allocate image memory!");

    vkBindImageMemory(RenderSystem::sDevice, m_handler, m_memory, 0);
}

void GPUImage::Allocate(const void* data) {
    void* _data;
    vkMapMemory(RenderSystem::sDevice, m_memory, 0, m_size.m_width * m_size.m_height * 4, 0, &_data);
    memcpy(_data, data, m_size.m_width * m_size.m_height * 4);
    vkUnmapMemory(RenderSystem::sDevice, m_memory);
}

void GPUImage::Allocate(BufferedImage& image) {
    Allocate(image.GetPixels());
}

void GPUImage::CopyBuffer(GPUBuffer& buffer) {
    VkCommandBuffer cmdBuffer = RenderSystem::BeginDataTransfer();

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_handler,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = m_size.m_mipmapLevel,
            .baseArrayLayer = 0,
            .layerCount = m_size.m_layers,
        }
    };

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = m_size.m_mipmapLevel,
            .baseArrayLayer = 0,
            .layerCount = m_size.m_layers
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            m_size.m_width,
            m_size.m_height,
            m_size.m_depth
        }
    };

    vkCmdCopyBufferToImage(cmdBuffer, buffer.m_handler, m_handler, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    RenderSystem::EndDataTransfer(cmdBuffer);
}

void GPUImage::Delete() {
    vkDestroyImage(RenderSystem::sDevice, m_handler, VK_NULL_HANDLE);
    vkFreeMemory(RenderSystem::sDevice, m_memory, VK_NULL_HANDLE);
}
