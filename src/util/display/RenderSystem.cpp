#include "util/display/RenderSystem.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_filesystem.h>
#include <cstring>
#include <optional>
#include <set>
#include <vulkan/vulkan_beta.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "util/display/vulkan/VkObjectMaps.h"
#include "util/display/vulkan/VkUtil.h"
#include "util/Constants.h"
#include "util/display/Window.h"
#include "util/display/vulkan/VkResultHandler.h"
#include "util/display/buffer/VertexBuffer.h"
#include "util/display/buffer/IndexBuffer.h"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

RenderSystem::Settings RenderSystem::sSettings;
VkInstance RenderSystem::sInstance = VK_NULL_HANDLE;
VkDebugUtilsMessengerEXT RenderSystem::sDebugMessenger = VK_NULL_HANDLE;
VkPhysicalDevice RenderSystem::sPhysicalDevice = VK_NULL_HANDLE;
VkDevice RenderSystem::sDevice = VK_NULL_HANDLE;
VkQueue RenderSystem::sGraphicsQueue = VK_NULL_HANDLE;
VkQueue RenderSystem::sPresentationQueue = VK_NULL_HANDLE;
VkQueue RenderSystem::sTransferQueue = VK_NULL_HANDLE;
VkSurfaceKHR RenderSystem::sSurface = VK_NULL_HANDLE;
VkSwapchainKHR RenderSystem::sSwapChain = VK_NULL_HANDLE;
std::vector<VkImage> RenderSystem::sSwapChainImages;
VkFormat RenderSystem::sSwapChainImageFormat;
VkExtent2D RenderSystem::sSwapChainExtent;
std::vector<VkImageView> RenderSystem::sSwapChainImageViews;
VkRenderPass RenderSystem::sRenderPass = VK_NULL_HANDLE;
std::vector<VkFramebuffer> RenderSystem::sSwapChainFramebuffers;
VkCommandPool RenderSystem::sCommandPool = VK_NULL_HANDLE;
VkCommandPool RenderSystem::sTransferCommandPool = VK_NULL_HANDLE;
uint32_t RenderSystem::sCurrentFrame = 0;
RenderSystem::QueueFamilies RenderSystem::sQueueFamilies;

VkCommandBuffer RenderSystem::sCommandBuffers[MAX_FRAMES_IN_FLIGHT];
VkSemaphore RenderSystem::sImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
VkFence RenderSystem::sInFlightFences[MAX_FRAMES_IN_FLIGHT];

std::vector<VkSemaphore> RenderSystem::sRenderFinishedSemaphores;

uint32_t RenderSystem::sImageIndex = 0;

std::vector<const char*> RenderSystem::sInstanceExtensions;
std::vector<const char*> RenderSystem::sDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME

#ifdef SDL_PLATFORM_MACOS // macOS support.
    ,VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
};

std::vector<const char*> RenderSystem::sLayers = {
    "VK_LAYER_KHRONOS_validation"
};

Logger RenderSystem::sLogger = Logger("RenderSystem");
VkApplicationInfo RenderSystem::sAppInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Voxel Renderer",
    .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
    .pEngineName = "Voxel Engine",
    .engineVersion = VK_MAKE_VERSION(0, 1, 0),
    .apiVersion = VK_API_VERSION_1_0
};

VkDebugUtilsMessengerCreateInfoEXT RenderSystem::sDebugMessengerInfo = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = ENABLED_SEVERITY_FLAGS,
    .messageType = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = DebugCallback,
    .pUserData = VK_NULL_HANDLE
};

void RenderSystem::Initialize(const Settings& settings) {
    sSettings = settings;

    CreateInstance();
    CreateDebugMessenger();
    CreateSurface();
    SelectBestPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffer();
    CreateSyncObjects();
}

