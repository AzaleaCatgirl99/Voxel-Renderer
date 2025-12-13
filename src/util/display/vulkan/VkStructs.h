#pragma once

#include "util/display/RenderConstants.h"
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

class VertexFormat;

// List of structs & enums used in RenderSystem. These are separated out into a special namespace and header to clean up the main file.
namespace VkStructs {

// Various different rendering swap intervals.
// https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentModeKHR.html
enum class SwapInterval {
    eImmediate,
    eVSync,
    eTripleBuffering
};

// Struct for storing uniform buffer data.
struct UBO {
    uint32_t size = 0;
    vk::Buffer buffers[VXL_RS_MAX_FRAMES_IN_FLIGHT];
    vk::DeviceMemory memory[VXL_RS_MAX_FRAMES_IN_FLIGHT];
    void* ptrs[VXL_RS_MAX_FRAMES_IN_FLIGHT];
};

// Struct for storing pipeline data.
struct Pipeline {
    // Info struct for creating a pipeline.
    struct Info {
        vk::PipelineShaderStageCreateInfo* shaderStages = nullptr;
        uint32_t shaderStageCount = 0;
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
        vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
        bool blending = false;
        vk::BlendFactor colorSrcFactor = vk::BlendFactor::eOne;
        vk::BlendFactor colorDstFactor = vk::BlendFactor::eZero;
        vk::BlendFactor alphaSrcFactor = vk::BlendFactor::eOne;
        vk::BlendFactor alphaDstFactor = vk::BlendFactor::eZero;
        vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
        bool useVertexFormat = false;
        const VertexFormat* vertexFormat;
        vk::VertexInputRate vertexInputRate = vk::VertexInputRate::eVertex;
        vk::Viewport viewport;
        vk::Rect2D scissor;
        vk::DescriptorSetLayoutBinding* bindings = nullptr;
        uint32_t bindingCount = 0;
        vk::DynamicState* dynamicStates = nullptr;
        uint32_t dynamicStateCount = 0;
    };

    VXL_INLINE operator vk::Pipeline&() noexcept {
        return pipeline;
    }

    vk::Pipeline pipeline;
    vk::PipelineLayout layout;
    vk::DescriptorSetLayout descriptorSetLayout;
};

}
