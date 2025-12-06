#pragma once

#include "util/display/RenderSystem.h"
#include "util/display/pipeline/Descriptor.h"
#include "util/display/pipeline/VertexFormat.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.h>
#include <experimental/simd>

class UniformBuffer;
class Texture;

// Utility for creating graphics pipelines.
class GraphicsPipeline final {
public:
    struct Info {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        bool blending = false;
        VkBlendFactor colorSrcFactor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor colorDstFactor = VK_BLEND_FACTOR_ZERO;
        VkBlendFactor alphaSrcFactor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor alphaDstFactor = VK_BLEND_FACTOR_ZERO;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        bool useVertexFormat = false;
        VertexFormat* vertexFormat;
        VkViewport viewport;
        VkRect2D scissor;
        Descriptor* descriptors = nullptr;
        uint32_t descriptorCount = 0;
        VkDynamicState* dynamicStates = nullptr;
        uint32_t dynamicStateCount = 0;
    };

    constexpr GraphicsPipeline(Info& info) {
        m_info = info;
    }

    void Build();
    void UpdateUniformDescSet(uint32_t binding, UniformBuffer& buffer);
    void Delete();
private:
    friend class RenderSystem;
    friend class UniformBuffer;

    constexpr VkPipelineRasterizationStateCreateInfo CreateRasterizationInfo() noexcept {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE, // Discard off-screen fragments, do not clamp.
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = m_info.polygonMode,
            .cullMode = m_info.cullMode,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f
        };
    }

    constexpr VkPipelineColorBlendAttachmentState CreateColorInfo() noexcept {
        return {
            .blendEnable = m_info.blending ? VK_TRUE : VK_FALSE,
            .srcColorBlendFactor = m_info.colorSrcFactor,
            .dstColorBlendFactor = m_info.colorDstFactor,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = m_info.alphaSrcFactor,
            .dstAlphaBlendFactor = m_info.alphaDstFactor,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = 
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT
        };
    }

    VkShaderModule CreateShader(const char* path);
    void CmdBind();

    Info m_info;
    VkPipeline m_handler = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    std::vector<std::array<VkDescriptorSet, RenderSystem::MAX_FRAMES_IN_FLIGHT>> m_descriptorSets;
};
