#pragma once

#include "renderer/detail/IRenderer.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "util/Constants.h"
#include "util/Features.h"
#include "util/Logger.h"
#include <vector>

namespace detail {

// Renderer implementation that uses Vulkan.
class VkRenderer : public IRenderer {
public:
    constexpr VkRenderer(Window* window, const Properties& properties) noexcept : IRenderer(window, properties) {
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
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
#endif
    0;

    // Sets what layers to enable.
    static std::vector<const char*> sLayers;

    // The required instance extensions needed for the Vulkan renderer.
    static std::vector<const char*> sInstanceExtensions;

    // The required device extensions needed for the Vulkan renderer.
    static std::vector<const char*> sDeviceExtensions;

    // Map for getting the Vulkan present mode from the render swap interval.
    static const std::unordered_map<eRenderSwapInterval, VkPresentModeKHR> sPresentModes;

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
    void CreateDebugMessenger();

    // Creates the swap chain.
    void CreateSwapChain();

    // Selects the best physical device (AKA GPU) that can use be used in renderer.
    void SelectBestPhysicalDevice();

    // Checks whether a specific physical device is usable for the renderer.
    bool IsPhysicalDeviceUsable(VkPhysicalDevice device);

    // Checks if a physical device supports all of the required extensions.
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

    // Chooses a swap surface format to use.
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    // Chooses a swap chain present mode to use.
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    // Chooses the swap extent to use and sets the height and width if necessary.
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    // Struct for storing queue family indices.
    struct QueueFamilyIndices {
        std::optional<uint32_t> m_graphics;
        std::optional<uint32_t> m_presentation;

        constexpr bool HasEverything() const noexcept {
            return m_graphics.has_value() && m_presentation.has_value();
        }
    };

    // Struct for storing the capabilities of the surface.
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR m_capabilities;
        std::vector<VkSurfaceFormatKHR> m_formats;
        std::vector<VkPresentModeKHR> m_presentModes;
    };

    // Gets the queue families of a physical device.
    QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);

    // Gets the swap chain capabilities of a physical device. 
    SwapChainSupportDetails GetSwapChainSupport(VkPhysicalDevice device);

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

    // Creates the image views needed for the swap chain.
    void CreateImageViews();
    
    // Creates the render pass.
    void CreateRenderPass();

    // Creates the graphics pipeline used for testing.
    // TODO replace with pipeline factory utility.
    void CreateTestGraphicsPipeline();

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_logicalDevice = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentationQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};

}
