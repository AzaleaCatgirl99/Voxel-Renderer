#include "renderer/detail/VkRenderer.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_filesystem.h>
#include <set>
#include <vulkan/vulkan_beta.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "util/BufferedFile.h"
#include "util/Constants.h"
#include "util/Window.h"
#include "renderer/detail/vulkan/VkResultHandler.h"

namespace detail {

// The required extensions needed for the renderer.
std::vector<const char*> VkRenderer::sInstanceExtensions;

std::vector<const char*> VkRenderer::sDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME

#ifdef SDL_PLATFORM_MACOS // macOS support.
    ,VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
};

// Map for getting the Vulkan present mode from the render swap interval.
const std::flat_map<eRenderSwapInterval, VkPresentModeKHR> VkRenderer::sPresentModes =
            {{RENDER_SWAP_INTERVAL_IMMEDIATE, VK_PRESENT_MODE_IMMEDIATE_KHR},
            {RENDER_SWAP_INTERVAL_VSYNC, VK_PRESENT_MODE_FIFO_KHR},
            {RENDER_SWAP_INTERVAL_TRIPLE_BUFFERING, VK_PRESENT_MODE_MAILBOX_KHR}};

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
    CreateDebugMessenger();

    // Creates the surface.
    CreateSurface();

    // Selects the best GPU to use.
    SelectBestPhysicalDevice();

    // Create the logical device.
    CreateLogicalDevice();

    // Create the swap chain.
    CreateSwapChain();

    // Create the image views for the swap chain.
    CreateImageViews();

    // Creates the render pass.
    CreateRenderPass();

    // Creates the test graphics pipeline.
    CreateTestGraphicsPipeline();

    // Creates the framebuffers for the swap chain.
    CreateFramebuffers();

    // Creates the command pool.
    CreateCommandPool();

    // Creates the command buffer.
    CreateCommandBuffer();

    // Creates the objects needed for syncing the frames.
    CreateSyncObjects();
}

void VkRenderer::Destroy() {
    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphore, VK_NULL_HANDLE);
    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphore, VK_NULL_HANDLE);
    vkDestroyFence(m_logicalDevice, m_inFlightFence, VK_NULL_HANDLE);

    vkDestroyCommandPool(m_logicalDevice, m_commandPool, VK_NULL_HANDLE);

    for (auto framebuffer : m_swapChainFramebuffers)
        vkDestroyFramebuffer(m_logicalDevice, framebuffer, VK_NULL_HANDLE);

    vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, VK_NULL_HANDLE);

    vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, VK_NULL_HANDLE);

    vkDestroyRenderPass(m_logicalDevice, m_renderPass, VK_NULL_HANDLE);

    for (auto imageView : m_swapChainImageViews)
        vkDestroyImageView(m_logicalDevice, imageView, VK_NULL_HANDLE);

    vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, VK_NULL_HANDLE);

    vkDestroyDevice(m_logicalDevice, VK_NULL_HANDLE);

    if (sEnableValidationLayers)
        DestroyDebugUtilsMessengerEXT(VK_NULL_HANDLE);

    SDL_Vulkan_DestroySurface(m_instance, m_surface, VK_NULL_HANDLE);

    vkDestroyInstance(m_instance, VK_NULL_HANDLE);
}

void VkRenderer::UpdateDisplay() {
    // TODO recreate the swap chain.
}

void VkRenderer::DrawFrame() {
    // Waits for the next frame.
    vkWaitForFences(m_logicalDevice, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);

    // Resets the flight fence.
    vkResetFences(m_logicalDevice, 1, &m_inFlightFence);

    // Gets the next image index in the swap chain to be used.
    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // Resets the command buffer.
    vkResetCommandBuffer(m_commandBuffer, 0);

    // Records the command buffer.
    RecordCommandBuffer(m_commandBuffer, imageIndex);

    // Creates the submit info needed to submit the command buffer.
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphore};
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    // Submits the command buffer to the graphics queue.
    VkResult result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence);
    VkResultHandler::CheckResult(result, "Failed to submit command buffer!");

    // The present info needed for submitting the result to the swap chain.
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = &m_swapChain,
        .pImageIndices = &imageIndex,
        .pResults = VK_NULL_HANDLE
    };

    // Submits the image to the swap chain.
    vkQueuePresentKHR(m_presentationQueue, &presentInfo);

    vkQueueWaitIdle(m_presentationQueue); // TODO remove once Princess's chappie is done.
}

