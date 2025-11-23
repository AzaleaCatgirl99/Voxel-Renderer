#pragma once

#include "renderer/detail/IRenderer.h"

namespace detail {

// Renderer implementation that uses Vulkan.
class VkRenderer : public IRenderer {
public:
    constexpr VkRenderer(Window* window) noexcept : IRenderer(window) {
    }

    virtual void Initialize() override;

    virtual void Destroy() override;
};

}
