#pragma once

#include "renderer/detail/IRenderer.h"
#include <vulkan/vulkan.h>
#include <util/Logger.h>
#include <vector>

namespace detail {

// Renderer implementation that uses Vulkan.
class VkRenderer : public IRenderer {
public:
    constexpr VkRenderer(Window* window) noexcept : IRenderer(window) {
    }

    // Initializes the Vulkan renderer.
    virtual void Initialize() override;

    // Destroys and cleans up Vulkan allocations.
    virtual void Destroy() override;
private:
    // Determines whether to use validation layers depending on if NDEBUG (no debugging) flag is enabled.
    #ifdef NDEBUG
        static constexpr const bool sEnableValidationLayers = false;
    #else
        static constexpr const bool sEnableValidationLayers = true;
    #endif

    // Set what severities to enable in the debug messenger.
    static constexpr const uint32_t sEnabledSeverityFlags = 
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    // Set what validation layers to enable.
    static constexpr const char* sValidationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    // Static Logger object used for info and error logging.
    static Logger sLogger;

    // Creates the application info struct.
    static VkApplicationInfo CreateAppInfo();

    // Gets the required extensions for the Vulkan instance.
    static std::vector<const char*> GetRequiredExtensions();

    // Creates the instance info struct.
    static VkInstanceCreateInfo CreateInstanceInfo();

    // Checks if the validation layers are supported.
    static bool CheckValidationLayerSupport();

    // Creates the debug info struct.
    static VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerInfo();

    // Debug callback function used when validation layers log information.
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT pMessageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT pMessageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    // Creates the Vulkan instance.
    void CreateInstance();

    // Creates the debug messenger.
    void SetupDebugMessenger();

    // Loads and calls the extension function for creating the debug messenger.
    VkResult CreateDebugUtilsMessengerEXT(
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    // Loads and calls the extension function for destroying the debug messenger.
    void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator);

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
};

}
