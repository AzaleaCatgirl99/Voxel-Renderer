#include "App.h"

#include "util/ImGUIHelper.h"
#include "util/display/RenderSystem.h"
#include "util/display/buffer/IndexBuffer.h"
#include "util/display/buffer/VertexBuffer.h"
#include "util/display/pipeline/GraphicsPipeline.h"
#include "util/display/pipeline/Type.h"
#include "util/display/pipeline/VertexFormat.h"
#include "util/Constants.h"
#include "util/display/Window.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

static const VertexFormat sTestFormat = VertexFormat()
                            .Element(RENDER_TYPE_VEC3)
                            .Element(RENDER_TYPE_VEC4);
static GraphicsPipeline sTestPipeline = GraphicsPipeline("test_vert.spv", "test_frag.spv")
                            .PolygonMode(RENDER_POLYGON_MODE_FILL)
                            .BlendFunc(RENDER_BLEND_FACTOR_ONE, RENDER_BLEND_FACTOR_DST_COLOR)
                            .CullMode(RENDER_CULL_MODE_BACK)
                            .Vertex(sTestFormat, RENDER_VERTEX_MODE_TRIANGLE_LIST);
static VertexBuffer sTestVertexBuffer = VertexBuffer(4, sTestFormat);
static IndexBuffer sTestIndexBuffer = IndexBuffer(2, RENDER_TYPE_UINT16_T);

bool App::sRunning = true;

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

        // ImGUIHelper::ProcessEvents();
    }

    // ImGUIHelper::BeginDraw();

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
    sTestPipeline.Delete();

    RenderSystem::Destroy();
    Window::Destroy();
}

int main() {
    App::Run();

    return 0;
}
