#include "renderer/detail/VkRenderer.h"

#include <SDL3/SDL_vulkan.h>

namespace detail {

// The required extensions needed for the renderer.
std::vector<const char*> VkRenderer::sRequiredExtensions;

// Sets what layers to enable.
std::vector<const char*> VkRenderer::sLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Craete the Logger for error catching.
Logger VkRenderer::sLogger = Logger("VkRenderer");

// The application info used by the renderer.
VkApplicationInfo VkRenderer::sAppInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Voxel Renderer",
    .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
    .pEngineName = "Voxel Engine",
    .engineVersion = VK_MAKE_VERSION(0, 1, 0),
    .apiVersion = VK_API_VERSION_1_0
};

// The info used by the renderer's debug messenger.
VkDebugUtilsMessengerCreateInfoEXT VkRenderer::sDebugMessengerInfo = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = sEnabledSeverityFlags,
    .messageType = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = DebugCallback,
    .pUserData = VK_NULL_HANDLE
};

// The renderer's instance create info.
VkInstanceCreateInfo VkRenderer::sCreateInfo;

void VkRenderer::Initialize() {
    // Create the Vulkan instance.
    CreateInstance();

    // Set up the debug messenger for catching info from the validation layers.
    SetupDebugMessenger();
}

void VkRenderer::Destroy() {
    // Destroy debug messenger first. Doesn't exist if validation layers disabled.
    if (sEnableValidationLayers)
        DestroyDebugUtilsMessengerEXT(VK_NULL_HANDLE);

    vkDestroyInstance(m_instance, VK_NULL_HANDLE);
}

void VkRenderer::UpdateDisplay() {
    // TODO make surface stuffies.
}

void VkRenderer::CreateInstance() {
    // Sets up the required extensions.
    SetupRequiredExtensions();

    // Sets up the instance info.
    SetupInstanceInfo();

    // Create the Vulkan instance.
    VkResult result = vkCreateInstance(&sCreateInfo, VK_NULL_HANDLE, &m_instance);

    // Catch any errors. Error codes at https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateInstance.html.
    const char* genericError = "Failed to create Vulkan instance. ";
    switch (result) {
        case VK_SUCCESS:
            sLogger.Info("Successfully created the Vulkan instance.");
            break; // Ran successfully.
        case VK_ERROR_EXTENSION_NOT_PRESENT: // TODO: Add optional extension functionality.
            throw sLogger.RuntimeError(genericError, "Extension not present!");
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            throw sLogger.RuntimeError(genericError, "Driver is not compatible!");
        case VK_ERROR_INITIALIZATION_FAILED:
            throw sLogger.RuntimeError(genericError, "Initialization failed on Vulkan instance creation!");
        case VK_ERROR_LAYER_NOT_PRESENT:
            throw sLogger.RuntimeError(genericError, "Layer not present!");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            throw sLogger.RuntimeError(genericError, "Ran out of device memory!");
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            throw sLogger.RuntimeError(genericError, "Ran out of host memory!");
        case VK_ERROR_UNKNOWN:
            throw sLogger.RuntimeError(genericError, "Unknown error occurred!");
        #ifndef SDL_PLATFORM_MACOS
        case VK_ERROR_VALIDATION_FAILED: // Error code does not exist on macOS
            throw sLogger.RuntimeError(genericError, "Validation failed!");
        #endif
        default:
            throw sLogger.RuntimeError(genericError, "Uknown error code [", result, "]!");
    }
}

void VkRenderer::SetupInstanceInfo() {
    // Create instance info struct.
    sCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        .pApplicationInfo = &sAppInfo, // Link the app info struct.
        .enabledLayerCount = static_cast<uint32_t>(sLayers.size()),
        .ppEnabledLayerNames = sLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(sRequiredExtensions.size()),
        .ppEnabledExtensionNames = sRequiredExtensions.data()
    };

    // Set validation layers to enable.
    if (sEnableValidationLayers && !CheckValidationLayerSupport())
        throw sLogger.RuntimeError("Not all of the requested validation layers are available!");

    if (sEnableValidationLayers)
        sCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &sDebugMessengerInfo; // Debug info.
}

void VkRenderer::SetupRequiredExtensions() {
    // Identify extension information.
    uint32_t sdlExtensionCount = 0;
    const char* const* sdlExtensions;

    // https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions
    sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    // Convert to vector so we can add more extensions.
    sRequiredExtensions = std::vector<const char*>(sdlExtensions, sdlExtensions + sdlExtensionCount);

    // Add support for macOS.
    #ifdef SDL_PLATFORM_MACOS
    sRequiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    #endif

    // Add extension for validation layer message callback.
    if (sEnableValidationLayers)
        sRequiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

bool VkRenderer::CheckValidationLayerSupport() {
    // Find the number of validation layers available.
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Find the properties of the available validation layers.
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // Check if the wanted validation layers are available.
    for (const char* layerName : sLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkRenderer::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {

    // Identify the string prefix to use depending on the message type.
    const char* messageTypeStr;
    switch (message_type) {
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
            sLogger.Println("Receieved unexpected debugging message type [", message_type, "]");
            messageTypeStr = "[UNKNOWN TYPE] ";
            break;
    }

    // Specify the type of log depending on the severity.
    switch (message_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            sLogger.Println(messageTypeStr, callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            sLogger.Println(messageTypeStr, callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            sLogger.Println(messageTypeStr, callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            sLogger.Println(messageTypeStr, callback_data->pMessage);
            break;
        default:
            sLogger.Println("Receieved unexpected debugging severity [", message_severity, "]");
            sLogger.Println("[UNKNOWN SEVERITY] ", messageTypeStr, callback_data->pMessage); // Uknown severity, this shouldn't happen.
            break;
    }
}

void VkRenderer::SetupDebugMessenger() {
    if (!sEnableValidationLayers) return;

    // Error codes at https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/vkCreateDebugUtilsMessengerEXT.html.
    const char* genericError = " Failed to setup debug messenger.";
    VkResult result = CreateDebugUtilsMessengerEXT(&sDebugMessengerInfo, VK_NULL_HANDLE, &m_debugMessenger);
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
        #ifndef SDL_PLATFORM_MACOS // Error code doesn't exist on MacOS.
        case VK_ERROR_VALIDATION_FAILED:
            throw sLogger.RuntimeError("Validation failed!", genericError);
        #endif
        default:
            throw sLogger.RuntimeError("Unknown error code [", result, "]!", genericError);
    }
}

VkResult VkRenderer::CreateDebugUtilsMessengerEXT(
    const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debug_messenger) {

    // Loads the function with instance proc.
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");

    // Failed to load the function if it returns nullptr.
    if (func)
        return func(m_instance, create_info, allocator, debug_messenger); // Run normally.

    return VK_ERROR_EXTENSION_NOT_PRESENT; // Return error, extension not found.
}

void VkRenderer::DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* allocator) {
    // Loads the function with instance proc.
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");

    // Failed to load if it returns nullptr.
    if (func)
        func(m_instance, m_debugMessenger, allocator); // Run normally.
    else
        sLogger.Error("Unable to load vkDestroyDebugUtilsMessengerEXT! Failed to destroy debug messenger.");
}

}
