#include "App.h"

#include "util/ImGUIHelper.h"
#include "util/display/RenderSystem.h"
#include "util/display/buffer/GPUBuffer.h"
#include "util/display/pipeline/GraphicsPipeline.h"
#include "util/display/pipeline/VertexFormat.h"
#include "util/Constants.h"
#include "util/display/Window.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <Test.cpp>

static const VertexFormat sTestFormat = VertexFormat()
                            .Element(RENDER_TYPE_VEC3)
                            .Element(RENDER_TYPE_VEC4);
static GraphicsPipeline sTestPipeline = GraphicsPipeline("test_vert.spv", "test_frag.spv")
                            .PolygonMode(RENDER_POLYGON_MODE_FILL)
                            .BlendFunc(RENDER_BLEND_FACTOR_ONE, RENDER_BLEND_FACTOR_DST_COLOR)
                            .CullMode(RENDER_CULL_MODE_BACK)
                            .Vertex(sTestFormat, RENDER_VERTEX_MODE_TRIANGLE_LIST);
static GPUBuffer sTestBuffer = GPUBuffer(GPU_BUFFER_SHARING_MODE_EXCLUSIVE, 21 * sizeof(float))
                            .Usage(GPU_BUFFER_USAGE_VERTEX_BUFFER);

bool App::sRunning = true;

void App::Run() {
    Test();
    
    // Init();

    // while (sRunning)
    //     MainLoop();

    // Cleanup();
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
    sTestBuffer.Build();

    float vertices[21] = {
        // Position                          Color
        0.0f, -0.75f, 0.0f,     1.0f, 0.0f, 0.0f, 1.0f,
        0.75f, 0.75f, 0.0f,     0.0f, 1.0f, 0.0f, 1.0f,
        -0.75f, 0.75f, 0.0f,     0.0f, 0.0f, 1.0f, 1.0f
    };

    sTestBuffer.Allocate(vertices, 21 * sizeof(float));
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
    sTestBuffer.CmdBind();

    RenderSystem::CmdDraw(3, 1);

    // ImGUIHelper::CmdDraw();

    RenderSystem::EndDrawFrame();
}

void App::Cleanup() {
    RenderSystem::WaitDevice();

    // ImGUIHelper::Destroy();

    sTestBuffer.Delete();
    sTestPipeline.Delete();

    RenderSystem::Destroy();
    Window::Destroy();
}

int main() {
    App::Run();

    return 0;
}
