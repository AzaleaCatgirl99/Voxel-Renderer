#pragma once

#include <vulkan/vulkan.h>

class GPUImage;

// Class/utility for making/deleting image views easy.
class GPUImageView final {
public:
    constexpr GPUImageView() = default;
    constexpr GPUImageView(GPUImage* image) {
        m_image = image;
    }

    void Build();
    void Delete();
private:
    VkImageView m_handler = VK_NULL_HANDLE;
    GPUImage* m_image = nullptr;
};
