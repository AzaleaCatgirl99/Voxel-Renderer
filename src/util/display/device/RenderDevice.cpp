#include "util/display/device/RenderDevice.h"

#include "util/Logger.h"
#include "util/display/RenderSystem.h"
#include "util/display/vulkan/VkResultHandler.h"
#include "util/display/device/GPUDevice.h"
#include <SDL3/SDL_vulkan.h>

Logger RenderDevice::sLogger = Logger("RenderDevice");

void RenderDevice::Build() {
    if (!m_gpu->GetQueueFamilies().m_graphics.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No graphics family found.");

    if (!m_gpu->GetQueueFamilies().m_present.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No presentation family found.");

    if (!m_gpu->GetQueueFamilies().m_transfer.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No transfer family found.");

    // Gets the unique queue families that exist.
    std::set<uint32_t> uniqueQueueFamilies = {m_gpu->GetQueueFamilies().m_graphics.value(),
                                            m_gpu->GetQueueFamilies().m_present.value(),
                                            m_gpu->GetQueueFamilies().m_transfer.value()};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        });
    }

    VkPhysicalDeviceFeatures features = m_gpu->GetFeatures();
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(), // Links the queues.
#ifdef SDL_PLATFORM_MACOS
        .enabledExtensionCount = 2,
#else
        .enabledExtensionCount = 1,
#endif
        .ppEnabledExtensionNames = GPUDevice::EXTENSIONS.data(),
        .pEnabledFeatures = &features // Links the device features.
    };

    if (RenderSystem::ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = RenderSystem::LAYERS;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(*m_gpu, &createInfo, VK_NULL_HANDLE, &m_context);
    VkResultHandler::CheckResult(result, "Failed to create context!", "Created context.");

    vkGetDeviceQueue(m_context, m_gpu->GetQueueFamilies().m_graphics.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_context, m_gpu->GetQueueFamilies().m_present.value(), uniqueQueueFamilies.size() < 2 ? 0 : 1, &m_presentQueue);

    uint32_t transferIndex = uniqueQueueFamilies.size() < 2 ? 0 : uniqueQueueFamilies.size() < 3 ? 1 : uniqueQueueFamilies.size() - 1;
    vkGetDeviceQueue(m_context, m_gpu->GetQueueFamilies().m_transfer.value(), transferIndex, &m_transferQueue);

    m_graphicsCmdPool = CreateCmdPool(m_gpu->GetQueueFamilies().m_graphics.value());
    m_transferCmdPool = CreateCmdPool(m_gpu->GetQueueFamilies().m_transfer.value());
}

VkCommandPool RenderDevice::CreateCmdPool(uint32_t family_index) {
    VkCommandPool pool = VK_NULL_HANDLE;

    VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = family_index
    };

    VkResult result = vkCreateCommandPool(m_context, &info, VK_NULL_HANDLE, &pool);
    VkResultHandler::CheckResult(result, "Failed to create command pool!");

    return pool;
}

VkCommandBuffer RenderDevice::CreateCmdBuffer(std::optional<VkCommandPool> pool) {
    VkCommandBuffer buffer = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool.has_value() ? pool.value() : m_graphicsCmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkResult result = vkAllocateCommandBuffers(m_context, &info, &buffer);
    VkResultHandler::CheckResult(result, "Failed to create command buffer!");

    return buffer;
}

void RenderDevice::CreateCmdBuffers(VkCommandBuffer* buffers, uint32_t count, std::optional<VkCommandPool> pool) {
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool.has_value() ? pool.value() : m_graphicsCmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count
    };

    VkResult result = vkAllocateCommandBuffers(m_context, &info, buffers);
    VkResultHandler::CheckResult(result, "Failed to create command buffers!");
}
