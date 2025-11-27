#pragma once

#include "renderer/detail/IRenderer.h"
#include "util/Constants.h"
#include <cassert>

class Window;

// Main renderer class. Wraps various different implementations of the renderer based on the given pipeline from the window.
class Renderer final {
public:
    // Various settings for the renderer.
    struct Settings {
        eRenderSwapInterval m_defaultSwapInterval = RENDER_SWAP_INTERVAL_IMMEDIATE;
    };

    // Creates the renderer context.
    static void CreateContext(Window* window, const Settings& settings);

    // Destroys the renderer context.
    static void DestroyContext();

    // Updates the display.
    static constexpr void UpdateDisplay() {
        assert(sContext != nullptr);

        sContext->UpdateDisplay();
    }

    // Draws the frame.
    static constexpr void DrawFrame() {
        assert(sContext != nullptr);

        sContext->DrawFrame();
    }
private:
    static detail::IRenderer* sContext;
};
