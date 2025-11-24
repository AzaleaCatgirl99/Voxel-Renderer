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

    virtual void Initialize() override;

    virtual void Destroy() override;
private:
    static constexpr const bool sEnableValidationLayers = true;

    static constexpr const uint32_t sEnabledSeverityFlags = 
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    static Logger sLogger;
    static std::vector<const char*> sValidationLayers;

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT pMessageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT pMessageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    void CreateInstance();

    void SetupDebugMessenger();

    VkApplicationInfo CreateAppInfo();

    std::vector<const char*> GetRequiredExtensions();

    VkInstanceCreateInfo CreateInstanceInfo();

    bool CheckValidationLayerSupport();

    VkResult CreateDebugUtilsMessengerEXT(
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator);

    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
};

}
