#include "renderer/Renderer.h"

#include "renderer/detail/VkRenderer.h"

void Renderer::Create(Renderer* instance, eRenderPipeline pipeline) {
    if (instance == nullptr) {
        instance = new Renderer();

        switch (pipeline) {
        case RENDER_PIPELINE_VULKAN:
            instance->m_context = new detail::VkRenderer();
        }

        instance->m_context->Initialize();
    }
}

void Renderer::Destroy() {
    m_context->Destroy();
}
