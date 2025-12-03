#pragma once

#include "util/display/pipeline/Type.h"
#include "util/display/buffer/GPUBuffer.h"
#include <cstdint>
#include <SDL3/SDL_platform_defines.h>

// Class for handling index buffer creation/usage.
class IndexBuffer final {
public:
    constexpr IndexBuffer(uint32_t size, eRenderType type) {
        m_buffer = GPUBuffer(SHARING_MODE, size * GetRenderTypeSize(type))
                    .UsageFlag(GPU_BUFFER_USAGE_TRANSFER_DST).UsageFlag(GPU_BUFFER_USAGE_INDEX_BUFFER)
                    .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_size = size * GetRenderTypeSize(type);
        m_type = type;
    }

    constexpr void Build() {
        m_buffer.Build();
    }

    constexpr void Allocate(const void* data) {
        // A staging buffer is needed in order to move over the data to the GPU.
        GPUBuffer staging = GPUBuffer(SHARING_MODE, m_size)
                        .UsageFlag(GPU_BUFFER_USAGE_TRANSFER_SRC)
                        .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                        .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        staging.Build();
        staging.Allocate(data, m_size);

        m_buffer.Copy(staging, m_size);

        // The staging buffer is deleted once the copy is done because it is not needed anymore.
        staging.Delete();
    }

    constexpr void Delete() {
        m_buffer.Delete();
    }
private:
    friend class RenderSystem;

#ifdef SDL_PLATFORM_APPLE // Fix for Apple devices only having one queue family.
    static constexpr const eGPUBufferSharingMode SHARING_MODE = GPU_BUFFER_SHARING_MODE_EXCLUSIVE;
#else
    static constexpr const eGPUBufferSharingMode SHARING_MODE = GPU_BUFFER_SHARING_MODE_CONCURRENT;
#endif

    GPUBuffer m_buffer;
    uint32_t m_size = 0;
    eRenderType m_type;
};
