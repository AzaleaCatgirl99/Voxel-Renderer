#include "App.h"

#include "renderer/Renderer.h"
#include "renderer/pipeline/GraphicsPipeline.h"
#include "util/Constants.h"
#include "util/Window.h"
#include <iostream>

static const GraphicsPipeline sTestPipeline = GraphicsPipeline("test", "test_vert.spv", "test_frag.spv")
                            // .PolygonMode(RENDER_POLYGON_MODE_LINE)
                            .BlendFunc(RENDER_BLEND_FACTOR_ONE, RENDER_BLEND_FACTOR_DST_COLOR)
                            .CullMode(RENDER_CULL_MODE_BACK);

bool App::sRunning = true;

void App::Run() {
    Init();

    while (sRunning)
        MainLoop();

    Cleanup();
}

void App::Init() {
    std::cout << "Princess is so cute that she might as well be a little fairy" << std::endl;

    DisplayMode mode = {
        .m_width = 1040,
		.m_height = 680,
		.m_minWidth = 840,
		.m_minHeight = 580
    };

    Window::Create("Voxel Renderer", mode, RENDER_PIPELINE_VULKAN);

    Renderer::Settings rendererSettings = {
        .m_defaultSwapInterval = RENDER_SWAP_INTERVAL_VSYNC
    };

    Renderer::CreateContext(rendererSettings);

    Renderer::RegisterPipeline(sTestPipeline);
}

void App::MainLoop() {
    while (Window::PollEvent()) {
        switch (Window::GetEvent()->type) {
        case SDL_EVENT_QUIT:
            Close();
            break;
        }
    }

    Renderer::BeginDrawFrame();

    Renderer::CmdBindPipeline(sTestPipeline);
    Renderer::CmdDraw(3, 1);

    Renderer::EndDrawFrame();
    Renderer::UpdateDisplay();
}

void App::Cleanup() {
    Renderer::DestroyContext();
    Window::Destroy();
}

int main() {
    App::Run();

    return 0;
}
