#pragma once

#include "util/Logger.h"
#include <cstdint>
#include <optional>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <SDL3/SDL_platform_defines.h>

class GPUDevice;

class RenderDevice final {
public:
    constexpr RenderDevice(VkInstance instance, GPUDevice* gpu) {
        m_instance = instance;
        m_gpu = gpu;
    }

    void Build();
    VkCommandPool CreateCmdPool(uint32_t family_index);
    VkCommandBuffer CreateCmdBuffer(std::optional<VkCommandPool> pool = std::nullopt);
    void CreateCmdBuffers(VkCommandBuffer* buffers, uint32_t count, std::optional<VkCommandPool> pool = std::nullopt);

    constexpr void Delete() {
        vkDestroyCommandPool(m_context, m_graphicsCmdPool, VK_NULL_HANDLE);
        vkDestroyCommandPool(m_context, m_transferCmdPool, VK_NULL_HANDLE);

        vkDestroyDevice(m_context, VK_NULL_HANDLE);
    }

    constexpr void FreeCmdBuffer(VkCommandBuffer buffer, std::optional<VkCommandPool> pool = std::nullopt) {
        vkFreeCommandBuffers(m_context, pool.has_value() ? pool.value() : m_graphicsCmdPool, 1, &buffer);
    }

    constexpr void FreeCmdBuffers(VkCommandBuffer* buffers, uint32_t count, std::optional<VkCommandPool> pool = std::nullopt) {
        vkFreeCommandBuffers(m_context, pool.has_value() ? pool.value() : m_graphicsCmdPool, count, buffers);
    }

    constexpr VkQueue GetGraphicsQueue() noexcept {
        return m_graphicsQueue;
    }

    constexpr VkQueue GetPresentQueue() noexcept {
        return m_presentQueue;
    }

    constexpr VkQueue GetTransferQueue() noexcept {
        return m_transferQueue;
    }

    constexpr VkCommandPool GetGraphicsCmdPool() noexcept {
        return m_graphicsCmdPool;
    }

    constexpr VkCommandPool GetTransferCmdPool() noexcept {
        return m_transferCmdPool;
    }

    constexpr operator VkDevice() noexcept {
        return m_context;
    }
private:
    static Logger sLogger;

    VkInstance m_instance = VK_NULL_HANDLE;
    GPUDevice* m_gpu = nullptr;
    VkDevice m_context = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;
    VkCommandPool m_graphicsCmdPool = VK_NULL_HANDLE;
    VkCommandPool m_transferCmdPool = VK_NULL_HANDLE;
};
