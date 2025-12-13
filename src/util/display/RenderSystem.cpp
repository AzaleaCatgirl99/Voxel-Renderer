#include "util/display/RenderSystem.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_filesystem.h>
#include <cstring>
#include <functional>
#include <optional>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "util/display/device/GPUDevice.h"
#include "util/display/device/SwapchainHandler.h"
#include "util/display/Window.h"
#include "util/display/vulkan/VkConversions.h"
#include "util/display/vulkan/VkDebugger.h"
#include "util/display/vulkan/VkInitializer.h"
#include "util/display/vulkan/VkResultHandler.h"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_beta.h>

RenderSystem::Settings RenderSystem::sSettings;

GPUDevice RenderSystem::sGPU;

// Logical Device variables.
vk::Device RenderSystem::sDevice;
vk::Queue RenderSystem::sGraphicsQueue;
vk::Queue RenderSystem::sPresentQueue;
vk::Queue RenderSystem::sTransferQueue;
vk::CommandPool RenderSystem::sGraphicsCmdPool;
vk::CommandPool RenderSystem::sTransferCmdPool;

vk::Instance RenderSystem::sInstance;
vk::SurfaceKHR RenderSystem::sSurface;
vk::RenderPass RenderSystem::sDefaultRenderPass;
uint32_t RenderSystem::sCurrentFrame = 0;

vk::CommandBuffer RenderSystem::sCommandBuffers[VXL_RS_MAX_FRAMES_IN_FLIGHT];
vk::Semaphore RenderSystem::sImageAvailableSemaphores[VXL_RS_MAX_FRAMES_IN_FLIGHT];
vk::Fence RenderSystem::sInFlightFences[VXL_RS_MAX_FRAMES_IN_FLIGHT];

std::vector<vk::Semaphore> RenderSystem::sRenderFinishedSemaphores;
std::vector<vk::Framebuffer> RenderSystem::sFramebuffers;

Logger RenderSystem::sLogger = Logger("RenderSystem");

void RenderSystem::Initialize(const Settings& settings) {
    sSettings = settings;

    CreateInstance();
#ifdef VXL_RENDERSYSTEM_DEBUG
    VkDebugger::CreateMessenger(sInstance);
#endif
    sSurface = Window::CreateSurface(sInstance);
    sGPU.Build(sInstance, sSurface);
    sDevice = VkInitializer::CreateDevice(&sGPU, sGraphicsQueue, sPresentQueue,
                            sTransferQueue, sGraphicsCmdPool, sTransferCmdPool);
    SwapchainHandler::Build(sDevice, &sGPU, sSurface, VkConversions::GetPresentMode(sSettings.swapInterval));

    for (uint32_t i = 0; i < VXL_RS_MAX_FRAMES_IN_FLIGHT; i++)
        sCommandBuffers[i] = CreateCmdBuffer();

    sDefaultRenderPass = VkInitializer::CreateSimpleRenderPass(sDevice);
    CreateFramebuffers();
    CreateSyncObjects();
}

void RenderSystem::Destroy() {
    for (size_t i = 0; i < VXL_RS_MAX_FRAMES_IN_FLIGHT; i++) {
        sDevice.destroySemaphore(sImageAvailableSemaphores[i]);
        sDevice.destroyFence(sInFlightFences[i]);
    }

    for (vk::Semaphore& semaphore : sRenderFinishedSemaphores)
        sDevice.destroySemaphore(semaphore);

    for (auto framebuffer : sFramebuffers)
        sDevice.destroyFramebuffer(framebuffer);

    sDevice.destroyRenderPass(sDefaultRenderPass);

    SwapchainHandler::Delete(sDevice);

    sDevice.destroyCommandPool(sGraphicsCmdPool);
    sDevice.destroyCommandPool(sTransferCmdPool);

    sDevice.destroy();

    Window::DestroySurface(sInstance, sSurface);

#ifdef VXL_RENDERSYSTEM_DEBUG
    VkDebugger::Destroy(sInstance);
#endif

    sInstance.destroy();
}

void RenderSystem::RecreateSwapchain() {
    if (Window::IsMinimized())
        return;

    sDevice.waitIdle();

    for (auto framebuffer : sFramebuffers)
        sDevice.destroyFramebuffer(framebuffer);

    SwapchainHandler::Rebuild(sDevice, &sGPU, sSurface, VkConversions::GetPresentMode(sSettings.swapInterval));

    CreateFramebuffers();
}

