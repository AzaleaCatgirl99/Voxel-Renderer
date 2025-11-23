#include "util/Window.h"

#include <SDL3/SDL_video.h>

Window::Window(const char* title, const DisplayMode& mode, eRenderPipeline pipeline) {
    m_pipeline = pipeline;

    // Initializes the the SDL3 video system, which is required for window management.
	SDL_Init(SDL_INIT_VIDEO);

    // SDL flags for use in the window.
    SDL_WindowFlags flags;

    // Sets the flag for the correct rendering pipeline.
    switch (pipeline) {
    case RENDER_PIPELINE_VULKAN:
        flags = SDL_WINDOW_VULKAN;
        break;
    }

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
	m_internal = SDL_CreateWindow(title, mode.m_width, mode.m_height, flags);

    SDL_SetWindowMinimumSize(m_internal, mode.m_minWidth, mode.m_minHeight);
}
