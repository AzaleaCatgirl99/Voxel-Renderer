#pragma once

#include "util/display/RenderSystem.h"
#include "util/display/pipeline/VertexFormat.h"
#include "util/display/buffer/GPUBuffer.h"
#include <cstdint>
#include <SDL3/SDL_platform_defines.h>
#include <vulkan/vulkan.h>

// Class for handling vertex buffer creation/usage.
class VertexBuffer final {
public:
    constexpr VertexBuffer(uint32_t size, const VertexFormat format) {
        m_buffer = GPUBuffer(SHARING_MODE, size * format.m_stride)
                    .UsageFlag(GPU_BUFFER_USAGE_TRANSFER_DST).UsageFlag(GPU_BUFFER_USAGE_VERTEX_BUFFER)
                    .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_size = size;
        m_format = format;
    }

    constexpr void Build() {
        m_buffer.Build();
    }

    constexpr void Allocate(const void* data) {
        // A staging buffer is needed in order to move over the data to the GPU.
        GPUBuffer staging = GPUBuffer(SHARING_MODE, m_size * m_format.m_stride)
                        .UsageFlag(GPU_BUFFER_USAGE_TRANSFER_SRC)
                        .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                        .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        staging.Build();
        staging.Allocate(data, m_size * m_format.m_stride);

        m_buffer.Copy(staging, m_size * m_format.m_stride);

        // The staging buffer is deleted once the copy is done because it is not needed anymore.
        staging.Delete();
    }

    constexpr void Delete() {
        m_buffer.Delete();
    }
private:
    friend class RenderSystem;

#ifdef SDL_PLATFORM_APPLE // Fix for Apple devices only having one queue family.
    static constexpr const eRenderSharingMode SHARING_MODE = RENDER_SHARING_MODE_EXCLUSIVE;
#else
    static constexpr const eRenderSharingMode SHARING_MODE = RENDER_SHARING_MODE_CONCURRENT;
#endif

    GPUBuffer m_buffer;
    uint32_t m_size = 0;
    VertexFormat m_format;
};
