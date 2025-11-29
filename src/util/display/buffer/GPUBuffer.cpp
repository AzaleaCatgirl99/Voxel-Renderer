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
        .memoryTypeIndex = RenderSystem::FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    result = vkAllocateMemory(RenderSystem::sDevice, &allocInfo, VK_NULL_HANDLE, &m_memory);
    VkResultHandler::CheckResult(result, "Failed to create buffer memory handler!");

    // Binds the buffer to the memory handler.
    vkBindBufferMemory(RenderSystem::sDevice, m_handler, m_memory, 0);
}

void GPUBuffer::Allocate(void* data, uint32_t size, uint32_t offset) {
    void* _data;
    vkMapMemory(RenderSystem::sDevice, m_memory, offset, size, 0, &_data);
    memcpy(_data, data, size);
    vkUnmapMemory(RenderSystem::sDevice, m_memory);
}

void GPUBuffer::CmdBind() {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(RenderSystem::sCommandBuffers[RenderSystem::sCurrentFrame], 0, 1, &m_handler, &offset);
}

void GPUBuffer::Delete() {
    vkDestroyBuffer(RenderSystem::sDevice, m_handler, VK_NULL_HANDLE);
    vkFreeMemory(RenderSystem::sDevice, m_memory, VK_NULL_HANDLE);
}
