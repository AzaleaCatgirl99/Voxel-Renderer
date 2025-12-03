#pragma once

#include "util/display/RenderSystem.h"
#include "util/display/buffer/GPUBuffer.h"
#include <cstdint>
#include <cstring>

class GraphicsPipeline;

// Class for handling uniform buffers.
class UniformBuffer final {
public:
    constexpr UniformBuffer(uint32_t size) {
        m_size = size;
    }

    constexpr void Build() {
        for (uint32_t i = 0; i < RenderSystem::MAX_FRAMES_IN_FLIGHT; i++) {
            m_buffers[i] = GPUBuffer(GPU_BUFFER_SHARING_MODE_EXCLUSIVE, m_size)
                                .UsageFlag(GPU_BUFFER_USAGE_UNIFORM_BUFFER)
                                .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                                .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            m_buffers[i].Build();
            // Data needs to be mapped in order to change consistently.
            m_buffers[i].MapData(&m_data[i], m_size);
        }
    }

    constexpr void Update(const void* data) {
        memcpy(m_data[RenderSystem::sCurrentFrame], data, m_size);
    }

    constexpr void Delete() {
        for (uint32_t i = 0; i < RenderSystem::MAX_FRAMES_IN_FLIGHT; i++)
            m_buffers[i].Delete();
    }
private:
    friend class GraphicsPipeline;

    uint32_t m_size = 0;
    GPUBuffer m_buffers[RenderSystem::MAX_FRAMES_IN_FLIGHT];
    void* m_data[RenderSystem::MAX_FRAMES_IN_FLIGHT];
};
