// This class only exists when debugging is enabled.
#ifdef VXL_DEBUG

#include "util/display/vulkan/VkDebugger.h"

PFN_vkCreateDebugUtilsMessengerEXT VkDebugger::pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT VkDebugger::pfnVkDestroyDebugUtilsMessengerEXT;

vk::DebugUtilsMessengerEXT VkDebugger::sMessenger;
vk::DebugUtilsMessengerCreateInfoEXT VkDebugger::sCreateInfo = {
    .messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
    .pfnUserCallback = Callback,
    .pUserData = VK_NULL_HANDLE
};
Logger VkDebugger::sLogger = Logger("VkDebugger");

void VkDebugger::Initialize(vk::InstanceCreateInfo& info) {
#ifdef VXL_VERBOSE_LOGGING
    sCreateInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
#endif
#ifdef VXL_INFO_LOGGING
    sCreateInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
#endif
#ifdef VXL_WARNING_LOGGING
    sCreateInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
#endif
#ifdef VXL_ERROR_LOGGING
    sCreateInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
#endif

    info.pNext = &sCreateInfo;
    info.enabledLayerCount = LAYER_COUNT;
    info.ppEnabledLayerNames = LAYERS;
}

void VkDebugger::CreateMessenger(vk::Instance& instance) {
    // Creates the function pointers needed for Vulkan HPP.
    pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                        instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                        instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

    sMessenger = instance.createDebugUtilsMessengerEXT(sCreateInfo);
}

void VkDebugger::Destroy(vk::Instance& instance) {
    instance.destroyDebugUtilsMessengerEXT(sMessenger);
}

bool VkDebugger::CheckValidationLayerSupport() {
    // Find the number of validation layers available.
    uint32_t layerCount;
    vk::Result result = vk::enumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE);
    if (result != vk::Result::eSuccess)
        return false;

    // Find the properties of the available validation layers.
    std::vector<vk::LayerProperties> availableLayers(layerCount);
    result = vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    if (result != vk::Result::eSuccess)
        return false;

    // Check if the wanted validation layers are available.
    for (const char* layerName : LAYERS) {
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

VKAPI_ATTR vk::Bool32 VKAPI_CALL VkDebugger::Callback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
    vk::DebugUtilsMessageTypeFlagsEXT message_type,
    const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {

    // Identify the string prefix to use depending on the message type.
    const char* messageTypeStr;
    if (message_type == vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
        messageTypeStr = "[VkDebug/GENERAL] ";
    else if (message_type == vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        messageTypeStr = "[VkDebug/VALIDATION] ";
    else if (message_type == vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        messageTypeStr = "[VkDebug/PERFORMANCE] ";
    else if (message_type == vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
        messageTypeStr = "[VkDebug/DEVICE_ADDR_BINDING] ";
    else
        messageTypeStr = "[VkDebug] ";

    // Specify the type of log depending on the severity.
    switch (message_severity) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        sLogger.Println(messageTypeStr, callback_data->pMessage);
        return VK_FALSE;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        sLogger.Println(messageTypeStr, callback_data->pMessage);
        return VK_FALSE;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        sLogger.Println(messageTypeStr, callback_data->pMessage);
        return VK_FALSE;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        sLogger.Println(messageTypeStr, callback_data->pMessage);
        return VK_TRUE;
    }
}

// These are wrappers for functions needed by Vulkan HPP.

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                const VkDebugUtilsMessengerCreateInfoEXT* info,
                                                const VkAllocationCallbacks* allocator,
                                                VkDebugUtilsMessengerEXT* messenger) {
    return VkDebugger::pfnVkCreateDebugUtilsMessengerEXT(instance, info, allocator, messenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                VkDebugUtilsMessengerEXT messenger,
                                                VkAllocationCallbacks const* allocator) {
    return VkDebugger::pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, allocator);
}

#endif
