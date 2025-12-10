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
    static VXL_INLINE const int Width() {
        int width = 0;
        SDL_GetWindowSize(sContext, &width, nullptr);
        return width;
    }

    // Gets the window height.
    static VXL_INLINE const int Height() {
        int height = 0;
        SDL_GetWindowSize(sContext, nullptr, &height);
        return height;
    }

    // Gets the window width with the DPI.
    static VXL_INLINE const int DisplayWidth() {
        return (int)(Width() * DPIScale());
    }

    // Gets the window height with the DPI.
    static VXL_INLINE const int DisplayHeight() {
        return (int)(Height() * DPIScale());
    }

    // Gets the scale for the DPI.
    static VXL_INLINE const float DPIScale() {
        return SDL_GetWindowDisplayScale(sContext);
    }

    // Gets the window's aspect ratio.
    static VXL_INLINE const float ApsectRatio() {
        return (float)Width() / (float)Height();
    }

    // Sets the window to fullscreen.
    static VXL_INLINE void SetFullscreen(bool toggle) {
        SDL_SetWindowFullscreen(sContext, toggle);
    }

    // Sets the window's title.
    static VXL_INLINE void SetTitle(const char* title) {
        SDL_SetWindowTitle(sContext, title);
    }

    // Destroys the window.
    static VXL_INLINE void Destroy() {
        SDL_DestroyWindow(sContext);
        SDL_Quit();
    }

    // Gets the current SDL event.
    static VXL_INLINE const SDL_Event* GetEvent() noexcept {
        return &sEvent;
    }

    // Polls the current SDL event.
    static VXL_INLINE bool PollEvent() {
        return SDL_PollEvent(&sEvent);
    }

    // Gets the mouse's X position.
    static VXL_INLINE const float MouseX() {
        float x;
        SDL_GetMouseState(&x, nullptr);
        return x;
    }

    // Gets the mouse's Y position.
    static VXL_INLINE const float MouseY() {
        float y;
        SDL_GetMouseState(nullptr, &y);
        return y;
    }

    // Gets the mouse's relative X position.
    static VXL_INLINE const float MouseRelativeX() {
        float x;
        SDL_GetRelativeMouseState(&x, nullptr);
        return x;
    }

    // Gets the mouse's relative Y position.
    static VXL_INLINE const float MouseRelativeY() {
        float y;
        SDL_GetRelativeMouseState(nullptr, &y);
        return y;
    }

    // Gets the mouse's relative position.
    static VXL_INLINE const glm::vec2 MouseRelativePos() {
        glm::vec2 ret;
        SDL_GetRelativeMouseState(&ret.x, &ret.y);
        return ret;
    }

    // Sets the mouse as grabbed by the window.
    static VXL_INLINE void SetMouseGrabbed(bool grabbed) {
        if (grabbed)
            SDL_HideCursor();
        else
            SDL_ShowCursor();

        SDL_SetWindowRelativeMouseMode(sContext, grabbed);
    }

    // Sets the mouse position.
    static VXL_INLINE void SetMousePosition(float x, float y) {
        SDL_WarpMouseInWindow(sContext, x, y);
    }

    // Creates a Vulkan window surface.
    static VXL_INLINE vk::SurfaceKHR CreateSurface(vk::Instance& instance) {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (!SDL_Vulkan_CreateSurface(sContext, instance, VK_NULL_HANDLE, &surface))
            throw sLogger.RuntimeError("Failed to create window surface! ", SDL_GetError());

        return surface;
    }

    // Destroys a Vulkan window surface.
    static VXL_INLINE void DestroySurface(vk::Instance& instance, vk::SurfaceKHR& surface) {
        SDL_Vulkan_DestroySurface(instance, surface, VK_NULL_HANDLE);
    }

    // Gets the SDL window context.
    static VXL_INLINE SDL_Window* GetContext() noexcept {
        return sContext;
    }

    // Checks if the window is minimized.
    static VXL_INLINE const bool IsMinimized() noexcept {
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
