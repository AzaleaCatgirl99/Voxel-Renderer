#pragma once

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <glm/glm.hpp>
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "util/Logger.h"

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
    static void Create(const char* title, const DisplayMode& mode);

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
        return (float)Width() / (float)Height();
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

    // Gets the mouse's X position.
    static constexpr const float MouseX() {
        float x;
        SDL_GetMouseState(&x, nullptr);
        return x;
    }

    // Gets the mouse's Y position.
    static constexpr const float MouseY() {
        float y;
        SDL_GetMouseState(nullptr, &y);
        return y;
    }

    // Gets the mouse's relative X position.
    static constexpr const float MouseRelativeX() {
        float x;
        SDL_GetRelativeMouseState(&x, nullptr);
        return x;
    }

    // Gets the mouse's relative Y position.
    static constexpr const float MouseRelativeY() {
        float y;
        SDL_GetRelativeMouseState(nullptr, &y);
        return y;
    }

    // Gets the mouse's relative position.
    static constexpr const glm::vec2 MouseRelativePos() {
        glm::vec2 ret;
        SDL_GetRelativeMouseState(&ret.x, &ret.y);
        return ret;
    }

    // Sets the mouse as grabbed by the window.
    static constexpr void SetMouseGrabbed(bool grabbed) {
        if (grabbed)
            SDL_HideCursor();
        else
            SDL_ShowCursor();

        SDL_SetWindowRelativeMouseMode(sContext, grabbed);
    }

    // Sets the mouse position.
    static constexpr void SetMousePosition(float x, float y) {
        SDL_WarpMouseInWindow(sContext, x, y);
    }

    // Creates a Vulkan window surface.
    static constexpr vk::SurfaceKHR CreateSurface(vk::Instance& instance) {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (!SDL_Vulkan_CreateSurface(sContext, instance, VK_NULL_HANDLE, &surface))
            throw sLogger.RuntimeError("Failed to create window surface! ", SDL_GetError());

        return surface;
    }

    // Destroys a Vulkan window surface.
    static constexpr void DestroySurface(vk::Instance& instance, vk::SurfaceKHR& surface) {
        SDL_Vulkan_DestroySurface(instance, surface, VK_NULL_HANDLE);
    }

    // Gets the SDL window context.
    static constexpr SDL_Window* GetContext() noexcept {
        return sContext;
    }

    // Checks if the window is minimized.
    static constexpr const bool IsMinimized() noexcept {
        return sMinimized;
    }
private:
    Window() = default;

    // Watches for specific events.
    static bool EventWatcher(void* app_data, SDL_Event* event);

    static SDL_Window* sContext;
    static SDL_Event sEvent;
    static Logger sLogger;
    static bool sMinimized;
};
