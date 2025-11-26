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
    Window(const char* title, const DisplayMode& mode, eRenderPipeline pipeline);

    // Gets the window width.
    constexpr const int Width() const {
        int width = 0;
        SDL_GetWindowSize(m_pHandler, &width, nullptr);
        return width;
    }

    // Gets the window height.
    constexpr const int Height() const {
        int height = 0;
        SDL_GetWindowSize(m_pHandler, nullptr, &height);
        return height;
    }

    // Gets the window width with the DPI.
    constexpr const int DisplayWidth() const {
        return (int)(Width() * DPIScale());
    }

    // Gets the window height with the DPI.
    constexpr const int DisplayHeight() const {
        return (int)(Height() * DPIScale());
    }

    // Gets the scale for the DPI.
    constexpr const float DPIScale() const {
        return SDL_GetWindowDisplayScale(m_pHandler);
    }

    // Gets the window's aspect ratio.
    constexpr const float ApsectRatio() const {
        float width = 0.0f, height = 0.0f;
        SDL_GetWindowAspectRatio(m_pHandler, &width, &height);
        return width / height;
    }

    // Gets the render pipeline for the window.
    constexpr const eRenderPipeline& Pipeline() const noexcept {
        return m_ePipeline;
    }

    // Sets the window to fullscreen.
    constexpr void SetFullscreen(bool toggle) {
        SDL_SetWindowFullscreen(m_pHandler, toggle);
    }

    // Sets the window's title.
    constexpr void SetTitle(const char* title) {
        SDL_SetWindowTitle(m_pHandler, title);
    }

    // Destroys the window.
    constexpr void Destroy() {
        SDL_DestroyWindow(m_pHandler);
        SDL_Quit();
    }

    // Gets the current SDL event.
    constexpr const SDL_Event* GetEvent() const noexcept {
        return &m_uEvent;
    }

    // Polls the current SDL event.
    constexpr bool PollEvent() {
        return SDL_PollEvent(&m_uEvent);
    }
private:
    friend detail::VkRenderer;

    static Logger sLogger;

    SDL_Window* m_pHandler = nullptr;
    SDL_Event m_uEvent;
    eRenderPipeline m_ePipeline;
};
