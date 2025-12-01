#include "util/display/buffer/GPUBuffer.h"

#include "util/display/RenderSystem.h"
#include "util/display/vulkan/VkResultHandler.h"
#include "util/display/vulkan/VkObjectMaps.h"

void GPUBuffer::Build() {
    VkBufferCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = m_flags,
        .size = m_size,
        .usage = m_usages,
        .sharingMode = VkObjectMaps::GetSharingMode(m_sharingMode)
    };

    VkResult result = vkCreateBuffer(RenderSystem::sDevice, &createInfo, VK_NULL_HANDLE, &m_handler);
    VkResultHandler::CheckResult(result, "Failed to create buffer!");

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(RenderSystem::sDevice, m_handler, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = RenderSystem::FindMemoryType(memoryRequirements.memoryTypeBits, m_memProperties)
    };

    result = vkAllocateMemory(RenderSystem::sDevice, &allocInfo, VK_NULL_HANDLE, &m_memory);
    VkResultHandler::CheckResult(result, "Failed to create buffer memory handler!");

    // Binds the buffer to the memory handler.
    vkBindBufferMemory(RenderSystem::sDevice, m_handler, m_memory, 0);
}

void GPUBuffer::Allocate(const void* data, uint32_t size, uint32_t offset) {
    void* _data;
    vkMapMemory(RenderSystem::sDevice, m_memory, offset, size, 0, &_data);
    memcpy(_data, data, size);
    vkUnmapMemory(RenderSystem::sDevice, m_memory);
}

void GPUBuffer::MapData(void** data, uint32_t size, uint32_t offset) {
    vkMapMemory(RenderSystem::sDevice, m_memory, offset, size, 0, data);
}

void GPUBuffer::Copy(GPUBuffer& src, uint32_t size, uint32_t src_offset, uint32_t dst_offset) {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = RenderSystem::sTransferCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    // Create temporary command buffer to copy the data.
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(RenderSystem::sDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Copies over the data.
    VkBufferCopy copyRegion = {
        .srcOffset = src_offset,
        .dstOffset = dst_offset,
        .size = size
    };
    vkCmdCopyBuffer(commandBuffer, src.m_handler, m_handler, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };
    vkQueueSubmit(RenderSystem::sTransferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(RenderSystem::sTransferQueue);

    vkFreeCommandBuffers(RenderSystem::sDevice, RenderSystem::sTransferCommandPool, 1, &commandBuffer);
}

void GPUBuffer::Delete() {
    vkDestroyBuffer(RenderSystem::sDevice, m_handler, VK_NULL_HANDLE);
    vkFreeMemory(RenderSystem::sDevice, m_memory, VK_NULL_HANDLE);
}
