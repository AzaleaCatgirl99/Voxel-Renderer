#include "renderer/Renderer.h"

#include "renderer/detail/VkRenderer.h"
#include "util/Window.h"

void Renderer::Create(Renderer* instance, Window* window) {
    if (instance == nullptr) {
        instance = new Renderer();

        switch (window->Pipeline()) {
        case RENDER_PIPELINE_VULKAN:
            instance->m_context = new detail::VkRenderer(window);
            break;
        }

        instance->m_context->Initialize();
    }
}

void Renderer::Destroy() {
    m_context->Destroy();
}
