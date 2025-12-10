#include "renderer/CubeRenderer.h"

#include <SDL3/SDL_filesystem.h>
#include <cstddef>
#include <cstdio>
#include <glm/ext.hpp>
#include <string>
#include "renderer/Camera.h"
#include "util/display/RenderSystem.h"
#include "util/display/pipeline/Type.h"
#include "util/display/pipeline/VertexFormat.h"

const VertexFormat CubeRenderer::sVertexFormat = VertexFormat()
                            .Element(DataType::eVec3)
                            .Element(DataType::eVec4);
RenderSystem::Pipeline CubeRenderer::sPipeline;
vk::DescriptorPool CubeRenderer::sDescPool;
vk::DescriptorSet CubeRenderer::sDescSets[RenderSystem::MAX_FRAMES_IN_FLIGHT];
vk::Buffer CubeRenderer::sVBO;
vk::DeviceMemory CubeRenderer::sVBOMemory;
vk::Buffer CubeRenderer::sIBO;
vk::DeviceMemory CubeRenderer::sIBOMemory;
RenderSystem::UBO CubeRenderer::sUBO;

void CubeRenderer::Initialize() {
    sUBO = RenderSystem::CreateUniformBuffer(sUniformSize);

    vk::DynamicState states[2] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    vk::DescriptorSetLayoutBinding bindings[1];
    bindings[0] = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex
    };

    RenderSystem::Pipeline::Info info = {
        .vertexShaderPath = std::string(SDL_GetBasePath()) + "assets/shaders/test_vert.spv",
        .fragmentShaderPath = std::string(SDL_GetBasePath()) + "assets/shaders/test_frag.spv",
        .topology = vk::PrimitiveTopology::eTriangleList,
        .blending = true,
        .colorSrcFactor = vk::BlendFactor::eOne,
        .colorDstFactor = vk::BlendFactor::eDstColor,
        .alphaSrcFactor = vk::BlendFactor::eOne,
        .alphaDstFactor = vk::BlendFactor::eDstAlpha,
        .cullMode = vk::CullModeFlagBits::eBack,
        .useVertexFormat = true,
        .vertexFormat = &sVertexFormat,
        .bindings = bindings,
        .bindingCount = 1,
        .dynamicStates = states,
        .dynamicStateCount = 2
    };

    sPipeline = RenderSystem::CreatePipeline(info);

    vk::DescriptorPoolSize poolSizes[1];
    poolSizes[0] = {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = RenderSystem::MAX_FRAMES_IN_FLIGHT
    };

    sDescPool = RenderSystem::CreateDescriptorPool(poolSizes, 1, RenderSystem::MAX_FRAMES_IN_FLIGHT);
    vk::DescriptorSetLayout layouts[RenderSystem::MAX_FRAMES_IN_FLIGHT] = {sPipeline.descriptorSetLayout, sPipeline.descriptorSetLayout};
    RenderSystem::CreateDescriptorSets(sDescSets, RenderSystem::MAX_FRAMES_IN_FLIGHT, sDescPool, layouts);

    vk::WriteDescriptorSet descriptorWrites[RenderSystem::MAX_FRAMES_IN_FLIGHT];
    vk::DescriptorBufferInfo bufferInfos[RenderSystem::MAX_FRAMES_IN_FLIGHT];
    for (size_t i = 0; i < RenderSystem::MAX_FRAMES_IN_FLIGHT; i++) {
        bufferInfos[i] = {
            .buffer = sUBO.buffers[i],
            .offset = 0,
            .range = sUniformSize
        };

        descriptorWrites[i] = {
            .dstSet = sDescSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfos[i]
        };
    }

    RenderSystem::GetDevice().updateDescriptorSets(RenderSystem::MAX_FRAMES_IN_FLIGHT, descriptorWrites, 0, VK_NULL_HANDLE);

    sVBO = RenderSystem::CreateVertexBuffer(24, sVertexFormat);
    sVBOMemory = RenderSystem::CreateMemory(sVBO, vk::MemoryPropertyFlagBits::eDeviceLocal);

    sIBO = RenderSystem::CreateIndexBuffer(36);
    sIBOMemory = RenderSystem::CreateMemory(sIBO, vk::MemoryPropertyFlagBits::eDeviceLocal);

    float begin = -1.0f;
    float end = 1.0f;

    float vertices[168] = {
        // Position                          Color
        // Front face
        begin, begin, end,     1.0f, 0.0f, 0.0f, 1.0f,
        end, begin, end,     0.0f, 1.0f, 0.0f, 1.0f,
        end, end, end,     0.0f, 0.0f, 1.0f, 1.0f,
        begin, end, end,     1.0f, 0.0f, 1.0f, 1.0f,

        // Back face
        begin, begin, begin,     1.0f, 0.0f, 0.0f, 1.0f,
        end, begin, begin,     0.0f, 1.0f, 0.0f, 1.0f,
        end, end, begin,     0.0f, 0.0f, 1.0f, 1.0f,
        begin, end, begin,     1.0f, 0.0f, 1.0f, 1.0f,

        // Left face
        begin, begin, begin,     1.0f, 0.0f, 0.0f, 1.0f,
        begin, begin, end,     0.0f, 1.0f, 0.0f, 1.0f,
        begin, end, end,     0.0f, 0.0f, 1.0f, 1.0f,
        begin, end, begin,     1.0f, 0.0f, 1.0f, 1.0f,

        // Right face
        end, begin, begin,     1.0f, 0.0f, 0.0f, 1.0f,
        end, begin, end,     0.0f, 1.0f, 0.0f, 1.0f,
        end, end, end,     0.0f, 0.0f, 1.0f, 1.0f,
        end, end, begin,     1.0f, 0.0f, 1.0f, 1.0f,

        // Top face
        begin, end, begin,     1.0f, 0.0f, 0.0f, 1.0f,
        end, end, begin,     0.0f, 1.0f, 0.0f, 1.0f,
        end, end, end,     0.0f, 0.0f, 1.0f, 1.0f,
        begin, end, end,     1.0f, 0.0f, 1.0f, 1.0f,

        // Bottom face
        begin, begin, begin,     1.0f, 0.0f, 0.0f, 1.0f,
        end, begin, begin,     0.0f, 1.0f, 0.0f, 1.0f,
        end, begin, end,     0.0f, 0.0f, 1.0f, 1.0f,
        begin, begin, end,     1.0f, 0.0f, 1.0f, 1.0f
    };

    RenderSystem::AllocateStagedMemory(sVBO, sVBOMemory, vertices, 168 * sizeof(float));

    uint16_t indices[36] = {
        // Front face
        0, 1, 2,
        2, 3, 0,

        // Back face
        4, 5, 6,
        6, 7, 4,

        // Left face
        8, 9, 10,
        10, 11, 8,

        // Right face
        12, 13, 14,
        14, 15, 12,

        // Top face
        16, 17, 18,
        18, 19, 16,

        // Bottom face
        20, 21, 22,
        22, 23, 20
    };

    RenderSystem::AllocateStagedMemory(sIBO, sIBOMemory, indices, 36 * sizeof(uint16_t));
}