void RenderSystem::UpdateDisplay() {
    vk::Result result0 = sDevice.waitForFences(1, &sInFlightFences[sCurrentFrame], VK_TRUE, UINT64_MAX);
    VkResultHandler::CheckResult(result0, "Failed to wait for in flight fence!");

    auto [result, imageIndex] = sDevice.acquireNextImageKHR(SwapchainHandler::sSwapchain, UINT64_MAX, sImageAvailableSemaphores[sCurrentFrame]);
    if (result == vk::Result::eErrorOutOfDateKHR) {
        RecreateSwapchain();
        return;
    } else {
        VkResultHandler::CheckResult(result, "Failed to get image index!");
    }

    result = sDevice.resetFences(1, &sInFlightFences[sCurrentFrame]);
    VkResultHandler::CheckResult(result, "Failed to reset in flight fence!");

    sCommandBuffers[sCurrentFrame].reset();
    BeginRecordCmdBuffer(sCommandBuffers[sCurrentFrame], imageIndex);

    // Calls the command callback, which stores all of the commands for rendering.
    sSettings.cmdCallback(sCommandBuffers[sCurrentFrame]);

    sCommandBuffers[sCurrentFrame].endRenderPass();
    sCommandBuffers[sCurrentFrame].end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &sImageAvailableSemaphores[sCurrentFrame],
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &sCommandBuffers[sCurrentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &sRenderFinishedSemaphores[imageIndex]
    };

    result = sGraphicsQueue.submit(1, &submitInfo, sInFlightFences[sCurrentFrame]);
    VkResultHandler::CheckResult(result, "Failed to submit command buffer!");

    vk::PresentInfoKHR presentInfo = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &sRenderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &SwapchainHandler::sSwapchain,
        .pImageIndices = &imageIndex,
        .pResults = VK_NULL_HANDLE
    };

    result = sPresentQueue.presentKHR(&presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
        RecreateSwapchain();
    else
        VkResultHandler::CheckResult(result, "Failed to present to the present queue!");

    // Advance to the next frame.
    sCurrentFrame = (sCurrentFrame + 1) % VXL_RS_MAX_FRAMES_IN_FLIGHT;
}

vk::CommandBuffer RenderSystem::CreateCmdBuffer(std::optional<vk::CommandPool> pool) {
    // The C version of this is required in order to allocate a buffer without using a vector.
    VkCommandBuffer buffer = VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool.has_value() ? pool.value() : sGraphicsCmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(sDevice, &info, &buffer);
    return buffer;
}

