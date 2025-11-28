#pragma once

#include "renderer/pipeline/Scissor.h"
#include "renderer/pipeline/Viewport.h"
#include "util/Constants.h"
#include <array>
#include <optional>
#include <string>

namespace detail {
class VkRenderer;
}

// Utility for creating graphics pipelines.
class GraphicsPipeline final {
public:
    constexpr GraphicsPipeline() = default;
    consteval GraphicsPipeline(const char* name, const char* vertex, const char* fragment) {
        m_name = name;
        m_vertex = vertex;
        m_fragment = fragment;
    }

    // Sets the polygon mode.
    constexpr GraphicsPipeline& PolygonMode(eRenderPolygonMode mode) noexcept {
        m_polygonMode = mode;

        return *this;
    }

    // Sets the cull mode.
    constexpr GraphicsPipeline& CullMode(eRenderCullMode mode) noexcept {
        m_cullMode = mode;

        return *this;
    }

    // Sets the blend func.
    constexpr GraphicsPipeline& BlendFunc(eRenderBlendFactor sfactor, eRenderBlendFactor dfactor) noexcept {
        m_blendFunc = {sfactor, dfactor, sfactor, dfactor};

        return *this;
    }

    // Sets the blend func.
    constexpr GraphicsPipeline& BlendFunc(eRenderBlendFactor sfactor0, eRenderBlendFactor dfactor0, eRenderBlendFactor sfactor1, eRenderBlendFactor dfactor1) noexcept {
        m_blendFunc = {sfactor0, dfactor0, sfactor1, dfactor1};

        return *this;
    }

    // Sets the viewport.
    constexpr GraphicsPipeline& Viewport(const RenderViewport& viewport) noexcept {
        m_viewport = viewport;

        return *this;
    }

    // Sets the scissor.
    constexpr GraphicsPipeline& Scissor(const RenderScissor& scissor) noexcept {
        m_scissor = scissor;

        return *this;
    }

    constexpr bool operator==(const GraphicsPipeline& other) noexcept {
        return m_name == other.m_name;
    }

    constexpr bool operator!=(const GraphicsPipeline& other) noexcept {
        return m_name != other.m_name;
    }
    
    constexpr bool operator<(const GraphicsPipeline& other) const noexcept {
        return m_name < other.m_name;
    }

    constexpr bool operator>(const GraphicsPipeline& other) const noexcept {
        return m_name > other.m_name;
    }
private:
    friend detail::VkRenderer;

    std::string m_name;
    const char* m_vertex;
    const char* m_fragment;
    std::optional<eRenderPolygonMode> m_polygonMode;
    std::optional<eRenderCullMode> m_cullMode;
    std::optional<std::array<eRenderBlendFactor, 4>> m_blendFunc;
    std::optional<RenderViewport> m_viewport;
    std::optional<RenderScissor> m_scissor;
};
