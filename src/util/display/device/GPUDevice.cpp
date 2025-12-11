#include "util/display/device/GPUDevice.h"

#include "util/Logger.h"
#include <SDL3/SDL_vulkan.h>
#include <cstddef>
#include <vector>

Logger GPUDevice::sLogger = Logger("GPUDevice");

void GPUDevice::Build(vk::Instance& instance, vk::SurfaceKHR& surface) {
    // Gets the amount of GPUs on the system that support Vulkan.
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    size_t currentBestScore = 0;
    size_t currentBestDeviceIndex = 0;
    // Goes through each device and checks whether it is the best one to use.
    for (size_t i = 0; i < devices.size(); i++) {
        size_t score = GetGPUScore(devices[i]);
        if (IsGPUUsable(devices[i], instance, surface) && score > currentBestScore) {
            currentBestScore = score;
            currentBestDeviceIndex = i;
        }
    }

    // Sets the best physical device.
    device = devices[currentBestDeviceIndex];

    // Reload these once the correct device has been set.
    GetSwapchainSupport(device, surface);
    CreateQueueFamilyIndices(device, instance);

    m_properties = device.getProperties();
    m_features = device.getFeatures();
#ifdef SDL_PLATFORM_APPLE
    // Disables robust buffer access if the platform is an Apple device.
    m_features.robustBufferAccess = VK_FALSE;
#endif

    m_memoryProperties = device.getMemoryProperties();

    // Find its name and log it.
    sLogger.Info("Using physical device '", m_properties.deviceName, "'.");
}

bool GPUDevice::IsGPUUsable(vk::PhysicalDevice& gpu, vk::Instance& instance, vk::SurfaceKHR& surface) {
    // Ensure all queue families are supported.
    CreateQueueFamilyIndices(gpu, instance);
    if (!m_queueFamilyIndices.IsFull())
        return false;
    
    // Ensure all required device extensions are supported.
    if (!CheckGPUExtensionSupport(gpu))
        return false;

    // Ensure required swap chain functionality.
    GetSwapchainSupport(gpu, surface);
    if (m_swapchainSupport.formats.empty() || m_swapchainSupport.presentModes.empty())
        return false;
    
    return true; // Device is Usable.
}

bool GPUDevice::CheckGPUExtensionSupport(vk::PhysicalDevice& gpu) {
    // Get extension information.
    std::vector<vk::ExtensionProperties> availableExtensions = gpu.enumerateDeviceExtensionProperties();

    // Ensure all required device extensions are supported.
    std::set<std::string> requiredExtensions(EXTENSIONS.begin(), EXTENSIONS.end());

    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

void GPUDevice::CreateQueueFamilyIndices(vk::PhysicalDevice& gpu, vk::Instance& instance) {
    // Gets the queue families on the device.
    m_queueFamilyProperties = gpu.getQueueFamilyProperties();

    // Goes through all of the flags and finds the indices for them.
    for (uint32_t i = 0; i < m_queueFamilyProperties.size(); i++) {
        if (m_queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
            m_queueFamilyIndices.graphics = i;

        if (SDL_Vulkan_GetPresentationSupport(instance, gpu, i))
            m_queueFamilyIndices.present = i;

        if ((m_queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer) && !(m_queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics))
            m_queueFamilyIndices.transfer = i;
        else if (m_queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer)
            m_queueFamilyIndices.transfer = i;

        // Breaks if everything has already been added to the indices.
        if (m_queueFamilyIndices.IsFull())
            break;
    }

    // Sets the uniques for the indices.
    if (m_queueFamilyIndices.graphics.has_value())
        m_queueFamilyIndices.m_uniques.emplace(m_queueFamilyIndices.graphics.value());
    if (m_queueFamilyIndices.present.has_value())
        m_queueFamilyIndices.m_uniques.emplace(m_queueFamilyIndices.present.value());
    if (m_queueFamilyIndices.transfer.has_value())
        m_queueFamilyIndices.m_uniques.emplace(m_queueFamilyIndices.transfer.value());

    // Sets the unique data for the indices.
    m_queueFamilyIndices.m_data = {m_queueFamilyIndices.m_uniques.begin(), m_queueFamilyIndices.m_uniques.end()};
}

void GPUDevice::GetSwapchainSupport(vk::PhysicalDevice& gpu, vk::SurfaceKHR& surface) {
    m_swapchainSupport.capabilities = gpu.getSurfaceCapabilitiesKHR(surface);
    m_swapchainSupport.formats = gpu.getSurfaceFormatsKHR(surface);
    m_swapchainSupport.presentModes = gpu.getSurfacePresentModesKHR(surface);
}

size_t GPUDevice::GetGPUScore(vk::PhysicalDevice& gpu) {
    size_t score = 0;
    m_properties = gpu.getProperties();

    // If the GPU is discrete, the score will be added by 1000.
    if (m_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        score += 1000;

    // Score is mostly determined by the maximum 2D image dimensions.
    score += m_properties.limits.maxImageDimension2D;

    return score;
}