void RenderSystem::Destroy() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(sDevice, sImageAvailableSemaphores[i], VK_NULL_HANDLE);
        vkDestroyFence(sDevice, sInFlightFences[i], VK_NULL_HANDLE);
    }

    for (size_t i = 0; i < sSwapChainImages.size(); i++)
        vkDestroySemaphore(sDevice, sRenderFinishedSemaphores[i], VK_NULL_HANDLE);

    vkDestroyCommandPool(sDevice, sTransferCommandPool, VK_NULL_HANDLE);
    vkDestroyCommandPool(sDevice, sCommandPool, VK_NULL_HANDLE);

    for (auto framebuffer : sSwapChainFramebuffers)
        vkDestroyFramebuffer(sDevice, framebuffer, VK_NULL_HANDLE);

    vkDestroyRenderPass(sDevice, sRenderPass, VK_NULL_HANDLE);

    for (auto imageView : sSwapChainImageViews)
        vkDestroyImageView(sDevice, imageView, VK_NULL_HANDLE);

    vkDestroySwapchainKHR(sDevice, sSwapChain, VK_NULL_HANDLE);

    vkDestroyDevice(sDevice, VK_NULL_HANDLE);

    if (ENABLE_VALIDATION_LAYERS)
        DestroyDebugUtilsMessengerEXT(VK_NULL_HANDLE);

    SDL_Vulkan_DestroySurface(sInstance, sSurface, VK_NULL_HANDLE);

    vkDestroyInstance(sInstance, VK_NULL_HANDLE);
}

void RenderSystem::UpdateDisplay() {
    RecreateSwapChain();
}

void RenderSystem::BeginDrawFrame() {
    vkWaitForFences(sDevice, 1, &sInFlightFences[sCurrentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(sDevice, 1, &sInFlightFences[sCurrentFrame]);

    vkAcquireNextImageKHR(sDevice, sSwapChain, UINT64_MAX, sImageAvailableSemaphores[sCurrentFrame], VK_NULL_HANDLE, &sImageIndex);

    vkResetFences(sDevice, 1, &sInFlightFences[sCurrentFrame]);

    vkResetCommandBuffer(sCommandBuffers[sCurrentFrame], 0);
    BeginRecordCmdBuffer(sCommandBuffers[sCurrentFrame], sImageIndex);
}

void RenderSystem::EndDrawFrame() {
    EndRecordCmdBuffer(sCommandBuffers[sCurrentFrame], sImageIndex);
    
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &sImageAvailableSemaphores[sCurrentFrame],
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &sCommandBuffers[sCurrentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &sRenderFinishedSemaphores[sImageIndex]
    };

    VkResult result = vkQueueSubmit(sGraphicsQueue, 1, &submitInfo, sInFlightFences[sCurrentFrame]);
    VkResultHandler::CheckResult(result, "Failed to submit command buffer!");

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &sRenderFinishedSemaphores[sImageIndex],
        .swapchainCount = 1,
        .pSwapchains = &sSwapChain,
        .pImageIndices = &sImageIndex,
        .pResults = VK_NULL_HANDLE
    };

    vkQueuePresentKHR(sPresentationQueue, &presentInfo);

    // Advance to the next frame.
    sCurrentFrame = (sCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderSystem::CmdBindVertexBuffers(VertexBuffer* buffers, uint32_t n) {
    VkBuffer _buffers[n];
    for (uint32_t i = 0; i < n; i++)
        _buffers[i] = buffers[i].m_buffer.m_handler;

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(sCommandBuffers[sCurrentFrame], 0, n, _buffers, &offset);
}

void RenderSystem::CmdBindVertexBuffer(VertexBuffer& buffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(sCommandBuffers[sCurrentFrame], 0, 1, &buffer.m_buffer.m_handler, &offset);
}

void RenderSystem::CmdBindIndexBuffer(IndexBuffer& buffer) {
    // TODO setup custom index type.
    vkCmdBindIndexBuffer(sCommandBuffers[sCurrentFrame], buffer.m_buffer.m_handler, 0, VkObjectMaps::GetIndexType(buffer.m_type));
}

void RenderSystem::CmdDraw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
    vkCmdDraw(sCommandBuffers[sCurrentFrame], vertex_count, instance_count, first_vertex, first_instance);
}

void RenderSystem::CmdDrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t first_instance) {
    vkCmdDrawIndexed(sCommandBuffers[sCurrentFrame], index_count, instance_count, first_index, 0, first_instance);
}

void RenderSystem::WaitDevice() {
    vkDeviceWaitIdle(sDevice);
}

void RenderSystem::CreateInstance() {
    SetupRequiredExtensions();

    if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayerSupport())
        throw sLogger.RuntimeError("Not all of the requested validation layers are available!");

    std::optional<VkDebugUtilsMessengerCreateInfoEXT> debugMessenger;
    if (ENABLE_VALIDATION_LAYERS)
        debugMessenger = sDebugMessengerInfo;

    VkResult result = VkUtil::CreateInstance(&sInstance, sInstanceExtensions, sLayers, sAppInfo, debugMessenger);
    VkResultHandler::CheckResult(result, "Failed to create Vulkan instance!", "Created Vulkan instance.");
}

