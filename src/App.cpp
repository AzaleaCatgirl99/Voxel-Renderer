#include "App.h"

#include "renderer/Camera.h"
#include "renderer/CubeRenderer.h"
#include "util/ImGUIHelper.h"
#include "util/display/RenderSystem.h"
#include "util/display/Window.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <Test.cpp>
#include <SDL3/SDL_timer.h>

bool App::sRunning = true;
float App::sDeltaTime = 0.0f;
float App::sLastFrame = 0.0f;

void App::Run() {
    // Test();

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

    Window::Create("Voxel Renderer", mode);

    RenderSystem::Settings rendererSettings = {
        .swapInterval = RenderSystem::SwapInterval::eVSync,
        .cmdCallback = RenderLoop
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

    RenderSystem::UpdateDisplay();
}

void App::RenderLoop(vk::CommandBuffer& buffer) {
    CubeRenderer::Settings cube = {
        .m_pos = {8.0f, 0.0f, 0.0f},
        .m_rot = {0.0f, -90.0f, 0.0f}
    };
    CubeRenderer::Draw(buffer, cube);
}

void App::Cleanup() {
    RenderSystem::GetDevice().waitIdle();

    // ImGUIHelper::Destroy();

    CubeRenderer::Destroy();

    RenderSystem::Destroy();
    Window::Destroy();
}

int main() {
    App::Run();

    return 0;
}
