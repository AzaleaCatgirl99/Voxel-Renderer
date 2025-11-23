#include "renderer/detail/VkRenderer.h"

#include <SDL3/SDL_vulkan.h>
#include <stdexcept>

namespace detail {

void VkRenderer::Initialize() {
    // Create app info struct for application info.
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    // Define application information.
    appInfo.pApplicationName = "Voxel Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);

    // Define engine information.
    appInfo.pEngineName = "Voxel Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);

    // Define API version.
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Create instance create info struct.
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo; // Link the app info struct.

    // Identify extension information.
    uint32_t sdlExtensionCount = 0;
    char const* const* sdlExtensions;

    // https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions
    sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    createInfo.enabledExtensionCount = sdlExtensionCount;
    createInfo.ppEnabledExtensionNames = sdlExtensions;

    // Set validation layers to enable
    createInfo.enabledLayerCount = 0;

    // Create the Vulkan instance.
    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!"); // TODO add logging system?
    }
}

void VkRenderer::Destroy() {
    vkDestroyInstance(m_instance, nullptr);
}

}