void VkRenderer::CreateInstance() {
    // Sets up the required extensions.
    SetupRequiredExtensions();

    // Sets up the instance info.
    SetupInstanceInfo();

    // Create the Vulkan instance.
    VkResult result = vkCreateInstance(&sCreateInfo, VK_NULL_HANDLE, &m_instance);
    VkResultHandler::CheckResult(result, "Failed to create Vulkan instance!", "Created Vulkan instance.");
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
        .enabledExtensionCount = static_cast<uint32_t>(sInstanceExtensions.size()),
        .ppEnabledExtensionNames = sInstanceExtensions.data()
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
    sInstanceExtensions = std::vector<const char*>(sdlExtensions, sdlExtensions + sdlExtensionCount);

    // Add support for macOS.
    #ifdef SDL_PLATFORM_MACOS
    sInstanceExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    sInstanceExtensions.emplace_back("VK_KHR_get_physical_device_properties2");
    #endif

    // Add extension for validation layer message callback.
    if (sEnableValidationLayers)
        sInstanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

bool VkRenderer::CheckValidationLayerSupport() {
    // Find the number of validation layers available.
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE);

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

void VkRenderer::CreateDebugMessenger() {
    if (!sEnableValidationLayers) return;

    VkResult result = CreateDebugUtilsMessengerEXT(&sDebugMessengerInfo, VK_NULL_HANDLE, &m_debugMessenger);
    VkResultHandler::CheckResult(result, "Failed to create debug messenger!", "Created debug messenger.");
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
        sLogger.Error("Failed to destroy debug messenger! Unable to load vkDestroyDebugUtilsMessengerEXT.");
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

    // Set the physical best physical device.
    m_physicalDevice = devices[currentBestDeviceIndex];

    // Find its name and log it.
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
    sLogger.Info("Using physical device '", properties.deviceName, "'.");
}

bool VkRenderer::IsPhysicalDeviceUsable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = GetQueueFamilies(device);

    // Ensure all queue families are supported.
    if (!indices.HasEverything())
        return false;
    
    // Ensure all required device extensions are supported.
    if (!CheckDeviceExtensionSupport(device))
        return false;

    // Ensure required swap chain functionality.
    SwapChainSupportDetails swapChainSupport = GetSwapChainSupport(device);
    if (swapChainSupport.m_formats.empty() || swapChainSupport.m_presentModes.empty())
        return false;
    
    return true; // Device is Usable.
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
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.m_graphics = i;

        // Adds the presentation index to the indices if support exists.
        if (SDL_Vulkan_GetPresentationSupport(m_instance, device, i))
            indices.m_presentation = i;

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
        throw sLogger.RuntimeError("Failed to create logical device! No graphics family found.");

    // Ensure presentation is not empty.
    if (!indices.m_presentation.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No presentation family found.");

    // Gets the unique queue families that exist.
    std::set<uint32_t> uniqueQueueFamilies = {indices.m_graphics.value(), indices.m_presentation.value()};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    // Creates all of the create info queue families.
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        });
    }

    // Gets the physical device's features.
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &features);
#ifdef SDL_PLATFORM_MACOS
    // Disables robust buffer access if the platform is macOS.
    features.robustBufferAccess = VK_FALSE;
#endif

    // Create the info struct for the logical device.
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(), // Links the queues.
        .enabledExtensionCount = static_cast<uint32_t>(sDeviceExtensions.size()),
        .ppEnabledExtensionNames = sDeviceExtensions.data(),
        .pEnabledFeatures = &features // Links the device features.
    };

    // Enables device-specific validation layer support for older implementations.
    if (sEnableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(sLayers.size());
        createInfo.ppEnabledLayerNames = sLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Creates the logical device.
    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, VK_NULL_HANDLE, &m_logicalDevice);
    VkResultHandler::CheckResult(result, "Failed to create logical device!", "Created logical device.");

    // Gets the queue handles.
    vkGetDeviceQueue(m_logicalDevice, indices.m_graphics.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_logicalDevice, indices.m_presentation.value(), uniqueQueueFamilies.size() < 2 ? 0 : 1, &m_presentationQueue);
}

