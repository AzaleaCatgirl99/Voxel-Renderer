#include "App.h"

#include "renderer/Camera.h"
#include "renderer/CubeRenderer.h"
#include "util/ImGUIHelper.h"
#include "util/display/RenderSystem.h"
#include "util/Constants.h"
#include "util/display/Window.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <SDL3/SDL_timer.h>

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

    CubeRenderer::Initialize();
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

    CubeRenderer::Settings cube = {
        .m_pos = {8.0f, 0.0f, 0.0f},
        .m_rot = {0.0f, -90.0f, 0.0f}
    };
    CubeRenderer::Draw(cube);

    RenderSystem::UpdateDisplay();
}

void App::Cleanup() {
    RenderSystem::WaitForDeviceIdle();

    // ImGUIHelper::Destroy();

    CubeRenderer::Destroy();

    RenderSystem::Destroy();
    Window::Destroy();
}

int main() {
    App::Run();

    return 0;
}
