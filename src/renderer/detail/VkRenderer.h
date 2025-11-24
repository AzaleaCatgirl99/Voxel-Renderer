#pragma once

#include "renderer/detail/IRenderer.h"
#include <vulkan/vulkan.h>
#include "util/Features.h"
#include "util/Logger.h"
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

    // Updates the Vulkan surface.
    virtual void UpdateDisplay() override;
private:
    // Determines whether to use validation layers depending on if NDEBUG (no debugging) flag is enabled.
#ifdef VXL_DEBUG
        static constexpr const bool sEnableValidationLayers = true;
#else
        static constexpr const bool sEnableValidationLayers = false;
#endif

    // Set what severities to enable in the debug messenger.
    static constexpr const uint32_t sEnabledSeverityFlags =
#if VXL_RENDERER_VERBOSE_LOG == VXL_TRUE
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
#endif
#if VXL_RENDERER_INFO_LOG == VXL_TRUE
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
#endif
#if VXL_RENDERER_WARNING_LOG == VXL_TRUE
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
#endif
#if VXL_RENDERER_ERROR_LOG == VXL_TRUE
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
#endif

    // Sets what layers to enable.
    static std::vector<const char*> sLayers;

    // The required extensions needed for the renderer.
    static std::vector<const char*> sRequiredExtensions;

    // Static Logger object used for info and error logging.
    static Logger sLogger;

    // The application info used by the renderer.
    static VkApplicationInfo sAppInfo;

    // The renderer's instance create info.
    static VkInstanceCreateInfo sCreateInfo;

    // The info used by the renderer's debug messenger.
    static VkDebugUtilsMessengerCreateInfoEXT sDebugMessengerInfo;

    // Gets the required extensions for the Vulkan instance.
    static void SetupRequiredExtensions();

    // sets up the instance info struct.
    static void SetupInstanceInfo();

    // Checks if the validation layers are supported.
    static bool CheckValidationLayerSupport();

    // Debug callback function used when validation layers log information.
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);

    // Creates the Vulkan instance.
    void CreateInstance();

    // Creates the debug messenger.
    void SetupDebugMessenger();

    // Loads and calls the extension function for creating the debug messenger.
    VkResult CreateDebugUtilsMessengerEXT(
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        const VkAllocationCallbacks* allocator,
        VkDebugUtilsMessengerEXT* debug_messenger);

    // Loads and calls the extension function for destroying the debug messenger.
    void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* allocator);

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
};

}
