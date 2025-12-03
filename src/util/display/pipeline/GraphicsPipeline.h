#pragma once

#include "util/display/RenderSystem.h"
#include "util/display/pipeline/Scissor.h"
#include "util/display/pipeline/VertexFormat.h"
#include "util/display/pipeline/Viewport.h"
#include "util/Constants.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vulkan/vulkan.h>

class UniformBuffer;

// Utility for creating graphics pipelines.
class GraphicsPipeline final {
public:
    constexpr GraphicsPipeline(const char* vertex, const char* fragment) {
        m_vertex = vertex;
        m_fragment = fragment;
    }

    constexpr GraphicsPipeline& PolygonMode(eRenderPolygonMode mode) noexcept {
        m_polygonMode = mode;

        return *this;
    }

    constexpr GraphicsPipeline& CullMode(eRenderCullMode mode) noexcept {
        m_cullMode = mode;

        return *this;
    }

    constexpr GraphicsPipeline& BlendFunc(eRenderBlendFactor sfactor, eRenderBlendFactor dfactor) noexcept {
        m_blendFunc = {sfactor, dfactor, sfactor, dfactor};

        return *this;
    }

    constexpr GraphicsPipeline& BlendFunc(eRenderBlendFactor sfactor0, eRenderBlendFactor dfactor0, eRenderBlendFactor sfactor1, eRenderBlendFactor dfactor1) noexcept {
        m_blendFunc = {sfactor0, dfactor0, sfactor1, dfactor1};

        return *this;
    }

    constexpr GraphicsPipeline& Viewport(const RenderViewport& viewport) noexcept {
        m_viewport = viewport;

        return *this;
    }

    constexpr GraphicsPipeline& Scissor(const RenderScissor& scissor) noexcept {
        m_scissor = scissor;

        return *this;
    }

    constexpr GraphicsPipeline& Vertex(const VertexFormat& format, eRenderVertexMode mode) noexcept {
        m_vertexFormat = format;
        m_vertexMode = mode;

        return *this;
    }

    constexpr GraphicsPipeline& Uniform(uint32_t binding, eRenderShaderStage stage, UniformBuffer* ubo, uint32_t count = 1) noexcept {
        if (m_layoutBinding.has_value())
            return *this;

        m_layoutBinding = {
            .binding = binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = count,
            .stageFlags = stage,
            .pImmutableSamplers = VK_NULL_HANDLE
        };

        m_ubo = ubo;

        return *this;
    }

    void Build();
    void Delete();
private:
    friend class RenderSystem;
    friend class UniformBuffer;

    void CmdBind();

    const char* m_vertex;
    const char* m_fragment;
    std::optional<eRenderPolygonMode> m_polygonMode;
    std::optional<eRenderCullMode> m_cullMode;
    std::optional<std::array<eRenderBlendFactor, 4>> m_blendFunc;
    std::optional<RenderViewport> m_viewport;
    std::optional<RenderScissor> m_scissor;
    std::optional<VertexFormat> m_vertexFormat;
    std::optional<eRenderVertexMode> m_vertexMode;
    VkPipeline m_handler = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    std::optional<VkDescriptorSetLayoutBinding> m_layoutBinding;
    UniformBuffer* m_ubo = nullptr;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSets[RenderSystem::MAX_FRAMES_IN_FLIGHT];
};
