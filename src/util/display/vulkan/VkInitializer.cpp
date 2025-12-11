#include "util/display/vulkan/VkInitializer.h"

#include "util/display/device/GPUDevice.h"

Logger VkInitializer::sLogger = Logger("VkInitializer");

vk::Device VkInitializer::CreateDevice(GPUDevice* gpu, vk::Queue& graphics, vk::Queue& present,
    vk::Queue& transfer, vk::CommandPool& graphics_pool, vk::CommandPool& transfer_pool) {
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
        .enabledExtensionCount = GPUDevice::EXTENSION_COUNT,
        .ppEnabledExtensionNames = GPUDevice::EXTENSIONS.data(),
        .pEnabledFeatures = &features
    };

    // TODO figure out how to clean up this code.
    size_t queueIndicesSize = gpu->GetQueueFamilies()->UniqueSize();

    vk::Device device = gpu->device.createDevice(info);

    graphics = device.getQueue(gpu->GetQueueFamilies()->graphics.value(), 0);
    present = device.getQueue(gpu->GetQueueFamilies()->present.value(), queueIndicesSize < 2 ? 0 : 1);

    uint32_t transferIndex = queueIndicesSize < 2 ? 0 : queueIndicesSize < 3 ? 1 : queueIndicesSize - 1;
    transfer = device.getQueue(gpu->GetQueueFamilies()->transfer.value(), transferIndex);

    vk::CommandPoolCreateInfo cmdPoolInfo = {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = gpu->GetQueueFamilies()->graphics.value()
    };

    graphics_pool = device.createCommandPool(cmdPoolInfo);

    cmdPoolInfo.queueFamilyIndex = gpu->GetQueueFamilies()->transfer.value();
    transfer_pool = device.createCommandPool(cmdPoolInfo);

    return device;
}