void VkRenderer::CreateSurface() {
    if (!SDL_Vulkan_CreateSurface(m_window->m_pHandler, m_instance, VK_NULL_HANDLE, &m_surface))
        throw sLogger.RuntimeError("Failed to create surface!", SDL_GetError());
    sLogger.Info("Created surface.");
}

bool VkRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    // Get extension information. Find number of extensions first for allocating the extension vector.
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &extensionCount, VK_NULL_HANDLE);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &extensionCount, availableExtensions.data());

    // Ensure all required device extensions are supported.
    std::set<std::string> requiredExtensions(sDeviceExtensions.begin(), sDeviceExtensions.end());

    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

VkRenderer::SwapChainSupportDetails VkRenderer::GetSwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    // Get the swap chain capabilities.
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.m_capabilities);

    // Get the swap chain formats.
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, VK_NULL_HANDLE);

    if (formatCount != 0) {
        details.m_formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.m_formats.data());
    }

    // Get the swap chain present modes.
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, VK_NULL_HANDLE);
    
    if (presentModeCount != 0) {
        details.m_presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.m_presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR VkRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // Check for the best format.
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

    // Use the first format if the best one is not present.
    return availableFormats[0];
}

VkPresentModeKHR VkRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Checks if the swap interval from the renderer properties is usable. If not, then VSYNC is used instead.
    for (const auto& availablePresentMode : availablePresentModes)
        if (availablePresentMode == sPresentModes.at(m_properties.m_swapInterval))
            return availablePresentMode;

    return sPresentModes.at(RENDER_SWAP_INTERVAL_VSYNC);
}

VkExtent2D VkRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // If extent is at the maximum value, return the extent as is.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    // Get the width and height of the window.
    VkExtent2D actualExtent = {
        .width = static_cast<uint32_t>(m_window->DisplayWidth()),
        .height = static_cast<uint32_t>(m_window->DisplayHeight())
    };

    // Clamp the values to be within the maximum capabilities for the extent.
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void VkRenderer::CreateSwapChain() {
    SwapChainSupportDetails swapChainSupport = GetSwapChainSupport(m_physicalDevice);

    // Get Choose the surface format, present mode, and extent.
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.m_capabilities);

    // Set the image count to the minimum plus one so we don't have to wait on the driver.
    uint32_t imageCount = swapChainSupport.m_capabilities.minImageCount + 1;

    // Account for the special case where the minumum image count equals the maximum image count.
    // Zero is a special case that means there is no maximum.
    if (swapChainSupport.m_capabilities.maxImageCount > 0 && imageCount > swapChainSupport.m_capabilities.maxImageCount)
        imageCount = swapChainSupport.m_capabilities.maxImageCount;
    
    // Create the swap chain info struct.
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapChainSupport.m_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    // Handle the cases for whether the queue families are the same.
    QueueFamilyIndices indices = GetQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.m_graphics.value(), indices.m_presentation.value()};

    if (indices.m_graphics != indices.m_presentation) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // Create the swap chain.
    VkResult result = vkCreateSwapchainKHR(m_logicalDevice, &swapChainCreateInfo, VK_NULL_HANDLE, &m_swapChain);
    VkResultHandler::CheckResult(result, "Failed to create swap chain!", "Created swap chain.");

    // Load the swap chain images.
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, VK_NULL_HANDLE);
    m_swapChainImages.resize(imageCount); // Allocate space first.
    vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

    // Save the image format and extent.
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void VkRenderer::CreateImageViews() {
    m_swapChainImageViews.resize(m_swapChainImages.size());
    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        // Create image view struct for each image.
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_swapChainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D, // 2D textures.
            .format = m_swapChainImageFormat
        };

        // Set the color channel swizzles to defaults.
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Set images such that they are accessed simply, no mipmapping or multiple layers.
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        // Create the image view.
        VkResult result = vkCreateImageView(m_logicalDevice, &imageViewCreateInfo, VK_NULL_HANDLE, &m_swapChainImageViews[i]);
        VkResultHandler::CheckResult(result, "Failed to create image view!");
    }
    sLogger.Info("Created image views.");
}