void CubeRenderer::Destroy() {
    RenderSystem::GetDevice().freeMemory(sVBOMemory);
    RenderSystem::GetDevice().freeMemory(sIBOMemory);

    RenderSystem::GetDevice().destroyBuffer(sVBO);
    RenderSystem::GetDevice().destroyBuffer(sIBO);
    sUBO.Destroy();

    RenderSystem::GetDevice().destroyDescriptorPool(sDescPool);

    sPipeline.Destroy();
}

void CubeRenderer::Draw(vk::CommandBuffer* buffer, const Settings& settings) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, settings.m_pos);
    model = glm::rotate(model, glm::radians(settings.m_rot.x), {1.0f, 0.0f, 0.0f});
    model = glm::rotate(model, glm::radians(settings.m_rot.y), {0.0f, 1.0f, 0.0f});
    model = glm::rotate(model, glm::radians(settings.m_rot.z), {0.0f, 0.0f, 1.0f});
    model = glm::scale(model, settings.m_scale);
    model[1][1] *= -1;

    ModelData data = {
        .m_model = model,
        .m_view = Camera::GetView(),
        .m_proj = Camera::GetProj()
    };

    sUBO.Update(&data);

    buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, sPipeline.layout, 0, 1, &sDescSets[RenderSystem::GetCurrentFrame()], 0, VK_NULL_HANDLE);
    buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, sPipeline);
    vk::DeviceSize offset = 0;
    buffer->bindVertexBuffers(0, 1, &sVBO, &offset);
    buffer->bindIndexBuffer(sIBO, 0, vk::IndexType::eUint16);

    buffer->drawIndexed(36, 1, 0, 0, 0);
}
