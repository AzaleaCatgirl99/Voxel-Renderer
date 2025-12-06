#pragma once

#include "util/Constants.h"
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.h>

enum eGPUBufferUsage {
    GPU_BUFFER_USAGE_TRANSFER_SRC = 0x00000001,
    GPU_BUFFER_USAGE_TRANSFER_DST = 0x00000002,
    GPU_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER = 0x00000004,
    GPU_BUFFER_USAGE_STORAGE_TEXEL_BUFFER = 0x00000008,
    GPU_BUFFER_USAGE_UNIFORM_BUFFER = 0x00000010,
    GPU_BUFFER_USAGE_STORAGE_BUFFER = 0x00000020,
    GPU_BUFFER_USAGE_INDEX_BUFFER = 0x00000040,
    GPU_BUFFER_USAGE_VERTEX_BUFFER = 0x00000080,
    GPU_BUFFER_USAGE_INDIRECT_BUFFER = 0x00000100
};

enum eGPUBufferCreateFlag {
    GPU_BUFFER_CREATE_SPARSE_BINDING = 0x00000001,
    GPU_BUFFER_CREATE_SPARSE_RESIDENCY = 0x00000002,
    GPU_BUFFER_CREATE_SPARSE_ALIASED = 0x00000004
};

enum eGPUBufferMemoryProperty {
    GPU_BUFFER_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001,
    GPU_BUFFER_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002,
    GPU_BUFFER_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004,
    GPU_BUFFER_MEMORY_PROPERTY_HOST_CACHED_BIT = 0x00000008,
    GPU_BUFFER_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT = 0x00000010
};

// Class for handling the creation and access of GPU buffers.
class GPUBuffer final {
public:
    constexpr GPUBuffer() = default;
    constexpr GPUBuffer(eRenderSharingMode mode, uint32_t size) {
        m_sharingMode = mode;
        m_size = size;
    }

    constexpr GPUBuffer& UsageFlag(eGPUBufferUsage usage) noexcept {
        m_usages |= usage;

        return *this;
    }

    constexpr GPUBuffer& CreateFlag(eGPUBufferCreateFlag flag) noexcept {
        m_flags |= flag;

        return *this;
    }

    constexpr GPUBuffer& MemoryPropertiesFlag(eGPUBufferMemoryProperty property) noexcept {
        m_memProperties |= property;

        return *this;
    }

    void Build();
    void Allocate(const void* data, uint32_t size, uint32_t offset = 0);
    void MapData(void** data, uint32_t size, uint32_t offset = 0);
    void Copy(GPUBuffer& src, uint32_t size, uint32_t src_offset = 0, uint32_t dst_offset = 0);
    void Delete();
private:
    friend class RenderSystem;
    friend class VertexBuffer;
    friend class IndexBuffer;
    friend class GraphicsPipeline;
    friend class GPUImage;

    eRenderSharingMode m_sharingMode;
    uint32_t m_size = 0;
    uint32_t m_usages = 0;
    uint32_t m_flags = 0;
    uint32_t m_memProperties = 0;
    VkBuffer m_handler = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
};