void VkRenderer::CreateRenderPass() {
    // Attachment used for the color buffer.
    VkAttachmentDescription colorAttachment = {
        .format = m_swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    // Reference for the color attachment.
    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    // The main subpass for the render pass.
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    // The dependency for the subpass.
    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    // The main create info for the render pass.
    VkRenderPassCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    // Creates the render pass.
    VkResult result = vkCreateRenderPass(m_logicalDevice, &createInfo, VK_NULL_HANDLE, &m_renderPass);
    VkResultHandler::CheckResult(result, "Failed to create render pass!", "Created render pass.");
}

void VkRenderer::CreateTestGraphicsPipeline() {
    // Gets shader file data.
    std::string basePath = SDL_GetBasePath();
    BufferedFile vertShader = BufferedFile::Read(basePath + "assets/shaders/test_vert.spv", true);
    BufferedFile fragShader = BufferedFile::Read(basePath + "assets/shaders/test_frag.spv", true);

    // Creates the vertex shader info.
    VkShaderModuleCreateInfo vertShaderCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vertShader.Size(),
        .pCode = vertShader.DataAsUInt32()
    };

    // Creates vertex shader module.
    VkShaderModule vertShaderModule;
    VkResult result = vkCreateShaderModule(m_logicalDevice, &vertShaderCreateInfo, VK_NULL_HANDLE, &vertShaderModule);
    VkResultHandler::CheckResult(result, "Failed to create vertex shader module!");

    // Creates the fragment shader info.
    VkShaderModuleCreateInfo fragShaderCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fragShader.Size(),
        .pCode = fragShader.DataAsUInt32()
    };

    // Creates fragment shader module.
    VkShaderModule fragShaderModule;
    result = vkCreateShaderModule(m_logicalDevice, &fragShaderCreateInfo, VK_NULL_HANDLE, &fragShaderModule);
    VkResultHandler::CheckResult(result, "Failed to create fragment shader module!");

    // Creates the vertex shader stage info.
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };
    
    // Creates the fragment shader stage info.
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };

    // Simple array for use in the graphics pipeline object creation.
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Configure the dynamic state setup.
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // Configure how vertex data is passed to the vertex shader.
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pVertexBindingDescriptions = VK_NULL_HANDLE,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = VK_NULL_HANDLE
    };

    // Configure how vertices are interpreted.
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Configure the viewport state. Ensure it is dynamic.
    VkPipelineViewportStateCreateInfo viewportStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1, // Only need 1 viewport and scissor.
        .pViewports = VK_NULL_HANDLE,
        .scissorCount = 1,
        .pScissors = VK_NULL_HANDLE
    };

    // Configure the rasterizer.
    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE, // Discard off-screen fragments, do not clamp.
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL, // Fill triangles.
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    // Configure multisampling. Disabled for now.
    VkPipelineMultisampleStateCreateInfo multisamplingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = VK_NULL_HANDLE,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    // Configure depth and stencil testing in the future.

    // Configure color blending for each frame buffer. Pass colors through unmodified for now.
    // Important for blending alpha channels.
    VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional.
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional.
        .colorBlendOp = VK_BLEND_OP_ADD, // Optional.
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional.
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional.
        .alphaBlendOp = VK_BLEND_OP_ADD, // Optional.
        .colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
    };

    // Configure global color blending rules that applies to all frame buffers.
    VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentInfo
    };
    colorBlendingInfo.blendConstants[0] = 0.0f;
    colorBlendingInfo.blendConstants[1] = 0.0f;
    colorBlendingInfo.blendConstants[2] = 0.0f;
    colorBlendingInfo.blendConstants[3] = 0.0f;

    // Configure and create pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = VK_NULL_HANDLE,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = VK_NULL_HANDLE
    };

    result = vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo, VK_NULL_HANDLE, &m_pipelineLayout);
    VkResultHandler::CheckResult(result, "Failed to create pipeline layout!", "Created pipeline layout.");

    // Create the graphices pipeline.
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterizerInfo,
        .pMultisampleState = &multisamplingInfo,
        .pDepthStencilState = VK_NULL_HANDLE,
        .pColorBlendState = &colorBlendingInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = m_pipelineLayout,
        .renderPass = m_renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    result = vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, VK_NULL_HANDLE, &m_graphicsPipeline);
    VkResultHandler::CheckResult(result, "Failed to create graphics pipeline!", "Created graphics pipeline.");

    // Deletes the shader modules. These should be at the end of the function.
    vkDestroyShaderModule(m_logicalDevice, fragShaderModule, VK_NULL_HANDLE);
    vkDestroyShaderModule(m_logicalDevice, vertShaderModule, VK_NULL_HANDLE);
}

