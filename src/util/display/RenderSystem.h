#pragma once

#include "util/BufferedFile.h"
#include "util/display/RenderConstants.h"
#include "util/display/pipeline/VertexFormat.h"
#include "util/display/vulkan/VkConversions.h"
#include "util/display/vulkan/VkInitializer.h"
#include "util/display/vulkan/VkStructs.h"
#include "vulkan/vulkan.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include "util/Logger.h"
#include "util/display/device/GPUDevice.h"
#include <vector>

class BufferedImage;

// Rendering implementation that uses Vulkan.
// TODO add anisotropic filtering check
class RenderSystem final {
public:
    // Various different rendering swap intervals.
    // https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentModeKHR.html
    using SwapInterval = VkStructs::SwapInterval;
    // Struct for storing uniform buffer data.
    using UBO = VkStructs::UBO;
    // Struct for storing pipeline data.
    using Pipeline = VkStructs::Pipeline;

    struct Settings {
        SwapInterval swapInterval;
        std::function<void(vk::CommandBuffer&)> cmdCallback;
    };

    static void Initialize(const Settings& settings);
    static void Destroy();
    static void RecreateSwapchain();
    static void UpdateDisplay();

    // ========== Commands ==========

    static VXL_INLINE std::vector<vk::CommandBuffer> CreateCmdBuffers(uint32_t count, std::optional<vk::CommandPool> pool = std::nullopt) {
        vk::CommandBufferAllocateInfo info = {
            .commandPool = pool.has_value() ? pool.value() : sGraphicsCmdPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = count
        };

        // Annoyingly HPP won't allow the use of just normal pointers for data.
        return sDevice.allocateCommandBuffers(info);
    }

    static vk::CommandBuffer CreateCmdBuffer(std::optional<vk::CommandPool> pool = std::nullopt);
    static vk::CommandBuffer BeginDataTransfer();
    static void EndDataTransfer(vk::CommandBuffer& buffer);

    // ========== Memory & Buffers ==========

    static UBO CreateUniformBuffer(vk::DeviceSize size);
    static vk::DeviceMemory CreateMemory(vk::Buffer& buffer, vk::MemoryPropertyFlags properties, bool bind = true);
    static void AllocateStagedBufferMemory(vk::Buffer& buffer, vk::DeviceMemory& memory, const void* data, vk::DeviceSize size);
    static void CopyBuffer(vk::Buffer& dst, vk::Buffer& src, vk::DeviceSize size, vk::DeviceSize src_offset = 0, vk::DeviceSize dst_offset = 0);

    static VXL_INLINE vk::Buffer CreateBuffer(vk::SharingMode mode, uint32_t size, vk::BufferCreateFlags flags, vk::BufferUsageFlags usages) {
        vk::BufferCreateInfo info = {
            .flags = flags,
            .size = size,
            .usage = usages,
            .sharingMode = mode
        };

        return sDevice.createBuffer(info);
    }

