#include "util/display/device/VkDeviceHandler.h"

#include "util/Logger.h"
#ifdef VXL_RENDERSYSTEM_DEBUG
#include "util/display/RenderSystem.h"
#endif
#include "util/display/device/SwapchainHandler.h"
#include "util/display/device/GPUDevice.h"
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_platform_defines.h>
#include <cstddef>
#include <cstdint>

Logger VkDeviceHandler::sLogger = Logger("RenderDevice");

void VkDeviceHandler::Build(vk::Instance* instance, GPUDevice* gpu) {
    
}