void RenderSystem::SetupRequiredExtensions() {
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
    if (ENABLE_VALIDATION_LAYERS)
        sInstanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

bool RenderSystem::CheckValidationLayerSupport() {
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

VKAPI_ATTR VkBool32 VKAPI_CALL RenderSystem::DebugCallback(
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

void RenderSystem::CreateDebugMessenger() {
    if (!ENABLE_VALIDATION_LAYERS) return;

    VkResult result = CreateDebugUtilsMessengerEXT(&sDebugMessengerInfo, VK_NULL_HANDLE, &sDebugMessenger);
    VkResultHandler::CheckResult(result, "Failed to create debug messenger!", "Created debug messenger.");
}

VkResult RenderSystem::CreateDebugUtilsMessengerEXT(
    const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debug_messenger) {

    // Loads the function with instance proc.
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(sInstance, "vkCreateDebugUtilsMessengerEXT");

    // Failed to load the function if it returns nullptr.
    if (func)
        return func(sInstance, create_info, allocator, debug_messenger); // Run normally.

    return VK_ERROR_EXTENSION_NOT_PRESENT; // Return error, extension not found.
}

void RenderSystem::DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* allocator) {
    // Loads the function with instance proc.
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(sInstance, "vkDestroyDebugUtilsMessengerEXT");

    // Failed to load if it returns nullptr.
    if (func)
        func(sInstance, sDebugMessenger, allocator); // Run normally.
    else
        sLogger.Error("Failed to destroy debug messenger! Unable to load vkDestroyDebugUtilsMessengerEXT.");
}

void RenderSystem::SelectBestPhysicalDevice() {
    // Gets the amount of GPUs on the system that support Vulkan.
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(sInstance, &count, VK_NULL_HANDLE);

    // Throws a runtime error if no GPUs with Vulkan support exist.
    if (count == 0)
        throw sLogger.RuntimeError("Failed to find a GPU with Vulkan support!");

    VkPhysicalDevice devices[count];
    vkEnumeratePhysicalDevices(sInstance, &count, devices);

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
    sPhysicalDevice = devices[currentBestDeviceIndex];

    // Find its name and log it.
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(sPhysicalDevice, &properties);
    sLogger.Info("Using physical device '", properties.deviceName, "'.");
}

bool RenderSystem::IsPhysicalDeviceUsable(VkPhysicalDevice device) {
    QueueFamilies indices = GetQueueFamilies(device);

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

size_t RenderSystem::GetPhysicalDeviceScore(VkPhysicalDevice device) {
    size_t score = 0;

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    // If the GPU is discrete, the score will be added by 1000.
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // Score is mostly determined by the maximum 2D image dimensions.
    score += properties.limits.maxImageDimension2D;

    return score;
}

RenderSystem::QueueFamilies RenderSystem::GetQueueFamilies(VkPhysicalDevice device) {
    QueueFamilies indices;

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

        if (SDL_Vulkan_GetPresentationSupport(sInstance, device, i))
            indices.m_presentation = i;

#ifdef SDL_PLATFORM_APPLE
        if (families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            indices.m_transfer = i;
#else
        if ((families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && !(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            indices.m_transfer = i;
#endif

        // Breaks if everything has already been added to the indices.
        if (indices.HasEverything())
            break;
    }

    return indices;
}

void RenderSystem::CreateLogicalDevice() {
    sQueueFamilies = GetQueueFamilies(sPhysicalDevice);

    if (!sQueueFamilies.m_graphics.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No graphics family found.");

    if (!sQueueFamilies.m_presentation.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No presentation family found.");

    if (!sQueueFamilies.m_transfer.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No transfer family found.");

    // Gets the unique queue families that exist.
    std::set<uint32_t> uniqueQueueFamilies = {sQueueFamilies.m_graphics.value(), sQueueFamilies.m_presentation.value(), sQueueFamilies.m_transfer.value()};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        });
    }

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(sPhysicalDevice, &features);
#ifdef SDL_PLATFORM_MACOS
    // Disables robust buffer access if the platform is macOS.
    features.robustBufferAccess = VK_FALSE;
#endif

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(), // Links the queues.
        .enabledExtensionCount = static_cast<uint32_t>(sDeviceExtensions.size()),
        .ppEnabledExtensionNames = sDeviceExtensions.data(),
        .pEnabledFeatures = &features // Links the device features.
    };

    if (ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(sLayers.size());
        createInfo.ppEnabledLayerNames = sLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(sPhysicalDevice, &createInfo, VK_NULL_HANDLE, &sDevice);
    VkResultHandler::CheckResult(result, "Failed to create logical device!", "Created logical device.");

    vkGetDeviceQueue(sDevice, sQueueFamilies.m_graphics.value(), 0, &sGraphicsQueue);
    vkGetDeviceQueue(sDevice, sQueueFamilies.m_presentation.value(), uniqueQueueFamilies.size() < 2 ? 0 : 1, &sPresentationQueue);

    uint32_t transferIndex = uniqueQueueFamilies.size() < 2 ? 0 : uniqueQueueFamilies.size() < 3 ? 1 : uniqueQueueFamilies.size() - 1;
    vkGetDeviceQueue(sDevice, sQueueFamilies.m_transfer.value(), transferIndex, &sTransferQueue);
}

void RenderSystem::CreateSurface() {
    if (!SDL_Vulkan_CreateSurface(Window::sContext, sInstance, VK_NULL_HANDLE, &sSurface))
        throw sLogger.RuntimeError("Failed to create surface!", SDL_GetError());
    sLogger.Info("Created surface.");
}

bool RenderSystem::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
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

RenderSystem::SwapChainSupportDetails RenderSystem::GetSwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, sSurface, &details.m_capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, sSurface, &formatCount, VK_NULL_HANDLE);

    if (formatCount != 0) {
        details.m_formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, sSurface, &formatCount, details.m_formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, sSurface, &presentModeCount, VK_NULL_HANDLE);
    
    if (presentModeCount != 0) {
        details.m_presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, sSurface, &presentModeCount, details.m_presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR RenderSystem::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // Check for the best format.
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

    // Use the first format if the best one is not present.
    return availableFormats[0];
}

VkPresentModeKHR RenderSystem::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Checks if the swap interval from the renderer properties is usable. If not, then VSYNC is used instead.
    for (const auto& availablePresentMode : availablePresentModes)
        if (availablePresentMode == VkObjectMaps::GetPresentMode(sSettings.m_swapInterval))
            return availablePresentMode;

    return VkObjectMaps::GetPresentMode(RENDER_SWAP_INTERVAL_VSYNC);
}

VkExtent2D RenderSystem::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // If extent is at the maximum value, return the extent as is.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    // Get the width and height of the window.
    VkExtent2D actualExtent = {
        .width = static_cast<uint32_t>(Window::DisplayWidth()),
        .height = static_cast<uint32_t>(Window::DisplayHeight())
    };

    // Clamp the values to be within the maximum capabilities for the extent.
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void RenderSystem::CreateSwapChain() {
    SwapChainSupportDetails swapChainSupport = GetSwapChainSupport(sPhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.m_capabilities);

    // Set the image count to the minimum plus one so we don't have to wait on the driver.
    uint32_t imageCount = swapChainSupport.m_capabilities.minImageCount + 1;

    // Account for the special case where the minumum image count equals the maximum image count.
    // Zero is a special case that means there is no maximum.
    if (swapChainSupport.m_capabilities.maxImageCount > 0 && imageCount > swapChainSupport.m_capabilities.maxImageCount)
        imageCount = swapChainSupport.m_capabilities.maxImageCount;
    
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = sSurface,
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
    uint32_t queueFamilyIndices[] = {sQueueFamilies.m_graphics.value(), sQueueFamilies.m_presentation.value()};

    if (sQueueFamilies.m_graphics != sQueueFamilies.m_presentation) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkResult result = vkCreateSwapchainKHR(sDevice, &swapChainCreateInfo, VK_NULL_HANDLE, &sSwapChain);
    VkResultHandler::CheckResult(result, "Failed to create swap chain!");

    // Load the swap chain images.
    vkGetSwapchainImagesKHR(sDevice, sSwapChain, &imageCount, VK_NULL_HANDLE);
    sSwapChainImages.resize(imageCount); // Allocate space first.
    vkGetSwapchainImagesKHR(sDevice, sSwapChain, &imageCount, sSwapChainImages.data());

    // Save the image format and extent.
    sSwapChainImageFormat = surfaceFormat.format;
    sSwapChainExtent = extent;
}

void RenderSystem::CreateImageViews() {
    sSwapChainImageViews.resize(sSwapChainImages.size());
    for (size_t i = 0; i < sSwapChainImages.size(); i++) {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = sSwapChainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D, // 2D textures.
            .format = sSwapChainImageFormat
        };

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

        VkResult result = vkCreateImageView(sDevice, &imageViewCreateInfo, VK_NULL_HANDLE, &sSwapChainImageViews[i]);
        VkResultHandler::CheckResult(result, "Failed to create image view!");
    }
}

void RenderSystem::CreateRenderPass() {
    VkAttachmentDescription colorAttachment = {
        .format = sSwapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(sDevice, &createInfo, VK_NULL_HANDLE, &sRenderPass);
    VkResultHandler::CheckResult(result, "Failed to create render pass!", "Created render pass.");
}

void RenderSystem::CreateFramebuffers() {
    sSwapChainFramebuffers.resize(sSwapChainImageViews.size());

    for (size_t i = 0; i < sSwapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            sSwapChainImageViews[i]
        };

        VkFramebufferCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = sRenderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = sSwapChainExtent.width,
            .height = sSwapChainExtent.height,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(sDevice, &createInfo, VK_NULL_HANDLE, &sSwapChainFramebuffers[i]);
        VkResultHandler::CheckResult(result, "Failed to create framebuffer!");
    }
}

void RenderSystem::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = sQueueFamilies.m_graphics.value()
    };

    VkResult result = vkCreateCommandPool(sDevice, &poolInfo, VK_NULL_HANDLE, &sCommandPool);
    VkResultHandler::CheckResult(result, "Failed to create command pool!", "Created command pool.");

    VkCommandPoolCreateInfo transferPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = sQueueFamilies.m_transfer.value()
    };

    result = vkCreateCommandPool(sDevice, &transferPoolInfo, VK_NULL_HANDLE, &sTransferCommandPool);
    VkResultHandler::CheckResult(result, "Failed to create transfer command pool!", "Created transfer command pool.");
}