vk::CommandBuffer RenderSystem::BeginDataTransfer() {
    vk::CommandBuffer buffer = CreateCmdBuffer(sTransferCmdPool);

    vk::CommandBufferBeginInfo info = {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    buffer.begin(info);
    return buffer;
}

void RenderSystem::EndDataTransfer(vk::CommandBuffer& buffer) {
    buffer.end();

    vk::SubmitInfo info = {
        .commandBufferCount = 1,
        .pCommandBuffers = &buffer
    };

    vk::Result result = sTransferQueue.submit(1, &info, VK_NULL_HANDLE);
    sTransferQueue.waitIdle();

    sDevice.freeCommandBuffers(sTransferCmdPool, 1, &buffer);
}

RenderSystem::UBO RenderSystem::CreateUniformBuffer(vk::DeviceSize size) {
    UBO ubo;
    
    for (uint32_t i = 0; i < VXL_RS_MAX_FRAMES_IN_FLIGHT; i++) {
        ubo.size = size;

        ubo.buffers[i] = CreateBuffer(vk::SharingMode::eExclusive, size, {}, vk::BufferUsageFlagBits::eUniformBuffer);
        ubo.memory[i] = CreateMemory(ubo.buffers[i],
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        // Data needs to be mapped in order to change consistently.
        ubo.ptrs[i] = MapMemory(ubo.memory[i], size);
    }

    return ubo;
}

vk::DeviceMemory RenderSystem::CreateMemory(vk::Buffer& buffer, vk::MemoryPropertyFlags properties, bool bind) {
    vk::MemoryRequirements memoryRequirements = sDevice.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo info = {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = sGPU.FindMemoryType(memoryRequirements.memoryTypeBits, properties)
    };

    vk::DeviceMemory memory = sDevice.allocateMemory(info);
    if (bind)
        BindMemory(buffer, memory);

    return memory;
}

void RenderSystem::AllocateStagedMemory(vk::Buffer& buffer, vk::DeviceMemory& memory, const void* data, vk::DeviceSize size) {
    // A staging buffer is needed in order to move over the data to the GPU.
    vk::Buffer stagingBuffer = CreateBuffer(
        sGPU.GetQueueFamilies()->IsSame() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
        size, {}, vk::BufferUsageFlagBits::eTransferSrc);
    vk::DeviceMemory stagingMemory = CreateMemory(stagingBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    AllocateMemory(stagingMemory, data, size);
    CopyBuffer(buffer, stagingBuffer, size);

    sDevice.freeMemory(stagingMemory);
    sDevice.destroyBuffer(stagingBuffer);
}

void RenderSystem::CopyBuffer(vk::Buffer& dst, vk::Buffer& src, vk::DeviceSize size, vk::DeviceSize src_offset, vk::DeviceSize dst_offset) {
    vk::CommandBuffer buffer = BeginDataTransfer();

    // Copies over the data.
    vk::BufferCopy copyRegion = {
        .srcOffset = src_offset,
        .dstOffset = dst_offset,
        .size = size
    };
    buffer.copyBuffer(src, dst, copyRegion);

    EndDataTransfer(buffer);
}

void RenderSystem::CreateDescriptorSets(vk::DescriptorSet* sets, uint32_t count, vk::DescriptorPool& pool, vk::DescriptorSetLayout* layouts) {
    vk::DescriptorSetAllocateInfo info = {
        .descriptorPool = pool,
        .descriptorSetCount = count,
        .pSetLayouts = layouts
    };

    // It creates a vector, however it will not be used.
    auto vec = sDevice.allocateDescriptorSets(info);
    for (uint32_t i = 0; i < count; i++)
        sets[i] = vec[i];
}

void RenderSystem::CreateInstance() {
    std::vector<const char*> extensions = GetRequiredExtensions();

#ifdef VXL_DEBUG
    if (!VkDebugger::CheckValidationLayerSupport())
        throw sLogger.RuntimeError("Not all of the requested validation layers are available!");
#endif

    vk::ApplicationInfo appInfo = {
        .pApplicationName = VXL_PROJECT_NAME,
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "RenderSystem",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = vk::ApiVersion14
    };

    vk::InstanceCreateInfo info = {
#ifdef SDL_PLATFORM_APPLE
        .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, // Apple devices support flag.
#endif
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

#ifdef VXL_DEBUG
    VkDebugger::Initialize(info); // Initializes the debugger.
#endif
    
    sInstance = vk::createInstance(info);
}

std::vector<const char*> RenderSystem::GetRequiredExtensions() {
    // Identify extension information.
    uint32_t sdlExtensionCount = 0;
    const char* const* sdlExtensions;

    // https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions
    sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    // Convert to vector so we can add more extensions.
    std::vector<const char*> extensions = std::vector<const char*>(sdlExtensions, sdlExtensions + sdlExtensionCount);

    // Add support for Apple devices.
#ifdef SDL_PLATFORM_APPLE
    extensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
    extensions.emplace_back("VK_KHR_get_physical_device_properties2");
#endif

#ifdef VXL_DEBUG
    // Add extension for validation layer message callback.
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

void RenderSystem::CreateFramebuffers() {
    sFramebuffers.resize(SwapchainHandler::sViews.size());

    for (size_t i = 0; i < sFramebuffers.size(); i++) {
        vk::FramebufferCreateInfo info = {
            .renderPass = sDefaultRenderPass,
            .attachmentCount = 1,
            .pAttachments = &SwapchainHandler::sViews[i],
            .width = SwapchainHandler::sExtent.width,
            .height = SwapchainHandler::sExtent.height,
            .layers = 1
        };

        sFramebuffers[i] = sDevice.createFramebuffer(info);
    }
}

void RenderSystem::CreateSyncObjects() {
    sRenderFinishedSemaphores.resize(SwapchainHandler::sImages.size());

    vk::SemaphoreCreateInfo semaphoreCreateInfo;

    vk::FenceCreateInfo fenceCreateInfo = {
        .flags = vk::FenceCreateFlagBits::eSignaled
    };

    for (size_t i = 0; i < VXL_RS_MAX_FRAMES_IN_FLIGHT; i++) {
        sImageAvailableSemaphores[i] = sDevice.createSemaphore(semaphoreCreateInfo);
        sInFlightFences[i] = sDevice.createFence(fenceCreateInfo);
    }

    for (size_t i = 0; i < SwapchainHandler::sImages.size(); i++)
        sRenderFinishedSemaphores[i] = sDevice.createSemaphore(semaphoreCreateInfo);

    sLogger.Info("Created sync objects.");
}

void RenderSystem::BeginRecordCmdBuffer(vk::CommandBuffer& commandBuffer, uint32_t imageIndex) {
    vk::CommandBufferBeginInfo beginInfo;

    commandBuffer.begin(beginInfo);

    vk::ClearValue color = {{{{0.0f, 0.0f, 0.0f, 1.0f}}}};

    // Start drawing by beginning a render pass.
    vk::RenderPassBeginInfo renderPassInfo = {
        .renderPass = sDefaultRenderPass,
        .framebuffer = sFramebuffers[imageIndex],
        // Keep the render area the same size as the images for the best performance.
        .renderArea = {
            .offset = {0, 0},
            .extent = SwapchainHandler::sExtent
        },
        .clearValueCount = 1,
        .pClearValues = &color
    };

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}
