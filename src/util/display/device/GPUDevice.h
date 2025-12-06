#pragma once

#include "util/Logger.h"
#include <array>
#include <cstddef>
#include <optional>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <SDL3/SDL_platform_defines.h>

class GPUDevice final {
public:
#ifdef SDL_PLATFORM_APPLE // Apple device support.
    static constexpr const uint32_t EXTENSION_COUNT = 2;
#else
    static constexpr const uint32_t EXTENSION_COUNT = 1;
#endif
    static constexpr std::array<const char*, EXTENSION_COUNT> EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME

#ifdef SDL_PLATFORM_APPLE // Apple device support.
        , VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> m_graphics;
        std::optional<uint32_t> m_present;
        std::optional<uint32_t> m_transfer;

        constexpr const bool IsFull() const noexcept {
            return m_graphics.has_value() && m_present.has_value() && m_transfer.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR m_capabilities;
        std::vector<VkSurfaceFormatKHR> m_formats;
        std::vector<VkPresentModeKHR> m_presentModes;
    };

    constexpr GPUDevice(VkInstance instance, VkSurfaceKHR surface) {
        m_instance = instance;
        m_surface = surface;
    }

    void Build();

    constexpr operator VkPhysicalDevice() noexcept {
        return m_context;
    }

    constexpr const VkPhysicalDeviceProperties GetProperties() const noexcept {
        return m_properties;
    }

    constexpr const VkPhysicalDeviceFeatures GetFeatures() const noexcept {
        return m_features;
    }

    constexpr const VkPhysicalDeviceMemoryProperties GetMemoryProperties() const noexcept {
        return m_memoryProperties;
    }

    constexpr const QueueFamilyIndices GetQueueFamilies() const noexcept {
        return m_queueFamilyIndices;
    }

    constexpr const SwapChainSupportDetails GetSwapChainSupport() const noexcept {
        return m_swapChainSupport;
    }

    constexpr const bool IsExtensionSupported(const char* extension) const {
        if (auto search = m_supportedExtensions.find(extension); search != m_supportedExtensions.end())
            return true;

        return false;
    }

    constexpr uint32_t FindMemoryType(uint32_t filter, VkMemoryPropertyFlags properties) {
        for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
            if (filter & (1 << i) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;

        throw sLogger.RuntimeError("Failed to find suitable memory type!");
    }
private:
    static Logger sLogger;

    bool IsGPUUsable(VkPhysicalDevice gpu);
    bool CheckGPUExtensionSupport(VkPhysicalDevice gpu);
    void CreateQueueFamilyIndices();
    void GetSwapChainSupport(VkPhysicalDevice gpu);
    size_t GetGPUScore(VkPhysicalDevice gpu);

    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_context = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_properties;
    VkPhysicalDeviceFeatures m_features;
    VkPhysicalDeviceMemoryProperties m_memoryProperties;
    std::set<const char*> m_supportedExtensions;
    QueueFamilyIndices m_queueFamilyIndices;
    uint32_t m_queueFamilyPropertiesSize = 0;
    VkQueueFamilyProperties* m_queueFamilyProperties = nullptr;
    SwapChainSupportDetails m_swapChainSupport;
};
