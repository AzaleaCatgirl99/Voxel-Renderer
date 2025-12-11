#pragma once

// This class only exists when debugging is enabled.
#ifdef VXL_DEBUG

#include "util/Logger.h"
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

// Utility for handling the debugging of the Vulkan API.
class VkDebugger final {
public:
    static VXL_INLINE uint32_t LAYER_COUNT = 1;
    static VXL_INLINE const char* LAYERS[LAYER_COUNT] = {
        "VK_LAYER_KHRONOS_validation"
    };
    static PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
    static PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

    static void Initialize(vk::InstanceCreateInfo& info);

    static void CreateMessenger(vk::Instance& instance);

    static void Destroy(vk::Instance& instance);

    static bool CheckValidationLayerSupport();
private:
    static vk::DebugUtilsMessengerEXT sMessenger;
    static vk::DebugUtilsMessengerCreateInfoEXT sCreateInfo;
    static Logger sLogger;

    // Used for adding messenger printing to Vulkan.
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL Callback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
        vk::DebugUtilsMessageTypeFlagsEXT message_type,
        const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);
};

#endif