void RenderSystem::CreateCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = sCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    VkResult result = vkAllocateCommandBuffers(sDevice, &allocInfo, &sCommandBuffers[sCurrentFrame]);
    VkResultHandler::CheckResult(result, "Failed to create command buffer!", "Created command buffer.");
}

void RenderSystem::CreateSyncObjects() {
    sRenderFinishedSemaphores.resize(sSwapChainImages.size());

    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result = vkCreateSemaphore(sDevice, &semaphoreCreateInfo, VK_NULL_HANDLE, &sImageAvailableSemaphores[i]);
        VkResultHandler::CheckResult(result, "Failed to create image available semaphore!");

        result = vkCreateFence(sDevice, &fenceCreateInfo, VK_NULL_HANDLE, &sInFlightFences[i]);
        VkResultHandler::CheckResult(result, "Failed to create in flight fence!");
    }

    for (size_t i = 0; i < sSwapChainImages.size(); i++) {
        VkResult result = vkCreateSemaphore(sDevice, &semaphoreCreateInfo, VK_NULL_HANDLE, &sRenderFinishedSemaphores[i]);
        VkResultHandler::CheckResult(result, "Failed to create render finished semaphore!");
    }

    sLogger.Info("Created sync objects.");
}

void RenderSystem::BeginRecordCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
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
        .renderPass = sRenderPass,
        .framebuffer = sSwapChainFramebuffers[imageIndex],
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };

    // Keep the render area the same size as the images for the best performance.
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = sSwapChainExtent;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set the dynamic viewport and scissor.
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(sSwapChainExtent.width),
        .height = static_cast<float>(sSwapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = sSwapChainExtent
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void RenderSystem::EndRecordCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    vkCmdEndRenderPass(commandBuffer);

    VkResult result = vkEndCommandBuffer(commandBuffer);
    VkResultHandler::CheckResult(result, "Failed to record command buffer!");
}

void RenderSystem::RecreateSwapChain() {
    if (Window::sMinimized)
        return;

    vkDeviceWaitIdle(sDevice);

    for (auto framebuffer : sSwapChainFramebuffers)
        vkDestroyFramebuffer(sDevice, framebuffer, VK_NULL_HANDLE);

    for (auto imageView : sSwapChainImageViews)
        vkDestroyImageView(sDevice, imageView, VK_NULL_HANDLE);

    vkDestroySwapchainKHR(sDevice, sSwapChain, VK_NULL_HANDLE);

    CreateSwapChain();
    CreateImageViews();
    CreateFramebuffers();
}

uint32_t RenderSystem::FindMemoryType(uint32_t filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(sPhysicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        if (filter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;

    throw sLogger.RuntimeError("Failed to find suitable memory type!");
}
