#include "App.h"

#include "renderer/Renderer.h"
#include "util/Window.h"
#include <iostream>

Window* App::sWindow = nullptr;
Renderer* App::sRenderer = nullptr;
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

    sWindow = new Window("Voxel Renderer", mode, RENDER_PIPELINE_VULKAN);

    Renderer::Create(sRenderer, sWindow);
}

void App::MainLoop() {
    while (sWindow->PollEvent()) {
        switch (sWindow->GetEvent()->type) {
        case SDL_EVENT_QUIT:
            sRunning = false;
            break;
        }
    }
}

void App::Cleanup() {
    sRenderer->Destroy();
    sWindow->Destroy();
}

int main() {
    App::Run();

    return 0;
}
