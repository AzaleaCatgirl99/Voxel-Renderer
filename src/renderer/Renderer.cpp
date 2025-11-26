#include "renderer/Renderer.h"

#include "renderer/detail/IRenderer.h"
#include "renderer/detail/VkRenderer.h"
#include "util/Window.h"
#include <cassert>

detail::IRenderer* Renderer::sContext = nullptr;

void Renderer::CreateContext(Window* window, const Settings& settings) {
    // Asserts that the context must be nullptr to make sure that it is not being created multiple times.
    assert(sContext == nullptr);

    // Asserts that the window is not nullptr. There must be a window to use the renderer.
    assert(window != nullptr);

    // The propertirs for the context.
    detail::IRenderer::Properties properties = {
        .m_swapInterval = settings.m_defaultSwapInterval
    };

    // Creates the correct context for the window's pipeline.
    switch (window->Pipeline()) {
    case RENDER_PIPELINE_VULKAN:
        sContext = new detail::VkRenderer(window, properties);
        break;
    }

    // Initializes the context.
    sContext->Initialize();
}

void Renderer::DestroyContext() {
    // Asserts that the context is not nullptr to make sure that it is not being destroyed when it shouldn't be.
    assert(sContext != nullptr);

    // Destroys the context and sets it to nullptr.
    sContext->Destroy();
    delete sContext;
    sContext = nullptr;
}
