#include "util/display/pipeline/GraphicsPipeline.h"

#include "util/BufferedFile.h"
#include "util/display/RenderSystem.h"
#include "util/display/vulkan/VkResultHandler.h"
#include "util/display/vulkan/VkObjectMaps.h"
#include <SDL3/SDL_filesystem.h>
#include <cstddef>
#include <cstdint>
#include "util/display/buffer/UniformBuffer.h"

void GraphicsPipeline::Build() {
    VkShaderModule vertShaderModule = CreateShader(m_info.vertexShaderPath.c_str());
    VkShaderModule fragShaderModule = CreateShader(m_info.fragmentShaderPath.c_str());

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

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = m_info.dynamicStateCount,
        .pDynamicStates = m_info.dynamicStates
    };

    // Checks whether the pipeline has a vertex format. If it does, then it'll setup the binding and attribute descriptions.
    VkVertexInputBindingDescription vertexBindingDesc;
    std::vector<VkVertexInputAttributeDescription> vertexAttribDescs;
    if (m_info.useVertexFormat) {
        vertexBindingDesc = {
            .binding = 0,
            .stride = static_cast<uint32_t>(m_info.vertexFormat->m_stride),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX // TODO this needs to change to setup instance rendering.
        };

        uint32_t offset = 0;
        for (uint32_t i = 0; i < m_info.vertexFormat->m_elementsSize; i++) {
            VkVertexInputAttributeDescription desc = {
                .location = i,
                .binding = 0,
                .format = VkObjectMaps::GetTypeFormat(m_info.vertexFormat->m_elements[i].m_type),
                .offset = offset
            };

            vertexAttribDescs.push_back(desc);

            offset += GetRenderTypeSize(m_info.vertexFormat->m_elements[i].m_type);
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = m_info.useVertexFormat ? 1u : 0u,
        .pVertexBindingDescriptions = m_info.useVertexFormat ? &vertexBindingDesc : VK_NULL_HANDLE,
        .vertexAttributeDescriptionCount = m_info.useVertexFormat ? static_cast<uint32_t>(vertexAttribDescs.size()) : 0u,
        .pVertexAttributeDescriptions = m_info.useVertexFormat ? vertexAttribDescs.data() : VK_NULL_HANDLE,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineViewportStateCreateInfo viewportStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1, // Only need 1 viewport and scissor.
        .pViewports = &m_info.viewport,
        .scissorCount = 1,
        .pScissors = &m_info.scissor
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

    auto colorBlendAttachmentInfo = CreateColorInfo();
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

    VkResult result;
    if (m_info.descriptorCount > 0) {
        VkDescriptorSetLayoutBinding bindings[m_info.descriptorCount];

        for (uint32_t i = 0; i < m_info.descriptorCount; i++) {
            bindings[i] = {
                .binding = m_info.descriptors[i].binding,
                .descriptorType = m_info.descriptors[i].type,
                .descriptorCount = 1,
                .stageFlags = m_info.descriptors[i].stage,
                .pImmutableSamplers = VK_NULL_HANDLE
            };
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = m_info.descriptorCount,
            .pBindings = bindings
        };

        result = vkCreateDescriptorSetLayout(RenderSystem::sDevice, &layoutInfo, VK_NULL_HANDLE, &m_descriptorSetLayout);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = m_info.descriptorCount > 0 ? 1u : 0u,
        .pSetLayouts = m_info.descriptorCount > 0 ? &m_descriptorSetLayout : VK_NULL_HANDLE,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = VK_NULL_HANDLE
    };

    result = vkCreatePipelineLayout(RenderSystem::sDevice, &pipelineLayoutInfo, VK_NULL_HANDLE, &m_layout);
    VkResultHandler::CheckResult(result, "Failed to create pipeline layout!");

    auto rasterizerInfo = CreateRasterizationInfo();

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

    if (m_info.descriptorCount > 0) {
        VkDescriptorPoolSize sizes[m_info.descriptorCount];

        for (uint32_t i = 0; i < m_info.descriptorCount; i++) {
            sizes[i] = {
                .type = m_info.descriptors[i].type,
                .descriptorCount = static_cast<uint32_t>(RenderSystem::MAX_FRAMES_IN_FLIGHT)
            };
        }

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = RenderSystem::MAX_FRAMES_IN_FLIGHT,
            .poolSizeCount = m_info.descriptorCount,
            .pPoolSizes = sizes
        };

        VkResult result = vkCreateDescriptorPool(RenderSystem::sDevice, &poolInfo, VK_NULL_HANDLE, &m_descriptorPool);
        VkResultHandler::CheckResult(result, "Failed to create descriptor pool!");

        VkDescriptorSetLayout layouts[RenderSystem::MAX_FRAMES_IN_FLIGHT] = {m_descriptorSetLayout, m_descriptorSetLayout};
        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = RenderSystem::MAX_FRAMES_IN_FLIGHT,
            .pSetLayouts = layouts
        };

        // for (uint32_t i = 0; i < m_info.descriptorCount; i++) {
        //     m_descriptorSets.try_emplace(m_info.descriptors[i].binding);

        //     result = vkAllocateDescriptorSets(RenderSystem::sDevice, &allocInfo, m_descriptorSets[m_info.descriptors[i].binding].data());
        //     VkResultHandler::CheckResult(result, "Failed to allocate descriptor sets!");
        // }
    }
}

void GraphicsPipeline::UpdateUniformDescSet(uint32_t binding, UniformBuffer& buffer) {
    if (m_info.descriptorCount == 0)
        return;

    VkBuffer gpuBuffers[RenderSystem::MAX_FRAMES_IN_FLIGHT] = {buffer.m_buffers[0].m_handler, buffer.m_buffers[1].m_handler};

    for (size_t i = 0; i < RenderSystem::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = gpuBuffers[i],
            .offset = 0,
            .range = VK_WHOLE_SIZE
        };

        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[binding][i],
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = VK_NULL_HANDLE,
            .pBufferInfo = &bufferInfo,
            .pTexelBufferView = VK_NULL_HANDLE
        };

        vkUpdateDescriptorSets(RenderSystem::sDevice, 1, &descriptorWrite, 0, VK_NULL_HANDLE);
    }
}

VkShaderModule GraphicsPipeline::CreateShader(const char* path) {
    BufferedFile file = BufferedFile::Read(path, true);

    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = file.Size(),
        .pCode = file.DataAsUInt32()
    };

    VkShaderModule module;
    VkResult result = vkCreateShaderModule(RenderSystem::sDevice, &info, VK_NULL_HANDLE, &module);
    VkResultHandler::CheckResult(result, "Failed to create shader module!");

    return module;
}

void GraphicsPipeline::CmdBind() {
    // if (m_info.descriptorCount == 0)
    //     vkCmdBindDescriptorSets(RenderSystem::sCommandBuffers[RenderSystem::sCurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_layout, 0, 1, &m_descriptorSets[RenderSystem::sCurrentFrame], 0, VK_NULL_HANDLE);

    vkCmdBindPipeline(RenderSystem::sCommandBuffers[RenderSystem::sCurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_handler);
}

void GraphicsPipeline::Delete() {
    // if (m_layoutBindingsCount > 0) {
    //     vkDestroyDescriptorPool(RenderSystem::sDevice, m_descriptorPool, VK_NULL_HANDLE);
    //     vkDestroyDescriptorSetLayout(RenderSystem::sDevice, m_descriptorSetLayout, VK_NULL_HANDLE);
    // }

    vkDestroyPipeline(RenderSystem::sDevice, m_handler, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(RenderSystem::sDevice, m_layout, VK_NULL_HANDLE);
}
