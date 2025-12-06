#pragma once

#include "util/Constants.h"
#include <cstdint>
#include <vulkan/vulkan.h>

struct GPUImageSize {
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_depth = 0;
    uint32_t m_mipmapLevel = 0;
    uint32_t m_layers = 0;
};

class GPUBuffer;
class BufferedImage;

class GPUImage final {
public:
    constexpr GPUImage() = default;
    constexpr GPUImage(const GPUImageSize& size, eRenderImageType type, eRenderColorFormat format, eRenderSampleCount samples, 
            uint32_t usages, eRenderImageTiling tiling, eRenderSharingMode sharing_mode, eRenderImageLayout layout) {
        m_size = size;
        m_type = type;
        m_format = format;
        m_samples = samples;
        m_usages = usages;
        m_tiling = tiling;
        m_sharingMode = sharing_mode;
        m_layout = layout;
    }

    void Build();
    void Allocate(const void* data);
    void Allocate(BufferedImage& image);
    // TODO make this copy buffer function more configurable.
    void CopyBuffer(GPUBuffer& buffer);
    void Delete();
private:
    friend class GPUImageView;

    VkImage m_handler = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    GPUImageSize m_size;
    eRenderImageType m_type;
    eRenderColorFormat m_format;
    eRenderSampleCount m_samples;
    uint32_t m_usages = 0;
    eRenderImageTiling m_tiling;
    eRenderSharingMode m_sharingMode;
    eRenderImageLayout m_layout;
};
