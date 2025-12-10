#pragma once

#include "util/Logger.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <vector>
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
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
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        std::optional<uint32_t> transfer;

        constexpr const bool IsFull() const noexcept {
            return graphics.has_value() && present.has_value() && transfer.has_value();
        }

        constexpr const bool IsSame() const noexcept {
            return m_uniques.size() == 1;
        }

        constexpr const size_t UniqueSize() const noexcept {
            return m_uniques.size();
        }

        constexpr const uint32_t GetUnique(size_t i) const noexcept {
            return m_data.at(i);
        }

        constexpr const uint32_t* Data() const noexcept {
            return m_data.data();
        }
    private:
        friend GPUDevice;

        std::set<uint32_t> m_uniques;
        std::vector<uint32_t> m_data;
    };

    struct SwapchainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    void Build(vk::Instance& instance, vk::SurfaceKHR& surface);

    constexpr operator vk::PhysicalDevice() noexcept {
        return device;
    }

    constexpr const vk::PhysicalDeviceProperties GetProperties() const noexcept {
        return m_properties;
    }

    constexpr const vk::PhysicalDeviceFeatures GetFeatures() const noexcept {
        return m_features;
    }

    constexpr const vk::PhysicalDeviceMemoryProperties GetMemoryProperties() const noexcept {
        return m_memoryProperties;
    }

    constexpr const QueueFamilyIndices* GetQueueFamilies() const noexcept {
        return &m_queueFamilyIndices;
    }

    constexpr const SwapchainSupportDetails GetSwapchainSupport() const noexcept {
        return m_swapchainSupport;
    }

    constexpr const bool IsExtensionSupported(const char* extension) const {
        if (auto search = m_supportedExtensions.find(extension); search != m_supportedExtensions.end())
            return true;

        return false;
    }

    constexpr uint32_t FindMemoryType(uint32_t filter, vk::MemoryPropertyFlags properties) {
        for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
            if (filter & (1 << i) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;

        throw sLogger.RuntimeError("Failed to find suitable memory type!");
    }

    vk::PhysicalDevice device;
private:
    static Logger sLogger;

    bool IsGPUUsable(vk::PhysicalDevice& gpu, vk::Instance& instance, vk::SurfaceKHR& surface);
    bool CheckGPUExtensionSupport(vk::PhysicalDevice& gpu);
    void CreateQueueFamilyIndices(vk::PhysicalDevice& gpu, vk::Instance& instance);
    void GetSwapchainSupport(vk::PhysicalDevice& gpu, vk::SurfaceKHR& surface);
    size_t GetGPUScore(vk::PhysicalDevice& gpu);

    vk::PhysicalDeviceProperties m_properties;
    vk::PhysicalDeviceFeatures m_features;
    vk::PhysicalDeviceMemoryProperties m_memoryProperties;
    std::set<const char*> m_supportedExtensions;
    QueueFamilyIndices m_queueFamilyIndices;
    std::vector<vk::QueueFamilyProperties> m_queueFamilyProperties;
    SwapchainSupportDetails m_swapchainSupport;
};
