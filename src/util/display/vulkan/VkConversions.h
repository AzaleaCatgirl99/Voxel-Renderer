#pragma once

#include "util/display/pipeline/Type.h"
#include "util/display/vulkan/VkStructs.h"
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

// Various different functions used to convert one type to a Vulkan type.
namespace VkConversions {

uint32_t GetIndexTypeSize(vk::IndexType type);
vk::PresentModeKHR GetPresentMode(VkStructs::SwapInterval interval);
vk::Format GetTypeFormat(DataType type);

}
