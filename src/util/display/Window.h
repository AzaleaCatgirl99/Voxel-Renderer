#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
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

    // ========== Create & Destroy ==========

    static void Create(const char* title, const DisplayMode& mode);

    // Destroys the window.
    static VXL_INLINE void Destroy() {
        SDL_DestroyWindow(sContext);
        SDL_Quit();
    }

    // ========== Main Getters ==========

    // Gets the SDL window context.
    static VXL_INLINE SDL_Window* GetContext() noexcept {
        return sContext;
    }

    // Checks if the window is minimized.
    static VXL_INLINE const bool IsMinimized() noexcept {
        return sMinimized;
    }

    // ========== Property Getters ==========

    // Gets the window size.
    static VXL_INLINE const glm::ivec2 GetSize() {
        glm::ivec2 size;
        SDL_GetWindowSize(sContext, &size.x, &size.y);
        return size;
    }

    // Gets the window size with the DPI.
    static VXL_INLINE const glm::ivec2 GetDisplaySize() {
        glm::ivec2 size = GetSize();
        size.x *= GetDPIScale();
        size.y *= GetDPIScale();

        return size;
    }

    // Gets the scale for the DPI.
    static VXL_INLINE const float GetDPIScale() {
        return SDL_GetWindowDisplayScale(sContext);
    }

    // Gets the window's aspect ratio.
    static VXL_INLINE const float ApsectRatio() {
        const glm::ivec2 size = GetSize();
        return (float)size.x / (float)size.y;
    }

    // ========== Property Setters ==========

    // Sets the window to fullscreen.
    static VXL_INLINE void SetFullscreen(bool toggle) {
        SDL_SetWindowFullscreen(sContext, toggle);
    }

    // Sets the window's title.
    static VXL_INLINE void SetTitle(const char* title) {
        SDL_SetWindowTitle(sContext, title);
    }

    // ========== Event & Input ==========

    // Gets the current SDL event.
    static VXL_INLINE const SDL_Event* GetEvent() noexcept {
        return &sEvent;
    }

    // Polls the current SDL event.
    static VXL_INLINE bool PollEvent() {
        return SDL_PollEvent(&sEvent);
    }

    // Gets the mouse's position.
    static VXL_INLINE const glm::vec2 GetMousePos() {
        glm::vec2 pos;
        SDL_GetMouseState(&pos.x, &pos.y);
        return pos;
    }

    // Gets the mouse's relative position.
    static VXL_INLINE const glm::vec2 GetMouseRelativePos() {
        glm::vec2 pos;
        SDL_GetRelativeMouseState(&pos.x, &pos.y);
        return pos;
    }

    // Sets the mouse as grabbed by the window.
    static void SetMouseGrabbed(bool grabbed);

    // Checks if the mouse is grabbed by the window or not.
    static VXL_INLINE const bool IsMouseGrabbed() noexcept {
        return sMouseGrabbed;
    }

    // Toggles the mouse grabbing.
    static VXL_INLINE void ToggleMouseGrabbed() {
        SetMouseGrabbed(!sMouseGrabbed);
    }

    // Sets the mouse position.
    static VXL_INLINE void SetMousePosition(float x, float y) {
        SDL_WarpMouseInWindow(sContext, x, y);
    }

    // Centers the mouse position.
    static VXL_INLINE void CenterMousePosition() {
        glm::ivec2 size = GetSize();
        SDL_WarpMouseInWindow(sContext, size.x / 2.0f, size.y / 2.0f);
    }

    // Gets the key states.
    static VXL_INLINE const bool* GetKeyStates() noexcept {
        return sKeyStates;
    }

    // ========== Surface & Rendering ==========

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
private:
    Window() = default;

    // Watches for specific events.
    static bool EventWatcher(void* app_data, SDL_Event* event);

    static SDL_Window* sContext;
    static SDL_Event sEvent;
    static Logger sLogger;
    static bool sMinimized;
    static bool sMouseGrabbed;
    static const bool* sKeyStates;
};