void VkRenderer::CreateFramebuffers() {
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_swapChainImageViews[i]
        };

        // The create info used by the framebuffer.
        VkFramebufferCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = m_swapChainExtent.width,
            .height = m_swapChainExtent.height,
            .layers = 1
        };

        // Creates the framebuffer.
        VkResult result = vkCreateFramebuffer(m_logicalDevice, &createInfo, VK_NULL_HANDLE, &m_swapChainFramebuffers[i]);
        VkResultHandler::CheckResult(result, "Failed to create framebuffer!");
    }

    sLogger.Info("Created framebuffers.");
}

void VkRenderer::CreateCommandPool() {
    QueueFamilyIndices queueFamilyIndices = GetQueueFamilies(m_physicalDevice);

    // Create the info struct for the command pool.
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.m_graphics.value()
    };

    VkResult result = vkCreateCommandPool(m_logicalDevice, &poolInfo, VK_NULL_HANDLE, &m_commandPool);
    VkResultHandler::CheckResult(result, "Failed to create command pool!", "Created command pool.");
}

void VkRenderer::CreateCommandBuffer() {
    // Create the allocate info struct for the command pool.
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkResult result = vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &m_commandBuffer);
    VkResultHandler::CheckResult(result, "Failed to create command buffer!", "Created command buffer.");
}

void VkRenderer::CreateSyncObjects() {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    // Creates the semaphore for the available image.
    VkResult result = vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, VK_NULL_HANDLE, &m_imageAvailableSemaphore);
    VkResultHandler::CheckResult(result, "Failed to create image available semaphore!");

    // Creates the semaphore for the render finished.
    result = vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, VK_NULL_HANDLE, &m_renderFinishedSemaphore);
    VkResultHandler::CheckResult(result, "Failed to create render finished semaphore!");

    // Creates the fence for in flight.
    result = vkCreateFence(m_logicalDevice, &fenceCreateInfo, VK_NULL_HANDLE, &m_inFlightFence);
    VkResultHandler::CheckResult(result, "Failed to create in flight fence!");

    sLogger.Info("Created sync objects.");
}

void VkRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    // Specify details about the usage of the command buffer.
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = VK_NULL_HANDLE
    };

    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    VkResultHandler::CheckResult(result, "Failed to begin recording command buffer!");
    
    // Clear color before drawing.
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    
    // Start drawing by beginning a render pass.
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_renderPass,
        .framebuffer = m_swapChainFramebuffers[imageIndex],
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };

    // Keep the render area the same size as the images for the best performance.
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind the grpahics pipeline.
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    // Set the dynamic viewport and scissor.
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_swapChainExtent.width),
        .height = static_cast<float>(m_swapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = m_swapChainExtent
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Draw the triangle.
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // End the render pass.
    vkCmdEndRenderPass(commandBuffer);

    // Finish recording the command buffer.
    result = vkEndCommandBuffer(commandBuffer);
    VkResultHandler::CheckResult(result, "Failed to record command buffer!");
}

}
