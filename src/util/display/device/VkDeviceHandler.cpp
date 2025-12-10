#include "util/display/device/VkDeviceHandler.h"

#include "util/Logger.h"
#ifdef VXL_RENDERSYSTEM_DEBUG
#include "util/display/RenderSystem.h"
#endif
#include "util/display/device/SwapchainHandler.h"
#include "util/display/device/GPUDevice.h"
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_platform_defines.h>
#include <cstddef>
#include <cstdint>

Logger VkDeviceHandler::sLogger = Logger("RenderDevice");

void VkDeviceHandler::Build(vk::Instance* instance, GPUDevice* gpu) {
    if (!gpu->GetQueueFamilies()->graphics.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No graphics family found.");

    if (!gpu->GetQueueFamilies()->present.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No presentation family found.");

    if (!gpu->GetQueueFamilies()->transfer.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No transfer family found.");

    vk::DeviceQueueCreateInfo queueCreateInfos[gpu->GetQueueFamilies()->UniqueSize()];
    float queuePriority = 1.0f;

    for (uint32_t i = 0; i < gpu->GetQueueFamilies()->UniqueSize(); i++) {
        queueCreateInfos[i] = {
            .queueFamilyIndex = gpu->GetQueueFamilies()->GetUnique(i),
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };
    }

    vk::PhysicalDeviceFeatures features = gpu->GetFeatures();

    vk::DeviceCreateInfo info = {
        .queueCreateInfoCount = static_cast<uint32_t>(gpu->GetQueueFamilies()->UniqueSize()),
        .pQueueCreateInfos = queueCreateInfos,
#ifdef VXL_RENDERSYSTEM_DEBUG
        .enabledLayerCount = RenderSystem::LAYER_COUNT,
        .ppEnabledLayerNames = RenderSystem::LAYERS,
#endif
        .enabledExtensionCount = GPUDevice::EXTENSION_COUNT,
        .ppEnabledExtensionNames = GPUDevice::EXTENSIONS.data(),
        .pEnabledFeatures = &features
    };

    // TODO figure out how to clean up this code.
    size_t queueIndicesSize = gpu->GetQueueFamilies()->UniqueSize();


    device = gpu->device.createDevice(info);

    graphicsQueue = device.getQueue(gpu->GetQueueFamilies()->graphics.value(), 0);
    presentQueue = device.getQueue(gpu->GetQueueFamilies()->present.value(), queueIndicesSize < 2 ? 0 : 1);

    uint32_t transferIndex = queueIndicesSize < 2 ? 0 : queueIndicesSize < 3 ? 1 : queueIndicesSize - 1;
    transferQueue = device.getQueue(gpu->GetQueueFamilies()->transfer.value(), transferIndex);

    vk::CommandPoolCreateInfo cmdPoolInfo = {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = gpu->GetQueueFamilies()->graphics.value()
    };

    graphicsCmdPool = device.createCommandPool(cmdPoolInfo);

    cmdPoolInfo.queueFamilyIndex = gpu->GetQueueFamilies()->transfer.value();
    transferCmdPool = device.createCommandPool(cmdPoolInfo);
}
