#pragma once

#include "renderer/detail/IRenderer.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
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
    static constexpr const uint32_t sEnabledSeverityFlags = 0 |
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
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
#endif
    0;

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

    // Interprets a VkResult error.
    static std::runtime_error InterpretVkError(VkResult result, const char* error);

    // Debug callback function used when validation layers log information.
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);

    // Gets the score of the given physical device.
    static size_t GetPhysicalDeviceScore(VkPhysicalDevice device);

    // Creates the Vulkan instance.
    void CreateInstance();

    // Creates the debug messenger.
    void SetupDebugMessenger();

    // Selects the best physical device (AKA GPU) that can use be used in renderer.
    void SelectBestPhysicalDevice();

    // Checks whether a specific physical device is usable for the renderer.
    bool IsPhysicalDeviceUsable(VkPhysicalDevice device);

    // Struct for storing queue family indices
    struct QueueFamilyIndices {
        std::optional<uint32_t> m_graphics;
        std::optional<uint32_t> m_presentation;

        constexpr bool HasEverything() const noexcept {
            return m_graphics.has_value() && m_presentation.has_value();
        }
    };

    // Gets the queue families for a specific physical device.
    QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);

    // Creates the logical device and assigns the graphics queue handle.
    void CreateLogicalDevice();

    // Loads and calls the extension function for creating the debug messenger.
    VkResult CreateDebugUtilsMessengerEXT(
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        const VkAllocationCallbacks* allocator,
        VkDebugUtilsMessengerEXT* debug_messenger);

    // Loads and calls the extension function for destroying the debug messenger.
    void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* allocator);

    // Creates the window surface needed for the renderer.
    void CreateSurface();

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_logicalDevice = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentationQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

}
