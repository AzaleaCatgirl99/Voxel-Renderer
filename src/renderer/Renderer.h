#pragma once

namespace detail {
class IRenderer;
}

enum eRenderPipeline {
    RENDER_PIPELINE_VULKAN
};

// Main renderer class. Wraps various different implementations of the renderer based on the given pipeline.
class Renderer final {
public:
    static void Create(Renderer* instance, eRenderPipeline pipeline = RENDER_PIPELINE_VULKAN);

    void Destroy();
private:
    Renderer() = default;

    detail::IRenderer* m_context = nullptr;
};
