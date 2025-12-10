#include "util/display/Window.h"

#include "util/display/RenderSystem.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

SDL_Window* Window::sContext = nullptr;
Logger Window::sLogger = Logger("Window");
SDL_Event Window::sEvent;
bool Window::sMinimized = false;

void Window::Create(const char* title, const DisplayMode& mode) {
    // Initializes the the SDL3 video system, which is required for window management.
	if (!SDL_Init(SDL_INIT_VIDEO))
        throw sLogger.RuntimeError("Failed to initialize SDL Video.");

    // SDL flags for use in the window.
    SDL_WindowFlags flags = SDL_WINDOW_VULKAN;

    // If fullscreen is enabled, adds the fullscreen flag.
    if (mode.m_fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;

    // If resizable is enabled, adds the resizable flag.
    if (mode.m_resizable)
        flags |= SDL_WINDOW_RESIZABLE;

    // If high DPI is enabled, adds the high DPI flag.
    if (mode.m_highDPI)
        flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

    // If borderless is enabled, adds the borderless flag.
    if (mode.m_borderless)
        flags |= SDL_WINDOW_BORDERLESS;

    // Creates a resizable window. Aborts the app if failed.
	sContext = SDL_CreateWindow(title, mode.m_width, mode.m_height, flags);
    if (!sContext)
        throw sLogger.RuntimeError("Failed to create internal window.");

    if (!SDL_SetWindowMinimumSize(sContext, mode.m_minWidth, mode.m_minHeight))
        throw sLogger.RuntimeError("Failed to set the minimum size.");

    SDL_AddEventWatch(EventWatcher, nullptr);
}

bool Window::EventWatcher(void* app_data, SDL_Event* event) {
    if (event->type == SDL_EVENT_WINDOW_EXPOSED)
        RenderSystem::RecreateSwapchain();

    if (event->type == SDL_EVENT_WINDOW_MINIMIZED)
        sMinimized = true;
    else
        sMinimized = false;

    return true;
}
