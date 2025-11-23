#pragma once

#include "renderer/detail/IRenderer.h"
#include <vulkan/vulkan.h>

namespace detail {

// Renderer implementation that uses Vulkan.
class VkRenderer : public IRenderer {
public:
    constexpr VkRenderer(Window* window) noexcept : IRenderer(window) {
    }

    virtual void Initialize() override;

    virtual void Destroy() override;
private:
    VkInstance m_instance;
};

}
