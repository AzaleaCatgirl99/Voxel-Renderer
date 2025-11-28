#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_main.h>
#include "util/Constants.h"
#include "util/Logger.h"

namespace detail {
class VkRenderer;
}

// Struct for toggling default display features.
struct DisplayMode final {
    int m_width = 0;
    int m_height = 0;
    int m_refreshRate = 0;
    bool m_fullscreen = false;
    bool m_highDPI = true;
    bool m_resizable = true;
    bool m_borderless = false;
    int m_minWidth = 0;
    int m_minHeight = 0;
};

// Utility for handling window management.
class Window final {
public:
    static void Create(const char* title, const DisplayMode& mode, eRenderPipeline pipeline);

    // Gets the window width.
    static constexpr const int Width() {
        int width = 0;
        SDL_GetWindowSize(sContext, &width, nullptr);
        return width;
    }

    // Gets the window height.
    static constexpr const int Height() {
        int height = 0;
        SDL_GetWindowSize(sContext, nullptr, &height);
        return height;
    }

    // Gets the window width with the DPI.
    static constexpr const int DisplayWidth() {
        return (int)(Width() * DPIScale());
    }

    // Gets the window height with the DPI.
    static constexpr const int DisplayHeight() {
        return (int)(Height() * DPIScale());
    }

    // Gets the scale for the DPI.
    static constexpr const float DPIScale() {
        return SDL_GetWindowDisplayScale(sContext);
    }

    // Gets the window's aspect ratio.
    static constexpr const float ApsectRatio() {
        float width = 0.0f, height = 0.0f;
        SDL_GetWindowAspectRatio(sContext, &width, &height);
        return width / height;
    }

    // Gets the render pipeline for the window.
    static constexpr const eRenderPipeline& Pipeline() noexcept {
        return sPipeline;
    }

    // Sets the window to fullscreen.
    static constexpr void SetFullscreen(bool toggle) {
        SDL_SetWindowFullscreen(sContext, toggle);
    }

    // Sets the window's title.
    static constexpr void SetTitle(const char* title) {
        SDL_SetWindowTitle(sContext, title);
    }

    // Destroys the window.
    static constexpr void Destroy() {
        SDL_DestroyWindow(sContext);
        SDL_Quit();
    }

    // Gets the current SDL event.
    static constexpr const SDL_Event* GetEvent() noexcept {
        return &sEvent;
    }

    // Polls the current SDL event.
    static constexpr bool PollEvent() {
        return SDL_PollEvent(&sEvent);
    }
private:
    Window() = default;

    friend detail::VkRenderer;

    static SDL_Window* sContext;
    static SDL_Event sEvent;
    static eRenderPipeline sPipeline;
    static Logger sLogger;
};
