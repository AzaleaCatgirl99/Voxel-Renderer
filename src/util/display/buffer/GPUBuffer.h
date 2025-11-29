#pragma once

#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.h>

// List of available usages for a GPU buffer.
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

// List of available sharing modes for a GPU buffer.
enum eGPUBufferSharingMode {
    GPU_BUFFER_SHARING_MODE_EXCLUSIVE,
    GPU_BUFFER_SHARING_MODE_CONCURRENT
};

// List of available create flags for a GPU buffer
enum eGPUBufferCreateFlag {
    GPU_BUFFER_CREATE_SPARSE_BINDING = 0x00000001,
    GPU_BUFFER_CREATE_SPARSE_RESIDENCY = 0x00000002,
    GPU_BUFFER_CREATE_SPARSE_ALIASED = 0x00000004
};

// Class for handling the creation and access of GPU buffers.
class GPUBuffer final {
public:
    constexpr GPUBuffer(eGPUBufferSharingMode mode, uint32_t size) {
        m_sharingMode = mode;
        m_size = size;
    }

    // Adds a usage flag.
    constexpr GPUBuffer& Usage(eGPUBufferUsage usage) noexcept {
        m_usages |= usage;

        return *this;
    }

    // Adds a create flag.
    constexpr GPUBuffer& Flag(eGPUBufferCreateFlag flag) noexcept {
        m_flags |= flag;

        return *this;
    }

    void Build();
    void Allocate(void* data, uint32_t size, uint32_t offset = 0);
    void CmdBind();
    void Delete();
private:
    friend class RenderSystem;

    eGPUBufferSharingMode m_sharingMode;
    uint32_t m_size = 0;
    uint32_t m_usages = 0;
    uint32_t m_flags = 0;
    VkBuffer m_handler = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
};
