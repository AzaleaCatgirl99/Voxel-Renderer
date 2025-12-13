#include "App.h"

#include "renderer/Camera.h"
#include "renderer/CubeRenderer.h"
#include "util/ImGUIHelper.h"
#include "util/display/RenderSystem.h"
#include "util/display/device/SwapchainHandler.h"
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
#ifdef VXL_TEST
    Test();
#else
    Init();

    while (sRunning)
        MainLoop();

    Cleanup();
#endif
}

void App::Init() {
    DisplayMode mode = {
        .m_width = 1040,
		.m_height = 680,
		.m_minWidth = 840,
		.m_minHeight = 580
    };

    Window::Create("Voxel Renderer", mode);

    // Set the mouse to be grabbed on boot.
    Window::SetMouseGrabbed(true);

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
    // Sets the dynamic viewport and scissor.
    vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(SwapchainHandler::sExtent.width),
        .height = static_cast<float>(SwapchainHandler::sExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    buffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor = {
        .offset = {0, 0},
        .extent = SwapchainHandler::sExtent
    };
    buffer.setScissor(0, 1, &scissor);

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
