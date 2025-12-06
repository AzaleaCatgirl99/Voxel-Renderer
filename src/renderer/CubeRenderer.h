#pragma once

#include "util/data/Std140Calc.h"
#include <glm/glm.hpp>

class VertexFormat;
class GraphicsPipeline;
class VertexBuffer;
class IndexBuffer;
class UniformBuffer;

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
    static void Draw(const Settings& settings);
private:
    struct ModelData {
        glm::mat4 m_model;
        glm::mat4 m_view;
        glm::mat4 m_proj;
    };

    static const VertexFormat sVertexFormat;
    static constexpr const uint32_t sUniformSize = Std140Calc().PutMat4().PutMat4().PutMat4().Get();
    // static GraphicsPipeline sPipeline;
    static VertexBuffer sVBO;
    static IndexBuffer sIBO;
    static UniformBuffer sUBO;
};
