#include "util/display/device/GPUDevice.h"

#include "util/Logger.h"
#include <SDL3/SDL_vulkan.h>
#include <cstddef>

Logger GPUDevice::sLogger = Logger("GPUDevice");

void GPUDevice::Build() {
    vkGetPhysicalDeviceProperties(m_context, &m_properties);
    vkGetPhysicalDeviceFeatures(m_context, &m_features);
#ifdef SDL_PLATFORM_MACOS
    // Disables robust buffer access if the platform is macOS.
    m_features.robustBufferAccess = VK_FALSE;
#endif

    vkGetPhysicalDeviceMemoryProperties(m_context, &m_memoryProperties);

    // Gets the amount of GPUs on the system that support Vulkan.
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, VK_NULL_HANDLE);

    // Throws a runtime error if no GPUs with Vulkan support exist.
    if (count == 0)
        throw sLogger.RuntimeError("Failed to find a GPU with Vulkan support!");

    VkPhysicalDevice devices[count];
    vkEnumeratePhysicalDevices(m_instance, &count, devices);

    size_t currentBestScore = 0;
    size_t currentBestDeviceIndex = 0;
    // Goes through each device and checks whether it is the best one to use.
    for (size_t i = 0; i < count; i++) {
        size_t score = GetGPUScore(devices[i]);
        if (IsGPUUsable(devices[i]) && score > currentBestScore) {
            currentBestScore = score;
            currentBestDeviceIndex = i;
        }
    }

    // Set the physical best physical device.
    m_context = devices[currentBestDeviceIndex];

    // Find its name and log it.
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_context, &properties);
    sLogger.Info("Using physical device '", properties.deviceName, "'.");
}

bool GPUDevice::IsGPUUsable(VkPhysicalDevice gpu) {
    // Ensure all queue families are supported.
    if (!m_queueFamilyIndices.IsFull())
        return false;
    
    // Ensure all required device extensions are supported.
    if (!CheckGPUExtensionSupport(gpu))
        return false;

    // Ensure required swap chain functionality.
    GetSwapChainSupport(gpu);
    if (m_swapChainSupport.m_formats.empty() || m_swapChainSupport.m_presentModes.empty())
        return false;
    
    return true; // Device is Usable.
}

bool GPUDevice::CheckGPUExtensionSupport(VkPhysicalDevice gpu) {
    // Get extension information. Find number of extensions first for allocating the extension vector.
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(m_context, VK_NULL_HANDLE, &extensionCount, VK_NULL_HANDLE);

    VkExtensionProperties availableExtensions[extensionCount];
    vkEnumerateDeviceExtensionProperties(m_context, VK_NULL_HANDLE, &extensionCount, availableExtensions);

    // Ensure all required device extensions are supported.
    std::set<std::string> requiredExtensions(EXTENSIONS.begin(), EXTENSIONS.end());

    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

void GPUDevice::CreateQueueFamilyIndices() {
    // Gets the queue families on the device.
    vkGetPhysicalDeviceQueueFamilyProperties(m_context, &m_queueFamilyPropertiesSize, m_queueFamilyProperties);

    // Goes through all of the flags and finds the indices for them.
    for (uint32_t i = 0; i < m_queueFamilyPropertiesSize; i++) {
        if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            m_queueFamilyIndices.m_graphics = i;

        if (SDL_Vulkan_GetPresentationSupport(m_instance, m_context, i))
            m_queueFamilyIndices.m_present = i;

#ifdef SDL_PLATFORM_APPLE
        if (m_queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            m_queueFamilyIndices.m_transfer = i;
#else
        if ((m_queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && !(m_queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            m_queueFamilyIndices.m_transfer = i;
#endif

        // Breaks if everything has already been added to the indices.
        if (m_queueFamilyIndices.IsFull())
            break;
    }
}

void GPUDevice::GetSwapChainSupport(VkPhysicalDevice device) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &m_swapChainSupport.m_capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, VK_NULL_HANDLE);

    if (formatCount != 0) {
        m_swapChainSupport.m_formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, m_swapChainSupport.m_formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, VK_NULL_HANDLE);
    
    if (presentModeCount != 0) {
        m_swapChainSupport.m_presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, m_swapChainSupport.m_presentModes.data());
    }
}

size_t GPUDevice::GetGPUScore(VkPhysicalDevice gpu) {
    size_t score = 0;

    // If the GPU is discrete, the score will be added by 1000.
    if (m_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // Score is mostly determined by the maximum 2D image dimensions.
    score += m_properties.limits.maxImageDimension2D;

    return score;
}
