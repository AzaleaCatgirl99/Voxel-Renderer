#pragma once

// Makes sure to remove constructors for structs.
#include "util/Logger.h"
#include "util/display/vulkan/VkStructs.h"
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

class GPUDevice;

// Utility for initializing/creating Vulkan objects easily.
class VkInitializer final {
public:
    static vk::Device CreateDevice(GPUDevice* gpu, vk::Queue& graphics, vk::Queue& present,
                                    vk::Queue& transfer, vk::CommandPool& graphics_pool, vk::CommandPool& transfer_pool);
    static VkStructs::Pipeline CreatePipeline(vk::Device& device, vk::RenderPass& render_pass, VkStructs::Pipeline::Info& info);
    static vk::RenderPass CreateSimpleRenderPass(vk::Device& device);
private:
    static Logger sLogger;
};
