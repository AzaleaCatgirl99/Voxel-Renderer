#include "renderer/detail/VkRenderer.h"

#include <SDL3/SDL_vulkan.h>
#include <cstddef>
#include <cstdint>
#include <vector>

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

// The renderer's instance create info.
VkInstanceCreateInfo VkRenderer::sCreateInfo;

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

void VkRenderer::Initialize() {
    // Create the Vulkan instance.
    CreateInstance();

    // Set up the debug messenger for catching info from the validation layers.
    SetupDebugMessenger();

    // Selects the best GPU to use.
    SelectBestPhysicalDevice();

    // Create the logical device.
    CreateLogicalDevice();
}

void VkRenderer::Destroy() {
    vkDestroyDevice(m_logicalDevice, nullptr);

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
    if (result != VK_SUCCESS)
        throw InterpretVkError(result, "Failed to create Vulkan instance.");
    sLogger.Info("Created the Vulkan instance.");
}

void VkRenderer::SetupInstanceInfo() {
    // Create instance info struct.
    sCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
#ifdef SDL_PLATFORM_MACOS
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR, // macOS support flag
#endif
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
            messageTypeStr = "[VkDebug/GENERAL] ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            messageTypeStr = "[VkDebug/VALIDATION] ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            messageTypeStr = "[VkDebug/PERFORMANCE] ";
            break;
        default:
            sLogger.Println("Receieved unexpected debugging message type [", message_type, "]");
            messageTypeStr = "[VkDebug/UNKNOWN] ";
            break;
    }

    // Specify the type of log depending on the severity.
    switch (message_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            sLogger.Println(messageTypeStr, callback_data->pMessage);
            return VK_FALSE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            sLogger.Println(messageTypeStr, callback_data->pMessage);
            return VK_FALSE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            sLogger.Println(messageTypeStr, callback_data->pMessage);
            return VK_FALSE;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            sLogger.Println(messageTypeStr, callback_data->pMessage);
            return VK_TRUE;
        default:
            sLogger.Println("Receieved unexpected debugging severity [", message_severity, "]");
            sLogger.Println("[UNKNOWN SEVERITY] ", messageTypeStr, callback_data->pMessage); // Unknown severity, this shouldn't happen.
            return VK_FALSE;
    }
}

void VkRenderer::SetupDebugMessenger() {
    if (!sEnableValidationLayers) return;

    VkResult result = CreateDebugUtilsMessengerEXT(&sDebugMessengerInfo, VK_NULL_HANDLE, &m_debugMessenger);
    if (result != VK_SUCCESS)
        throw InterpretVkError(result, "Failed to create the debug messenger.");
    sLogger.Info("Created the debug messenger.");
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

void VkRenderer::SelectBestPhysicalDevice() {
    // Gets the amount of GPUs on the system that support Vulkan.
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, VK_NULL_HANDLE);

    // Throws a runtime error if no GPUs with Vulkan support exist.
    if (count == 0)
        throw sLogger.RuntimeError("Failed to find a GPU with Vulkan support!");

    // Gets the device handlers.
    VkPhysicalDevice devices[count];
    vkEnumeratePhysicalDevices(m_instance, &count, devices);

    size_t currentBestScore = 0;
    size_t currentBestDeviceIndex = 0;
    // Goes through each device and checks whether it is the best one to use.
    for (size_t i = 0; i < count; i++) {
        size_t score = GetPhysicalDeviceScore(devices[i]);
        if (IsPhysicalDeviceUsable(devices[i]) && score > currentBestScore) {
            currentBestScore = score;
            currentBestDeviceIndex = i;
        }
    }

    m_physicalDevice = devices[currentBestDeviceIndex];
}

bool VkRenderer::IsPhysicalDeviceUsable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = GetQueueFamilies(device);

    return indices.HasEverything();
}

