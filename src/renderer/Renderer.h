#pragma once

#include "renderer/detail/IRenderer.h"
#include "renderer/pipeline/GraphicsPipeline.h"
#include "util/Constants.h"
#include <cassert>

// Main renderer class. Wraps various different implementations of the renderer based on the given pipeline from the window.
class Renderer final {
public:
    // Various settings for the renderer.
    struct Settings {
        eRenderSwapInterval m_defaultSwapInterval = RENDER_SWAP_INTERVAL_IMMEDIATE;
        bool m_useImGUI = true;
    };

    // Creates the renderer context.
    static void CreateContext(const Settings& settings);

    // Destroys the renderer context.
    static void DestroyContext();

    // Initializes ImGUI.
    static constexpr void InitImGUI() {
        assert(sContext != nullptr);

        sContext->InitImGUI();
    }

    // Updates the display.
    static constexpr void UpdateDisplay() {
        assert(sContext != nullptr);

        sContext->UpdateDisplay();
    }

    // Begins drawing the frame.
    static constexpr void BeginDrawFrame() {
        assert(sContext != nullptr);

        sContext->BeginDrawFrame();
    }

    // Ends drawing the frame.
    static constexpr void EndDrawFrame() {
        assert(sContext != nullptr);

        sContext->EndDrawFrame();
    }

    // Registers a graphics pipeline.
    static constexpr void RegisterPipeline(const GraphicsPipeline& pipeline) {
        assert(sContext != nullptr);

        sContext->RegisterPipeline(pipeline);
    }

    // Creates a buffer.
    static constexpr void CreateBuffer(const GPUBuffer& buffer) {
        assert(sContext != nullptr);

        sContext->CreateBuffer(buffer);
    }

    // Allocates memory to a buffer.
    static constexpr void AllocateBufferMemory(const GPUBuffer& buffer, void* data, uint32_t size, uint32_t offset = 0) {
        assert(sContext != nullptr);

        sContext->AllocateBufferMemory(buffer, data, size, offset);
    }

    // Binds a graphics pipeline.
    static constexpr void CmdBindPipeline(const GraphicsPipeline& pipeline) {
        assert(sContext != nullptr);

        sContext->CmdBindPipeline(pipeline);
    }

    // Binds a buffer.
    static constexpr void CmdBindBuffer(const GPUBuffer& buffer) {
        assert(sContext != nullptr);

        sContext->CmdBindBuffer(buffer);
    }

    // Draws.
    static constexpr void CmdDraw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex = 0, uint32_t first_instance = 0) {
        assert(sContext != nullptr);

        sContext->CmdDraw(vertex_count, instance_count, first_vertex, first_instance);
    }
private:
    static detail::IRenderer* sContext;
};
