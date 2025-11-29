#include "util/display/pipeline/GraphicsPipeline.h"

#include "util/BufferedFile.h"
#include "util/display/RenderSystem.h"
#include "util/display/vulkan/VkResultHandler.h"
#include "util/display/vulkan/VkObjectMaps.h"
#include <SDL3/SDL_filesystem.h>

void GraphicsPipeline::Build() {
    std::string basePath = SDL_GetBasePath();
    BufferedFile vertShader = BufferedFile::Read(basePath + "assets/shaders/" + m_vertex, true);
    BufferedFile fragShader = BufferedFile::Read(basePath + "assets/shaders/" + m_fragment, true);

    VkShaderModuleCreateInfo vertShaderCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vertShader.Size(),
        .pCode = vertShader.DataAsUInt32()
    };

    VkShaderModule vertShaderModule;
    VkResult result = vkCreateShaderModule(RenderSystem::sDevice, &vertShaderCreateInfo, VK_NULL_HANDLE, &vertShaderModule);
    VkResultHandler::CheckResult(result, "Failed to create vertex shader module!");

    VkShaderModuleCreateInfo fragShaderCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fragShader.Size(),
        .pCode = fragShader.DataAsUInt32()
    };

    VkShaderModule fragShaderModule;
    result = vkCreateShaderModule(RenderSystem::sDevice, &fragShaderCreateInfo, VK_NULL_HANDLE, &fragShaderModule);
    VkResultHandler::CheckResult(result, "Failed to create fragment shader module!");

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStates;

    if (!m_viewport.has_value())
        dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);

    if (!m_scissor.has_value())
        dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // Checks whether the pipeline has a vertex format. If it does, then it'll setup the binding and attribute descriptions.
    bool useVertex = m_vertexFormat.has_value();
    VkVertexInputBindingDescription vertexBindingDesc;
    std::vector<VkVertexInputAttributeDescription> vertexAttribDescs;
    if (useVertex) {
        vertexBindingDesc = {
            .binding = 0,
            .stride = static_cast<uint32_t>(m_vertexFormat->m_stride),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX // TODO this needs to change to setup instance rendering.
        };

        uint32_t offset = 0;
        for (uint32_t i = 0; i < m_vertexFormat->m_elementsSize; i++) {
            VkVertexInputAttributeDescription desc = {
                .location = i,
                .binding = 0,
                .format = VkObjectMaps::GetTypeFormat(m_vertexFormat->m_elements[i].m_type),
                .offset = offset
            };

            vertexAttribDescs.push_back(desc);

            offset += GetRenderTypeSize(m_vertexFormat->m_elements[i].m_type);
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = useVertex ? 1u : 0u,
        .pVertexBindingDescriptions = useVertex ? &vertexBindingDesc : VK_NULL_HANDLE,
        .vertexAttributeDescriptionCount = useVertex ? static_cast<uint32_t>(vertexAttribDescs.size()) : 0u,
        .pVertexAttributeDescriptions = useVertex ? vertexAttribDescs.data() : VK_NULL_HANDLE,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport;
    if (m_viewport.has_value())
        viewport = {
            .x = m_viewport->x,
            .y = m_viewport->y,
            .width = m_viewport->width,
            .height = m_viewport->height,
            .minDepth = m_viewport->minDepth,
            .maxDepth = m_viewport->maxDepth
        };

    VkRect2D scissor;
    if (m_scissor.has_value())
        scissor = {
            .offset = {m_scissor->x, m_scissor->y},
            .extent = {m_scissor->width, m_scissor->height}
        };

    VkPipelineViewportStateCreateInfo viewportStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1, // Only need 1 viewport and scissor.
        .pViewports = m_viewport.has_value() ? &viewport : VK_NULL_HANDLE,
        .scissorCount = 1,
        .pScissors = m_scissor.has_value() ? &scissor : VK_NULL_HANDLE
    };

    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE, // Discard off-screen fragments, do not clamp.
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = m_polygonMode.has_value() ? VkObjectMaps::GetPolygonMode(m_polygonMode.value()) : VK_POLYGON_MODE_FILL,
        .cullMode = m_cullMode.has_value() ? VkObjectMaps::GetCullMode(m_cullMode.value()) : VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisamplingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = VK_NULL_HANDLE,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo = {
        .blendEnable = m_blendFunc.has_value() ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = m_blendFunc.has_value() ? VkObjectMaps::GetBlendFactor(m_blendFunc->at(0)) : VK_BLEND_FACTOR_ONE, // Optional.
        .dstColorBlendFactor = m_blendFunc.has_value() ? VkObjectMaps::GetBlendFactor(m_blendFunc->at(1)) : VK_BLEND_FACTOR_ZERO, // Optional.
        .colorBlendOp = VK_BLEND_OP_ADD, // Optional.
        .srcAlphaBlendFactor = m_blendFunc.has_value() ? VkObjectMaps::GetBlendFactor(m_blendFunc->at(2)) : VK_BLEND_FACTOR_ONE, // Optional.
        .dstAlphaBlendFactor = m_blendFunc.has_value() ? VkObjectMaps::GetBlendFactor(m_blendFunc->at(3)) : VK_BLEND_FACTOR_ZERO, // Optional.
        .alphaBlendOp = VK_BLEND_OP_ADD, // Optional.
        .colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentInfo
    };
    colorBlendingInfo.blendConstants[0] = 0.0f;
    colorBlendingInfo.blendConstants[1] = 0.0f;
    colorBlendingInfo.blendConstants[2] = 0.0f;
    colorBlendingInfo.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = VK_NULL_HANDLE,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = VK_NULL_HANDLE
    };

    result = vkCreatePipelineLayout(RenderSystem::sDevice, &pipelineLayoutInfo, VK_NULL_HANDLE, &m_layout);
    VkResultHandler::CheckResult(result, "Failed to create pipeline layout!");

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterizerInfo,
        .pMultisampleState = &multisamplingInfo,
        .pDepthStencilState = VK_NULL_HANDLE,
        .pColorBlendState = &colorBlendingInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = m_layout,
        .renderPass = RenderSystem::sRenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    result = vkCreateGraphicsPipelines(RenderSystem::sDevice, VK_NULL_HANDLE, 1, &pipelineInfo, VK_NULL_HANDLE, &m_handler);
    VkResultHandler::CheckResult(result, "Failed to create graphics pipeline!");

    vkDestroyShaderModule(RenderSystem::sDevice, fragShaderModule, VK_NULL_HANDLE);
    vkDestroyShaderModule(RenderSystem::sDevice, vertShaderModule, VK_NULL_HANDLE);
}

void GraphicsPipeline::CmdBind() {
    vkCmdBindPipeline(RenderSystem::sCommandBuffers[RenderSystem::sCurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_handler);
}

void GraphicsPipeline::Delete() {
    vkDestroyPipeline(RenderSystem::sDevice, m_handler, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(RenderSystem::sDevice, m_layout, VK_NULL_HANDLE);
}
