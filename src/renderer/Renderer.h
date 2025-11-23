#pragma once

namespace detail {
class IRenderer;
}

class Window;

// Main renderer class. Wraps various different implementations of the renderer based on the given pipeline from the window.
class Renderer final {
public:
    static void Create(Renderer* instance, Window* window);

    void Destroy();
private:
    Renderer() = default;

    detail::IRenderer* m_context = nullptr;
};
