#pragma once

// Makes sure to remove constructors for structs.
#include "util/Logger.h"
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

class GPUDevice;

// Utility for initializing Vulkan objects easily.
class VkInitializer final {
public:
    static vk::Device CreateDevice(GPUDevice* gpu, vk::Queue& graphics, vk::Queue& present,
                                    vk::Queue& transfer, vk::CommandPool& graphics_pool, vk::CommandPool& transfer_pool);
private:
    static Logger sLogger;
};
