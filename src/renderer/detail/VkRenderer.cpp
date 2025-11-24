#include "renderer/detail/VkRenderer.h"

#include <SDL3/SDL_vulkan.h>
#include <stdexcept>

namespace detail {

// Craete the Logger for error catching.
Logger VkRenderer::sLogger = Logger("VkRenderer");

// Set what validation layers to enable.
std::vector<const char*> VkRenderer::sValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

void VkRenderer::Initialize() {
    // Create the Vulkan instance.
    CreateInstance();

    // Set up the debug messenger for catching info from the validation layers.
    SetupDebugMessenger();
}

void VkRenderer::CreateInstance() {
    // Create the instance info.
    VkInstanceCreateInfo createInfo = CreateInstanceInfo();

    // Create the Vulkan instance.
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);

    // Catch any errors. Error codes at https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateInstance.html.
    const char* genericError = " Failed to create Vulkan instance.";
    switch (result) {
        case VK_SUCCESS:
            sLogger.Info("Successfully created the Vulkan instance.");
            break; // Ran successfully.
        case VK_ERROR_EXTENSION_NOT_PRESENT: // TODO: Add optional extension functionality.
            throw sLogger.RuntimeError("Extension not present!", genericError);
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            throw sLogger.RuntimeError("Driver is not compatible!", genericError);
        case VK_ERROR_INITIALIZATION_FAILED:
            throw sLogger.RuntimeError("Initialization failed on Vulkan instance creation!", genericError);
        case VK_ERROR_LAYER_NOT_PRESENT:
            throw sLogger.RuntimeError("Layer not present!", genericError);
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            throw sLogger.RuntimeError("Ran out of device memory!", genericError);
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            throw sLogger.RuntimeError("Ran out of host memory!", genericError);
        case VK_ERROR_UNKNOWN:
            throw sLogger.RuntimeError("Unknown error occurred!", genericError);
        #ifndef SDL_PLATFORM_MACOS
        case VK_ERROR_VALIDATION_FAILED: // Error code does not exist on Macos
            throw sLogger.RuntimeError("Validation failed!", genericError);
        #endif
        default:
            throw sLogger.RuntimeError("Uknown error code! Code: ", result, ",", genericError);
    }
}

VkInstanceCreateInfo VkRenderer::CreateInstanceInfo() {
    // Create the application info.
    VkApplicationInfo appInfo = CreateAppInfo();

    // Create instance info struct.
    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo // Link the app info struct.
    };

    // Add extension info to instance info.
    std::vector<const char*> requiredExtensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // Set validation layers to enable.
    // Start with ensuring the validation layers are available.
    if (sEnableValidationLayers && !CheckValidationLayerSupport()) {
        throw sLogger.RuntimeError("Not all of the requested validation layers are available!");
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
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Voxel Renderer",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "Voxel Engine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_0
    };
    return appInfo;
}

std::vector<const char*> VkRenderer::GetRequiredExtensions() {
    // Identify extension information.
    uint32_t sdlExtensionCount = 0;
    char const* const* sdlExtensions;

    // https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions
    sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    // Convert to vector so we can add more extensions.
    std::vector<const char*> requiredExtensions(sdlExtensions, sdlExtensions + sdlExtensionCount);

    // Add support for MacOS.
    #ifdef SDL_PLATFORM_MACOS
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    // Add extension for validation layer message callback.
    if (sEnableValidationLayers) {
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return requiredExtensions;
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

VKAPI_ATTR VkBool32 VKAPI_CALL VkRenderer::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT pMessageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT pMessageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    
    if (pMessageSeverity & sEnabledSeverityFlags == 0) {
        return;
    }

    // This can be better written, make a 3 length static indexable array.
    char* messageTypeStr;
    switch (pMessageType) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            messageTypeStr = "[GENERAL] ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            messageTypeStr = "[VALIDATION] ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            messageTypeStr = "[PERFORMANCE] ";
            break;
        default:
            messageTypeStr = "[TYPE UNKNOWN] ";
            break;
    }

    switch (pMessageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            sLogger.Verbose(messageTypeStr, pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            sLogger.Info(messageTypeStr, pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            sLogger.Warning(messageTypeStr, pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            sLogger.Error(messageTypeStr, pCallbackData->pMessage);
            break;
        default:
            sLogger.Error("[SEVERITY UNKNOWN]", messageTypeStr, pCallbackData->pMessage);
            break;
    }
}

void VkRenderer::SetupDebugMessenger() {
    if (!sEnableValidationLayers) return;

    // Create the info struct for the debug messenger.
    VkDebugUtilsMessengerCreateInfoEXT createInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = sEnabledSeverityFlags,
        .messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = VkRenderer::DebugCallback,
        .pUserData = nullptr
    };

    // Error codes at https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/vkCreateDebugUtilsMessengerEXT.html.
    const char* genericError = " Failed to setup debug messenger.";
    VkResult result = CreateDebugUtilsMessengerEXT(&createInfo, nullptr, &m_debugMessenger);
    switch (result) {
        case VK_SUCCESS:
            sLogger.Info("Successfully created the debug messenger.");
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            throw sLogger.RuntimeError("Ran out of host memory!", genericError);
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            throw sLogger.RuntimeError("Failed to find extension!", genericError);
        case VK_ERROR_UNKNOWN:
            throw sLogger.RuntimeError("Unknown error!", genericError);
        #ifndef SDL_PLATFORM_MACOS
        case VK_ERROR_VALIDATION_FAILED:
            throw sLogger.RuntimeError("Validation failed!", genericError);
        #endif
        default:
            throw sLogger.RuntimeError("Unknown error code!", genericError);
    }
}

VkResult VkRenderer::CreateDebugUtilsMessengerEXT(
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(m_instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VkRenderer::DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(m_instance, m_debugMessenger, pAllocator);
    }
}

void VkRenderer::Destroy() {
    if (sEnableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(nullptr);
    }

    vkDestroyInstance(m_instance, nullptr);

    // TODO Destroy window

    // TODO Terminate SDL. These are handled by the Renderer?
}

}