size_t VkRenderer::GetPhysicalDeviceScore(VkPhysicalDevice device) {
    size_t score = 0;

    // Gets the device's properties.
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    // Gets the device's features.
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    // If the GPU is discrete, the score will be added by 1000.
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // Score is mostly determined by the maximum 2D image dimensions.
    score += properties.limits.maxImageDimension2D;

    return score;
}

VkRenderer::QueueFamilyIndices VkRenderer::GetQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    // Gets the amount of queue families on the device.
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, VK_NULL_HANDLE);

    // Gets the queue families on the device.
    VkQueueFamilyProperties families[count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families);

    // Goes through all of the flags and finds the indices for them.
    for (uint32_t i = 0; i < count; i++) {
        if (families[count].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.m_graphics = i;

        // Breaks if everything has already been added to the indices.
        if (indices.HasEverything())
            break;
    }

    return indices;
}

void VkRenderer::CreateLogicalDevice() {
    // Find the indices of the queue families.
    QueueFamilyIndices indices = GetQueueFamilies(m_physicalDevice);

    // Ensure graphics is not empty.
    if (!indices.m_graphics.has_value())
        throw sLogger.RuntimeError("No graphics family found! Failed to create the logical device.");

    // Create the graphics queue info struct.
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo graphicsQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = indices.m_graphics.value(),
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    // Create a blank device features since we do not need any special support.
    VkPhysicalDeviceFeatures deviceFeatures{};

    // Create the info struct for the logical device.
    VkDeviceCreateInfo logicalDeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &graphicsQueueCreateInfo, // Link the graphics queue.
        .enabledExtensionCount = 0,
        .pEnabledFeatures = &deviceFeatures // Link the device features.
    };

    // Enable device-specific validation layer support for older implementations.
    if (sEnableValidationLayers) {
        logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(sLayers.size());
        logicalDeviceCreateInfo.ppEnabledLayerNames = sLayers.data();
    } else {
        logicalDeviceCreateInfo.enabledLayerCount = 0;
    }

    // Create the logical device.
    VkResult result = vkCreateDevice(m_physicalDevice, &logicalDeviceCreateInfo, VK_NULL_HANDLE, &m_logicalDevice);
    if (result != VK_SUCCESS)
        throw InterpretVkError(result, "Failed to create the logical device.");
    sLogger.Info("Created the logical device.");

    // Get the graphics queue handle.
    vkGetDeviceQueue(m_logicalDevice, indices.m_graphics.value(), 0, &m_graphicsQueue);
}

std::runtime_error VkRenderer::InterpretVkError(VkResult result, const char* genericError) {
    // Supports:
    // Instance - https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateInstance.html
    // Debug messenger - https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/vkCreateDebugUtilsMessengerEXT.html
    // Device - https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateDevice.html
    switch(result) {
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return sLogger.RuntimeError("Extension not present! ", genericError);
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return sLogger.RuntimeError("Driver is not compatible! ", genericError);
        case VK_ERROR_INITIALIZATION_FAILED:
            return sLogger.RuntimeError("Initialization failed! ", genericError);
        case VK_ERROR_LAYER_NOT_PRESENT:
            return sLogger.RuntimeError("Validation layer is not present! ", genericError);
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return sLogger.RuntimeError("Ran out of device memory! ", genericError);
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return sLogger.RuntimeError("Ran out of host memory! ", genericError);
        case VK_ERROR_DEVICE_LOST:
            return sLogger.RuntimeError("Device lost! ", genericError);
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return sLogger.RuntimeError("Feature not present! ", genericError);
        case VK_ERROR_TOO_MANY_OBJECTS:
            return sLogger.RuntimeError("Too many objects! ", genericError);
        case VK_ERROR_UNKNOWN:
            return sLogger.RuntimeError("Unknown error occurred! ", genericError);
        #ifndef SDL_PLATFORM_MACOS
        case VK_ERROR_VALIDATION_FAILED: // Error code does not exist on macOS
            return sLogger.RuntimeError("Validation failed! ", genericError);
        #endif
        default:
            return sLogger.RuntimeError("Uknown error code [", result, "]! ", genericError);
    }
}

}
