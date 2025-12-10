#pragma once

#include "util/Logger.h"
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_shared.hpp>

class GPUDevice;
class SwapChain;

// Class for handling specific aspects of the Vulkan device.
class VkDeviceHandler final {
public:
    void Build(vk::Instance* instance, GPUDevice* gpu);

    VXL_INLINE void Delete() {
        device.destroyCommandPool(graphicsCmdPool);
        device.destroyCommandPool(transferCmdPool);

        device.destroy();
    }

    VXL_INLINE operator vk::Device&() noexcept {
        return device;
    }

    vk::Device device;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
    vk::Queue transferQueue;
    vk::CommandPool graphicsCmdPool;
    vk::CommandPool transferCmdPool;
private:
    static Logger sLogger;
};
