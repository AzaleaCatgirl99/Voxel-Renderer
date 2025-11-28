#pragma once

#include "util/Constants.h"
#include <cstdint>

class GraphicsPipeline;

namespace detail {

// Internal base virtual renderer.
class IRenderer {
public:
    struct Properties {
        eRenderSwapInterval m_swapInterval;
    };

    constexpr IRenderer(const Properties& properties) noexcept {
        m_properties = properties;
    }

    virtual void Initialize() = 0;
    virtual void Destroy() = 0;
    virtual void UpdateDisplay() = 0;
    virtual void BeginDrawFrame() = 0;
    virtual void EndDrawFrame() = 0;
    virtual void RegisterPipeline(const GraphicsPipeline& pipeline) = 0;
    virtual void CmdBindPipeline(const GraphicsPipeline& pipeline) = 0;
    virtual void CmdDraw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) = 0;

    virtual ~IRenderer() {
    }
protected:
    Properties m_properties;
};

}