    static VXL_INLINE vk::Buffer CreateVertexBuffer(vk::DeviceSize size, const VertexFormat format) {
        return CreateBuffer(sGPU.GetQueueFamilies()->IsSame() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
                    size * format.GetStride(), {},
                    vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    }

    static VXL_INLINE vk::Buffer CreateIndexBuffer(vk::DeviceSize size, vk::IndexType type = vk::IndexType::eUint16) {
        return CreateBuffer(sGPU.GetQueueFamilies()->IsSame() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
                    size * VkConversions::GetIndexTypeSize(type), {},
                    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    }

    static VXL_INLINE vk::Buffer CreateStorageBuffer(vk::DeviceSize size) {
        return CreateBuffer(sGPU.GetQueueFamilies()->IsSame() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
                    size, {},
                    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
    }

    static VXL_INLINE void BindMemory(vk::Buffer& buffer, vk::DeviceMemory& memory, vk::DeviceSize offset = 0) {
        sDevice.bindBufferMemory(buffer, memory, offset);
    }

    static VXL_INLINE void* MapMemory(vk::DeviceMemory& memory, vk::DeviceSize size, vk::DeviceSize offset = 0) {
        return sDevice.mapMemory(memory, offset, size);
    }

    static VXL_INLINE void UnmapMemory(vk::DeviceMemory& memory) {
        sDevice.unmapMemory(memory);
    }

    static VXL_INLINE void AllocateMemory(vk::DeviceMemory& memory, const void* data, vk::DeviceSize size, vk::DeviceSize offset = 0) {
        void* ptr = MapMemory(memory, size, offset);
        memcpy(ptr, data, size);
        UnmapMemory(memory);
    }

    static VXL_INLINE void FreeMemory(vk::DeviceMemory& memory) {
        sDevice.freeMemory(memory);
    }

    static VXL_INLINE void DestroyBuffer(vk::Buffer& buffer) {
        sDevice.destroyBuffer(buffer);
    }

    static VXL_INLINE void UpdateUniformBuffer(UBO& ubo, const void* data) {
        memcpy(ubo.ptrs[sCurrentFrame], data, ubo.size);
    }

    static VXL_INLINE void DestroyUniformBuffer(UBO& ubo) {
        for (uint32_t i = 0; i < VXL_RS_MAX_FRAMES_IN_FLIGHT; i++) {
            sDevice.destroyBuffer(ubo.buffers[i]);
            sDevice.freeMemory(ubo.memory[i]);
        }
    }

    // ========== Pipeline, Shaders & Descriptors ==========

    static void CreateDescriptorSets(vk::DescriptorSet* sets, uint32_t count, vk::DescriptorPool& pool, vk::DescriptorSetLayout* layouts);

    static VXL_INLINE vk::ShaderModule CreateShader(const std::string& path) {
        BufferedFile file(path);

        vk::ShaderModuleCreateInfo info = {
            .codeSize = file.Size(),
            .pCode = file.DataAsUInt32()
        };

        return sDevice.createShaderModule(info);
    }

    static VXL_INLINE vk::DescriptorPool CreateDescriptorPool(vk::DescriptorPoolSize* sizes, uint32_t size_count, uint32_t sets) {
        vk::DescriptorPoolCreateInfo info = {
            .maxSets = sets,
            .poolSizeCount = size_count,
            .pPoolSizes = sizes
        };

        return sDevice.createDescriptorPool(info);
    }

    static VXL_INLINE Pipeline CreatePipeline(Pipeline::Info& info, vk::RenderPass& pass = sDefaultRenderPass) {
        return VkInitializer::CreatePipeline(sDevice, pass, info);
    }

    static VXL_INLINE void DestroyShader(vk::ShaderModule& shader) {
        sDevice.destroyShaderModule(shader);
    }

    static VXL_INLINE void DestroyDescriptorPool(vk::DescriptorPool& pool) {
        sDevice.destroyDescriptorPool(pool);
    }

    static VXL_INLINE void UpdateDescriptorSets(uint32_t write_count, const vk::WriteDescriptorSet* writes,
                                                uint32_t copy_count = 0, const vk::CopyDescriptorSet* copies = VK_NULL_HANDLE) {
        sDevice.updateDescriptorSets(write_count, writes, copy_count, copies);
    }

    static VXL_INLINE void DestroyPipeline(Pipeline& pipeline) {
        if (pipeline.descriptorSetLayout != VK_NULL_HANDLE)
            sDevice.destroyDescriptorSetLayout(pipeline.descriptorSetLayout);

        sDevice.destroyPipeline(pipeline.pipeline);
        sDevice.destroyPipelineLayout(pipeline.layout);
    }

    // ========== Images & Textures ==========

    static vk::DeviceMemory CreateImageMemory(vk::Image& image, vk::MemoryPropertyFlags properties, bool bind = true);
    static void AllocateImageMemory(vk::Image& image, vk::DeviceMemory& memory, const void* data, vk::DeviceSize size);
    static void AllocateImageMemory(vk::Image& image, vk::DeviceMemory& memory, BufferedImage& data);
    static void CopyBufferToImage(vk::Image& dst, vk::Buffer& src, vk::DeviceSize size, vk::DeviceSize src_offset = 0, vk::DeviceSize dst_offset = 0);
    // "Trans"ition :hehe: - AzaleaCatgirl99
    static void TransitionImageLayout(vk::Image& image, vk::ImageLayout cis, vk::ImageLayout trans);

    static VXL_INLINE vk::Image CreateImage(vk::ImageType type, uint32_t level, uint32_t width, uint32_t height,
                                            uint32_t depth, uint32_t layers, vk::ImageUsageFlags usage,
                                            vk::ImageTiling tiling = vk::ImageTiling::eOptimal,
                                            vk::Format format = vk::Format::eR32G32B32A32Uint) {
        return sDevice.createImage({
            .imageType = type,
            .format = format,
            .extent = {
                .width = width,
                .height = height,
                .depth = depth
            },
            .mipLevels = level + 1,
            .arrayLayers = layers,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = tiling,
            .usage = usage,
            .sharingMode = sGPU.GetQueueFamilies()->IsSame() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent
        });
    }

    static VXL_INLINE vk::Image CreateImage2D(uint32_t level, uint32_t width, uint32_t height, vk::ImageUsageFlags usage,
                                                vk::Format format = vk::Format::eR32G32B32A32Uint) {
        return CreateImage(vk::ImageType::e2D, level, width, height, 1, 1, usage);
    }

    static VXL_INLINE vk::Image CreateImage2DArray(uint32_t level, uint32_t width, uint32_t height, uint32_t layers,
                                                    vk::ImageUsageFlags usage, vk::Format format = vk::Format::eR32G32B32A32Uint) {
        return CreateImage(vk::ImageType::e2D, level, width, height, 1, layers, usage);
    }

    static VXL_INLINE vk::Image CreateImage3D(uint32_t level, uint32_t width, uint32_t height, uint32_t depth,
                                                vk::ImageUsageFlags usage, vk::Format format = vk::Format::eR32G32B32A32Uint) {
        return CreateImage(vk::ImageType::e3D, level, width, height, depth, 1, usage);
    }

    static VXL_INLINE vk::Image CreateImage3DArray(uint32_t level, uint32_t width, uint32_t height, uint32_t depth, uint32_t layers,
                                                vk::ImageUsageFlags usage, vk::Format format = vk::Format::eR32G32B32A32Uint) {
        return CreateImage(vk::ImageType::e3D, level, width, height, depth, layers, usage);
    }

    static VXL_INLINE void BindImageMemory(vk::Image& image, vk::DeviceMemory& memory, vk::DeviceSize offset = 0) {
        sDevice.bindImageMemory(image, memory, offset);
    }

    // ========== Getters & Setters ==========

    static VXL_INLINE const uint32_t GetCurrentFrame() noexcept {
        return sCurrentFrame;
    }

    static VXL_INLINE vk::RenderPass& GetDefaultRenderPass() noexcept {
        return sDefaultRenderPass;
    }

    static VXL_INLINE vk::CommandBuffer& GetCurrentCmdBuffer() noexcept {
        return sCommandBuffers[sCurrentFrame];
    }

    static VXL_INLINE vk::Device& GetDevice() noexcept {
        return sDevice;
    }
private:
    static Settings sSettings;

    // Physical Device variables.
    static GPUDevice sGPU;

    // Logical Device variables.
    static vk::Device sDevice;
    static vk::Queue sGraphicsQueue;
    static vk::Queue sPresentQueue;
    static vk::Queue sTransferQueue;
    static vk::CommandPool sGraphicsCmdPool;
    static vk::CommandPool sTransferCmdPool;

    static vk::Instance sInstance;
    static vk::SurfaceKHR sSurface;
    static vk::RenderPass sDefaultRenderPass;
    static uint32_t sCurrentFrame;

    static vk::CommandBuffer sCommandBuffers[VXL_RS_MAX_FRAMES_IN_FLIGHT];
    static vk::Semaphore sImageAvailableSemaphores[VXL_RS_MAX_FRAMES_IN_FLIGHT];
    static vk::Fence sInFlightFences[VXL_RS_MAX_FRAMES_IN_FLIGHT];

    static std::vector<vk::Semaphore> sRenderFinishedSemaphores;
    static std::vector<vk::Framebuffer> sFramebuffers;

    static Logger sLogger;

    static void CreateInstance();
    static std::vector<const char*> GetRequiredExtensions();
    static void CreateFramebuffers();
    static void CreateCommandBuffer();
    static void CreateSyncObjects();
    static void BeginRecordCmdBuffer(vk::CommandBuffer& buffer, uint32_t imageIndex);
};
