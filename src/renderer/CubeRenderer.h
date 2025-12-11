#pragma once

#include "util/data/Std140Calc.h"
#include "util/display/RenderSystem.h"
#include <glm/glm.hpp>
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

class VertexFormat;

// Class for testing rendering individual cubes.
class CubeRenderer final {
public:
    struct Settings {
        glm::vec3 m_pos;
        glm::vec3 m_scale = {1.0f, 1.0f, 1.0f};
        glm::vec3 m_rot;
    };

    static void Initialize();
    static void Destroy();
    static void Draw(vk::CommandBuffer& buffer, const Settings& settings);
private:
    struct ModelData {
        glm::mat4 m_model;
        glm::mat4 m_view;
        glm::mat4 m_proj;
    };

    static const VertexFormat sVertexFormat;
    static constexpr const uint32_t sUniformSize = Std140Calc().PutMat4().PutMat4().PutMat4().Get();
    static RenderSystem::Pipeline sPipeline;
    static vk::DescriptorPool sDescPool;
    static vk::DescriptorSet sDescSets[RenderSystem::MAX_FRAMES_IN_FLIGHT];
    static vk::Buffer sVBO;
    static vk::DeviceMemory sVBOMemory;
    static vk::Buffer sIBO;
    static vk::DeviceMemory sIBOMemory;
    static RenderSystem::UBO sUBO;
};
