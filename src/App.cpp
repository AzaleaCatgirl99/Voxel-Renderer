#include "App.h"

#include "renderer/Renderer.h"
#include "util/Window.h"
#include <iostream>

Window* App::sWindow = nullptr;
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

    Renderer::Settings rendererSettings;

    Renderer::CreateContext(sWindow, rendererSettings);
}

void App::MainLoop() {
    while (sWindow->PollEvent()) {
        switch (sWindow->GetEvent()->type) {
        case SDL_EVENT_QUIT:
            Close();
            break;
        }
    }
}

void App::Cleanup() {
    Renderer::DestroyContext();
    sWindow->Destroy();
}

int main() {
    App::Run();

    return 0;
}
