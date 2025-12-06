#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

struct Descriptor {
    uint32_t binding = 0;
    VkShaderStageFlags stage = 0;
    VkDescriptorType type;
};
