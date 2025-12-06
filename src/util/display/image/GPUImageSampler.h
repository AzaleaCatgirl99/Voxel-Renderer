#pragma once

#include "util/Constants.h"
#include <vulkan/vulkan.h>

// Class/utility for making/deleting image samplers.
// TODO add final features
class GPUImageSampler final {
public:
    constexpr GPUImageSampler() = default;
    constexpr GPUImageSampler(eRenderFilter min_filter, eRenderFilter mag_filter, eRenderWrapMode wrap,
                                int mipmap = 0, float anisotropy = 0.0f) {
        m_minFilter = min_filter;
        m_magFilter = mag_filter;
        m_wrap = wrap;
        m_mipmap = mipmap;
        m_anisotropy = anisotropy;
    }

    void Build();
    void Delete();
private:
    VkSampler m_handler = VK_NULL_HANDLE;
    eRenderFilter m_minFilter;
    eRenderFilter m_magFilter;
    eRenderWrapMode m_wrap;
    int m_mipmap = 0;
    float m_anisotropy = 0.0f;
};
