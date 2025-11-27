#pragma once

#include "util/Constants.h"

class Window;

namespace detail {

// Internal base virtual renderer.
class IRenderer {
public:
    struct Properties {
        eRenderSwapInterval m_swapInterval;
    };

    constexpr IRenderer(Window* window, const Properties& properties) noexcept {
        m_window = window;
        m_properties = properties;
    }

    virtual void Initialize() = 0;
    virtual void Destroy() = 0;
    virtual void UpdateDisplay() = 0;
    virtual void DrawFrame() = 0;

    virtual ~IRenderer() {
    }
protected:
    Window* m_window = nullptr;
    Properties m_properties;
};

}
