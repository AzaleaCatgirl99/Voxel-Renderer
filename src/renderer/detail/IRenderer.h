#pragma once

class Window;

namespace detail {

// Internal base virtual renderer.
class IRenderer {
public:
    constexpr IRenderer(Window* window) noexcept {
        m_window = window;
    }

    virtual void Initialize() = 0;

    virtual void Destroy() = 0;
protected:
    Window* m_window = nullptr;
};

}
