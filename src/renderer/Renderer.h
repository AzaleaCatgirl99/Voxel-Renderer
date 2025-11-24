#pragma once

#include "renderer/detail/IRenderer.h"

class Window;

// Main renderer class. Wraps various different implementations of the renderer based on the given pipeline from the window.
class Renderer final {
public:
    static void Create(Renderer* instance, Window* window);

    static void Destroy(Renderer* instance);

    constexpr void UpdateDisplay() {
        m_context->UpdateDisplay();
    }
private:
    Renderer() = default;
    ~Renderer();

    detail::IRenderer* m_context = nullptr;
};
