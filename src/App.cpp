#include "App.h"

#include <glm/trigonometric.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "renderer/Camera.h"
#include "util/ImGUIHelper.h"
#include "util/data/Std140Calc.h"
#include "util/display/RenderSystem.h"
#include "util/display/buffer/IndexBuffer.h"
#include "util/display/buffer/UniformBuffer.h"
#include "util/display/buffer/VertexBuffer.h"
#include "util/display/pipeline/GraphicsPipeline.h"
#include "util/display/pipeline/Type.h"
#include "util/display/pipeline/VertexFormat.h"
#include "util/Constants.h"
#include "util/display/Window.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <SDL3/SDL_timer.h>

struct CamTest {
    glm::mat4 m_model;
    glm::mat4 m_view;
    glm::mat4 m_proj;
};

static const VertexFormat sTestFormat = VertexFormat()
                            .Element(RENDER_TYPE_VEC3)
                            .Element(RENDER_TYPE_VEC4);
static const uint32_t sUniformSize = Std140Calc().PutMat4().PutMat4().PutMat4().Get();
static UniformBuffer sTestUniformBuffer = UniformBuffer(sUniformSize);
static GraphicsPipeline sTestPipeline = GraphicsPipeline("test_vert.spv", "test_frag.spv")
                            .PolygonMode(RENDER_POLYGON_MODE_FILL)
                            .BlendFunc(RENDER_BLEND_FACTOR_ONE, RENDER_BLEND_FACTOR_DST_COLOR)
                            .CullMode(RENDER_CULL_MODE_BACK)
                            .Vertex(sTestFormat, RENDER_VERTEX_MODE_TRIANGLE_LIST)
                            .Uniform(0, RENDER_SHADER_STAGE_VERTEX, &sTestUniformBuffer);
static VertexBuffer sTestVertexBuffer = VertexBuffer(4, sTestFormat);
static IndexBuffer sTestIndexBuffer = IndexBuffer(2, RENDER_TYPE_UINT16_T);

bool App::sRunning = true;
float App::sDeltaTime = 0.0f;
float App::sLastFrame = 0.0f;

void App::Run() {
    Init();

    while (sRunning)
        MainLoop();

    Cleanup();
}

void App::Init() {
    DisplayMode mode = {
        .m_width = 1040,
		.m_height = 680,
		.m_minWidth = 840,
		.m_minHeight = 580
    };

    Window::Create("Voxel Renderer", mode, RENDER_PIPELINE_VULKAN);

    RenderSystem::Settings rendererSettings = {
        .m_swapInterval = RENDER_SWAP_INTERVAL_VSYNC,
    };
    RenderSystem::Initialize(rendererSettings);

    // ImGUIHelper::Initialize();

    sTestUniformBuffer.Build();
    sTestPipeline.Build();
    sTestVertexBuffer.Build();
    sTestIndexBuffer.Build();

    float vertices[28] = {
        // Position                          Color
        -0.75f, -0.75f, 0.0f,     1.0f, 0.0f, 0.0f, 1.0f,
        0.75f, -0.75f, 0.0f,     0.0f, 1.0f, 0.0f, 1.0f,
        0.75f, 0.75f, 0.0f,     0.0f, 0.0f, 1.0f, 1.0f,
        -0.75f, 0.75f, 0.0f,     1.0f, 0.0f, 1.0f, 1.0f,
    };

    uint16_t indices[6] = {
        // First triangle.
        0, 1, 2,
        // Second triangle.
        2, 3, 0
    };

    sTestVertexBuffer.Allocate(vertices);
    sTestIndexBuffer.Allocate(indices);
}

void App::MainLoop() {
    while (Window::PollEvent()) {
        switch (Window::GetEvent()->type) {
        case SDL_EVENT_QUIT:
            Close();
            break;
        }

        Camera::TickEvents();

        // ImGUIHelper::ProcessEvents();
    }

    float frame = SDL_GetTicks();
    sDeltaTime = frame - sLastFrame;
    sLastFrame = frame;

    // ImGUIHelper::BeginDraw();

    Camera::Update();

    glm::mat4 model(1.0f);
    model = glm::translate(model, {8.0f, 0.0f, 0.0f});
    model = glm::rotate(model, glm::radians(-90.0f), {0.0f, 1.0f, 0.0f});
    model[1][1] *= -1;

    CamTest camTest = {
        .m_model = model,
        .m_view = Camera::GetView(),
        .m_proj = Camera::GetProj()
    };

    sTestUniformBuffer.Update(&camTest);

    RenderSystem::BeginDrawFrame();

    sTestPipeline.CmdBind();
    RenderSystem::CmdBindVertexBuffer(sTestVertexBuffer);
    RenderSystem::CmdBindIndexBuffer(sTestIndexBuffer);

    RenderSystem::CmdDrawIndexed(6, 1);

    // ImGUIHelper::CmdDraw();

    RenderSystem::EndDrawFrame();
}

void App::Cleanup() {
    RenderSystem::WaitDevice();

    // ImGUIHelper::Destroy();

    sTestVertexBuffer.Delete();
    sTestIndexBuffer.Delete();
    sTestUniformBuffer.Delete();
    sTestPipeline.Delete();

    RenderSystem::Destroy();
    Window::Destroy();
}

int main() {
    App::Run();

    return 0;
}
