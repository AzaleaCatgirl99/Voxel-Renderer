#include "util/display/device/SwapchainHandler.h"

#include "util/display/Window.h"
#include "util/display/device/GPUDevice.h"
#include <cstddef>
#include <cstdint>

vk::SwapchainKHR SwapchainHandler::sSwapchain;
vk::Extent2D SwapchainHandler::sExtent;
vk::SurfaceFormatKHR SwapchainHandler::sSurfaceFormat;
vk::PresentModeKHR SwapchainHandler::sPresentMode;
vk::Format SwapchainHandler::sImageFormat;
std::vector<vk::Image> SwapchainHandler::sImages;
std::vector<vk::ImageView> SwapchainHandler::sViews;

void SwapchainHandler::CreateContext(vk::Device& device, GPUDevice* gpu, vk::SurfaceKHR& surface, vk::PresentModeKHR present_mode) {
    auto swapchainSupport = gpu->GetSwapchainSupport();

    ChooseSurfaceFormat(swapchainSupport.formats);
    ChoosePresentMode(swapchainSupport.presentModes, present_mode);
    ChooseExtent(swapchainSupport.capabilities);

    // Set the image count to the minimum plus one so we don't have to wait on the driver.
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

    // Account for the special case where the minumum image count equals the maximum image count.
    // Zero is a special case that means there is no maximum.
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
        imageCount = swapchainSupport.capabilities.maxImageCount;
    
    vk::SwapchainCreateInfoKHR info = {
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = sSurfaceFormat.format,
        .imageColorSpace = sSurfaceFormat.colorSpace,
        .imageExtent = sExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = swapchainSupport.capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = sPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    // If the indices are all not the same, then change the info data.
    if (!gpu->GetQueueFamilies()->IsSame()) {
        info.imageSharingMode = vk::SharingMode::eConcurrent;
        info.queueFamilyIndexCount = gpu->GetQueueFamilies()->UniqueSize();
        info.pQueueFamilyIndices = gpu->GetQueueFamilies()->Data();
    }

    sSwapchain = device.createSwapchainKHR(info);

    // Load the swap chain images.
    sImages = device.getSwapchainImagesKHR(sSwapchain);
    sImageFormat = sSurfaceFormat.format;
}

void SwapchainHandler::CreateImageViews(vk::Device& device) {
    sViews.resize(sImages.size());
    for (size_t i = 0; i < sViews.size(); i++) {
        vk::ImageViewCreateInfo info = {
            .image = sImages[i],
            .viewType = vk::ImageViewType::e2D, // 2D textures.
            .format = sImageFormat,
            .components = {
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity
            },
            // Set images such that they are accessed simply, no mipmapping or multiple layers.
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        sViews[i] = device.createImageView(info);
    }
}

void SwapchainHandler::ChooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR>& available_formats) {
    // Check for the best format.
    for (const auto& availableFormat : available_formats)
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            sSurfaceFormat = availableFormat;
            return;
        }

    // Use the first format if the best one is not present.
    sSurfaceFormat = available_formats[0];
}

void SwapchainHandler::ChoosePresentMode(std::vector<vk::PresentModeKHR>& available_modes, vk::PresentModeKHR prefered_mode) {
    // Checks if the prefered present mode is usablle. If not, then VSYNC/Fifo is used instead.
    for (const auto& availablePresentMode : available_modes)
        if (availablePresentMode == prefered_mode) {
            sPresentMode = availablePresentMode;
            return;
        }

    sPresentMode = vk::PresentModeKHR::eFifo;
}

void SwapchainHandler::ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    // If extent is at the maximum value, return the extent as is.
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        sExtent = capabilities.currentExtent;
        return;
    }

    glm::ivec2 windowSize = Window::GetDisplaySize();
    // Get the width and height of the window.
    VkExtent2D actualExtent = {
        .width = static_cast<uint32_t>(windowSize.x),
        .height = static_cast<uint32_t>(windowSize.y)
    };

    // Clamp the values to be within the maximum capabilities for the extent.
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    sExtent = actualExtent;
}
