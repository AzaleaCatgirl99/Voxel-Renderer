#pragma once

#include <cstddef>
#include <vector>
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_shared.hpp>

class GPUDevice;

// Class for wrapping many of the Vulkan swap chain functionality.
class SwapchainHandler final {
public:
    // ========== Building/deletion ==========

    static constexpr void Build(vk::Device& device, GPUDevice* gpu, vk::SurfaceKHR& surface, vk::PresentModeKHR present_mode) {
        CreateContext(device, gpu, surface, present_mode);
        CreateImageViews(device);
    }

    static constexpr void Delete(vk::Device& device) {
        for (auto view : sViews)
            device.destroyImageView(view);

        device.destroySwapchainKHR(sSwapchain);
    }

    static constexpr void Rebuild(vk::Device& device, GPUDevice* gpu, vk::SurfaceKHR& surface, vk::PresentModeKHR present_mode) {
        Delete(device);
        Build(device, gpu, surface, present_mode);
    }

    static vk::SwapchainKHR sSwapchain;
    static vk::Extent2D sExtent;
    static vk::SurfaceFormatKHR sSurfaceFormat;
    static vk::PresentModeKHR sPresentMode;
    static vk::Format sImageFormat;
    static std::vector<vk::Image> sImages;
    static std::vector<vk::ImageView> sViews;
private:
    static void CreateContext(vk::Device& device, GPUDevice* gpu, vk::SurfaceKHR& surface, vk::PresentModeKHR present_mode);
    static void CreateImageViews(vk::Device& device);
    static void ChooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR>& available_formats);
    static void ChoosePresentMode(std::vector<vk::PresentModeKHR>& available_modes, vk::PresentModeKHR prefered_mode);
    static void ChooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
};
