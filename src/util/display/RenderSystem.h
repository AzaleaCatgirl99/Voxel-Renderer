#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vulkan/vulkan.h>
#include "util/Constants.h"
#include "util/Features.h"
#include "util/Logger.h"
#include <vector>

// Renderer implementation that uses Vulkan.
class RenderSystem {
public:
    struct Settings {
        eRenderSwapInterval m_swapInterval;
    };

    static void Initialize(const Settings& settings);
    static void Destroy();
    static void UpdateDisplay();
    static void BeginDrawFrame();
    static void EndDrawFrame();
    static void CmdDraw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex = 0, uint32_t first_instance = 0);
    static void WaitDevice();
private:
    friend class GraphicsPipeline;
    friend class GPUBuffer;
    friend class ImGUIHelper;

    struct QueueFamilyIndices {
        std::optional<uint32_t> m_graphics;
        std::optional<uint32_t> m_presentation;

        constexpr bool HasEverything() const noexcept {
            return m_graphics.has_value() && m_presentation.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR m_capabilities;
        std::vector<VkSurfaceFormatKHR> m_formats;
        std::vector<VkPresentModeKHR> m_presentModes;
    };

#ifdef VXL_DEBUG
        static constexpr const bool ENABLE_VALIDATION_LAYERS = true;
#else
        static constexpr const bool ENABLE_VALIDATION_LAYERS = false;
#endif
    static constexpr const uint32_t ENABLED_SEVERITY_FLAGS =
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
    static constexpr const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    static Settings sSettings;
    static VkInstance sInstance;
    static VkDebugUtilsMessengerEXT sDebugMessenger;
    static VkPhysicalDevice sPhysicalDevice;
    static VkDevice sDevice;
    static VkQueue sGraphicsQueue;
    static VkQueue sPresentationQueue;
    static VkSurfaceKHR sSurface;
    static VkSwapchainKHR sSwapChain;
    static std::vector<VkImage> sSwapChainImages;
    static VkFormat sSwapChainImageFormat;
    static VkExtent2D sSwapChainExtent;
    static std::vector<VkImageView> sSwapChainImageViews;
    static VkRenderPass sRenderPass;
    static std::vector<VkFramebuffer> sSwapChainFramebuffers;
    static VkCommandPool sCommandPool;
    static uint32_t sCurrentFrame;
    static QueueFamilyIndices sQueueFamilies;

    static VkCommandBuffer sCommandBuffers[MAX_FRAMES_IN_FLIGHT];
    static VkSemaphore sImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    static VkFence sInFlightFences[MAX_FRAMES_IN_FLIGHT];

    static std::vector<VkSemaphore> sRenderFinishedSemaphores;

    static uint32_t sImageIndex;

    static std::vector<const char*> sLayers;
    static std::vector<const char*> sInstanceExtensions;
    static std::vector<const char*> sDeviceExtensions;
    static Logger sLogger;
    static VkApplicationInfo sAppInfo;
    static VkDebugUtilsMessengerCreateInfoEXT sDebugMessengerInfo;

    static void SetupRequiredExtensions();
    static bool CheckValidationLayerSupport();
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);
    static size_t GetPhysicalDeviceScore(VkPhysicalDevice device);
    static void CreateInstance();
    static void CreateDebugMessenger();
    static void CreateSwapChain();
    static void SelectBestPhysicalDevice();
    static bool IsPhysicalDeviceUsable(VkPhysicalDevice device);
    static bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    static QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);
    static SwapChainSupportDetails GetSwapChainSupport(VkPhysicalDevice device);
    static void CreateLogicalDevice();
    static VkResult CreateDebugUtilsMessengerEXT(
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        const VkAllocationCallbacks* allocator,
        VkDebugUtilsMessengerEXT* debug_messenger);
    static void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* allocator);
    static void CreateSurface();
    static void CreateImageViews();
    static void CreateRenderPass();
    static void CreateFramebuffers();
    static void CreateCommandPool();
    static void CreateCommandBuffer();
    static void CreateSyncObjects();
    static void BeginRecordCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    static void EndRecordCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    static void RecreateSwapChain();
    static uint32_t FindMemoryType(uint32_t filter, VkMemoryPropertyFlags properties);
};
