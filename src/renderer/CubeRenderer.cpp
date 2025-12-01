#include "renderer/CubeRenderer.h"

#include <glm/ext.hpp>
#include "renderer/Camera.h"
#include "util/display/buffer/IndexBuffer.h"
#include "util/display/buffer/UniformBuffer.h"
#include "util/display/buffer/VertexBuffer.h"
#include "util/display/pipeline/GraphicsPipeline.h"
#include "util/display/pipeline/VertexFormat.h"

const VertexFormat CubeRenderer::sVertexFormat = VertexFormat()
                            .Element(RENDER_TYPE_VEC3)
                            .Element(RENDER_TYPE_VEC4);
GraphicsPipeline CubeRenderer::sPipeline = GraphicsPipeline("test_vert.spv", "test_frag.spv")
                            .PolygonMode(RENDER_POLYGON_MODE_FILL)
                            .BlendFunc(RENDER_BLEND_FACTOR_ONE, RENDER_BLEND_FACTOR_DST_COLOR)
                            .CullMode(RENDER_CULL_MODE_BACK)
                            .Vertex(sVertexFormat, RENDER_VERTEX_MODE_TRIANGLE_LIST)
                            .Uniform(0, RENDER_SHADER_STAGE_VERTEX, &sUBO);
VertexBuffer CubeRenderer::sVBO = VertexBuffer(24, sVertexFormat);
IndexBuffer CubeRenderer::sIBO = IndexBuffer(36, RENDER_TYPE_UINT16_T);
UniformBuffer CubeRenderer::sUBO = UniformBuffer(sUniformSize);

void CubeRenderer::Initialize() {
    sUBO.Build();
    sPipeline.Build();
    sVBO.Build();
    sIBO.Build();

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

    sVBO.Allocate(vertices);
    sIBO.Allocate(indices);
}

void CubeRenderer::Destroy() {
    sVBO.Delete();
    sIBO.Delete();
    sUBO.Delete();
    sPipeline.Delete();
}

void CubeRenderer::Draw(const Settings& settings) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, settings.m_pos);
    model = glm::rotate(model, glm::radians(settings.m_rot.x), {1.0f, 0.0f, 0.0f});
    model = glm::rotate(model, glm::radians(settings.m_rot.y), {0.0f, 1.0f, 0.0f});
    model = glm::rotate(model, glm::radians(settings.m_rot.z), {0.0f, 0.0f, 1.0f});
    model = glm::scale(model, settings.m_scale);
    model[1][1] *= -1;

    CameraUBO uboData = {
        .m_model = model,
        .m_view = Camera::GetView(),
        .m_proj = Camera::GetProj()
    };

    sUBO.Update(&uboData);

    RenderSystem::BindPipeline(sPipeline);
    RenderSystem::BindVertexBuffer(sVBO);
    RenderSystem::BindIndexBuffer(sIBO);

    RenderSystem::DrawIndexed(36, 1);
}
