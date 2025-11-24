#include "renderer/detail/VkRenderer.h"

#include <SDL3/SDL_vulkan.h>
#include <stdexcept>

#define DEBUG

namespace detail {

// Craete the Logger for error catching.
Logger VkRenderer::sLogger = Logger("VkRenderer");

// Set whether or not to enable validation layers.
#ifdef DEBUG
    bool VkRenderer::sEnableValidationLayers = true;
#else
    bool VkRenderer::sEnableValidationLayers = false;
#endif

// Set what validation layers to enable.
std::vector<const char*> VkRenderer::sValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

void VkRenderer::Initialize() {
    // Create the instance info.
    VkInstanceCreateInfo createInfo = CreateInstanceInfo();

    // Create the Vulkan instance.
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);

    // Catch any errors. Error codes at https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateInstance.html.
    const char* genericError = " Failed to create Vulkan instance.";
    switch (result) {
        case VK_SUCCESS:
            return; // Ran successfully.
        case VK_ERROR_EXTENSION_NOT_PRESENT: // TODO: Add optional extension functionality.
            sLogger.RuntimeError("Extension not present!", genericError);
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            sLogger.RuntimeError("Driver is not compatible!", genericError);
        case VK_ERROR_INITIALIZATION_FAILED:
            sLogger.RuntimeError("Initialization failed on Vulkan instance creation!", genericError);
        case VK_ERROR_LAYER_NOT_PRESENT:
            sLogger.RuntimeError("Layer not present!", genericError);
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            sLogger.RuntimeError("Ran out of device memory!", genericError);
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            sLogger.RuntimeError("Ran out of host memory!", genericError);
        case VK_ERROR_VALIDATION_FAILED:
            sLogger.RuntimeError("Validation failed!", genericError);
        case VK_ERROR_UNKNOWN:
            sLogger.RuntimeError("Unknown error occurred!", genericError);
        default:
            sLogger.RuntimeError("Uknown error code! Code: ", result, ",", genericError);
    }
}

VkInstanceCreateInfo VkRenderer::CreateInstanceInfo() {
    // Create the application info.
    VkApplicationInfo appInfo = CreateAppInfo();

    // Create instance into struct.
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo; // Link the app info struct.

    // Identify extension information.
    uint32_t sdlExtensionCount = 0;
    char const* const* sdlExtensions;

    // https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions
    sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    // Convert to vector so we can add more extensions.
    std::vector<const char*> requiredExtensions(sdlExtensionCount);
    for (uint32_t i = 0; i < sdlExtensionCount; i++) {
        requiredExtensions.emplace_back(sdlExtensions[i]);
    }

    // Add support for MacOS.
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    // Add extension info to instance info.
    createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // Set validation layers to enable.
    // Start with ensuring the validation layers are available.
    if (sEnableValidationLayers && !CheckValidationLayerSupport()) {
        sLogger.Error("Not all of the requested validation layers are available!");
    }

    if (sEnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(sValidationLayers.size());
        createInfo.ppEnabledLayerNames = sValidationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0; // Set to zero if validation layers are disabled.
    }

    return createInfo;
}

VkApplicationInfo VkRenderer::CreateAppInfo() {
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

    return appInfo;
}

bool VkRenderer::CheckValidationLayerSupport() {
    // Find the number of validation layers available.
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Find the properties of the available validation layers.
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Check if the wanted validation layers are available.
    for (const char* layerName : sValidationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void VkRenderer::Destroy() {
    vkDestroyInstance(m_instance, nullptr);
}

}
