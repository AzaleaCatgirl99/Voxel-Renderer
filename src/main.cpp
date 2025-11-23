#include <iostream>
#include "util/Constants.h"
#include "util/Window.h"

static Window* sWindow = nullptr;
static bool sRunning = true;

int main() {
    std::cout << "Princess is so cute that she might as well be a little fairy" << std::endl;

    DisplayMode mode = {
        .m_width = 1040,
		.m_height = 680,
		.m_minWidth = 840,
		.m_minHeight = 580
    };

    sWindow = new Window("Voxel Renderer", mode, RENDER_PIPELINE_VULKAN);

    while (sRunning) {
        while (sWindow->PollEvent()) {
            switch (sWindow->GetEvent()->type) {
            case SDL_EVENT_QUIT:
                sRunning = false;
                break;
            }
        }
    }

    sWindow->Destroy();

    return 0;
}
