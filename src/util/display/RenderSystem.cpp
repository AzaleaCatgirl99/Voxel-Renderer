#include "util/display/RenderSystem.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_filesystem.h>
#include <cstring>
#include <functional>
#include <optional>
#include <vulkan/vulkan_beta.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "util/BufferedFile.h"
#include "util/display/device/GPUDevice.h"
#include "util/display/device/SwapchainHandler.h"
#include "util/display/Window.h"
#include "util/display/vulkan/VkResultHandler.h"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include "util/display/pipeline/VertexFormat.h"

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
#ifdef VXL_RENDERSYSTEM_DEBUG
vk::DebugUtilsMessengerEXT RenderSystem::sDebugMessenger;
vk::DebugUtilsMessengerCreateInfoEXT RenderSystem::sDebugCreateInfo = {
    .messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
    .pfnUserCallback = DebugCallback,
    .pUserData = VK_NULL_HANDLE
};
PFN_vkCreateDebugUtilsMessengerEXT RenderSystem::pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT RenderSystem::pfnVkDestroyDebugUtilsMessengerEXT;
#endif
vk::SurfaceKHR RenderSystem::sSurface;
vk::RenderPass RenderSystem::sRenderPass;
uint32_t RenderSystem::sCurrentFrame = 0;

vk::CommandBuffer RenderSystem::sCommandBuffers[MAX_FRAMES_IN_FLIGHT];
vk::Semaphore RenderSystem::sImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
vk::Fence RenderSystem::sInFlightFences[MAX_FRAMES_IN_FLIGHT];
uint32_t RenderSystem::sImageIndex = 0;

std::vector<vk::Semaphore> RenderSystem::sRenderFinishedSemaphores;
std::vector<vk::Framebuffer> RenderSystem::sFramebuffers;

Logger RenderSystem::sLogger = Logger("RenderSystem");

vk::ClearValue RenderSystem::sClearColor = {{{{0.0f, 0.0f, 0.0f, 1.0f}}}};

void RenderSystem::Initialize(const Settings& settings) {
    sSettings = settings;

    CreateInstance();
#ifdef VXL_RENDERSYSTEM_DEBUG
    CreateDebugMessenger();
#endif
    sSurface = Window::CreateSurface(sInstance);
    sGPU.Build(sInstance, sSurface);
    CreateDevice();
    SwapchainHandler::Build(sDevice, &sGPU, sSurface, GetPresentMode(sSettings.swapInterval));

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        sCommandBuffers[i] = CreateCmdBuffer();

    CreateRenderPass();
    CreateFramebuffers();
    CreateSyncObjects();
}

void RenderSystem::Destroy() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        sDevice.destroySemaphore(sImageAvailableSemaphores[i]);
        sDevice.destroyFence(sInFlightFences[i]);
    }

    for (vk::Semaphore& semaphore : sRenderFinishedSemaphores)
        sDevice.destroySemaphore(semaphore);

    for (auto framebuffer : sFramebuffers)
        sDevice.destroyFramebuffer(framebuffer);

    sDevice.destroyRenderPass(sRenderPass);

    SwapchainHandler::Delete(sDevice);

    sDevice.destroyCommandPool(sGraphicsCmdPool);
    sDevice.destroyCommandPool(sTransferCmdPool);

    sDevice.destroy();

    Window::DestroySurface(sInstance, sSurface);

#ifdef VXL_RENDERSYSTEM_DEBUG
    sInstance.destroyDebugUtilsMessengerEXT(sDebugMessenger);
#endif

    sInstance.destroy();
}

void RenderSystem::RecreateSwapchain() {
    if (Window::IsMinimized())
        return;

    sDevice.waitIdle();

    for (auto framebuffer : sFramebuffers)
        sDevice.destroyFramebuffer(framebuffer);

    SwapchainHandler::Rebuild(sDevice, &sGPU, sSurface, GetPresentMode(sSettings.swapInterval));

    CreateFramebuffers();
}

void RenderSystem::UpdateDisplay() {
    vk::Result result = sDevice.waitForFences(1, &sInFlightFences[sCurrentFrame], VK_TRUE, UINT64_MAX);
    result = sDevice.resetFences(1, &sInFlightFences[sCurrentFrame]);

    auto index = sDevice.acquireNextImageKHR(SwapchainHandler::sSwapchain, 0, sImageAvailableSemaphores[sCurrentFrame]);
    VkResultHandler::CheckResult(index.result, "Failed to get image index!");
    sImageIndex = index.value;

    result = sDevice.resetFences(1, &sInFlightFences[sCurrentFrame]);

    sCommandBuffers[sCurrentFrame].reset();
    BeginRecordCmdBuffer(sCommandBuffers[sCurrentFrame], sImageIndex);

    // Calls the command callback, which stores all of the commands for rendering.
    sSettings.cmdCallback(&sCommandBuffers[sCurrentFrame]);

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
        .pSignalSemaphores = &sRenderFinishedSemaphores[sImageIndex]
    };

    result = sGraphicsQueue.submit(1, &submitInfo, sInFlightFences[sCurrentFrame]);
    VkResultHandler::CheckResult(result, "Failed to submit command buffer!");

    vk::PresentInfoKHR presentInfo = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &sRenderFinishedSemaphores[sImageIndex],
        .swapchainCount = 1,
        .pSwapchains = &SwapchainHandler::sSwapchain,
        .pImageIndices = &sImageIndex,
        .pResults = VK_NULL_HANDLE
    };

    result = sPresentQueue.presentKHR(&presentInfo);
    VkResultHandler::CheckResult(result, "Failed to present to the present queue!");

    // Advance to the next frame.
    sCurrentFrame = (sCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

std::vector<vk::CommandBuffer> RenderSystem::CreateCmdBuffers(uint32_t count, std::optional<vk::CommandPool> pool) {
    vk::CommandBufferAllocateInfo info = {
        .commandPool = pool.has_value() ? pool.value() : sGraphicsCmdPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = count
    };

    // Annoyingly HPP won't allow the use of just normal pointers for data.
    return sDevice.allocateCommandBuffers(info);
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

vk::Buffer RenderSystem::CreateBuffer(vk::SharingMode mode, uint32_t size, vk::BufferCreateFlags flags, vk::BufferUsageFlags usages) {
    vk::BufferCreateInfo info = {
        .flags = flags,
        .size = size,
        .usage = usages,
        .sharingMode = mode
    };

    return sDevice.createBuffer(info);
}

vk::Buffer RenderSystem::CreateVertexBuffer(vk::DeviceSize size, const VertexFormat format) {
    return CreateBuffer(
        sGPU.GetQueueFamilies()->IsSame() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
        size * format.GetStride(), {}, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
}

vk::Buffer RenderSystem::CreateIndexBuffer(vk::DeviceSize size, vk::IndexType type) {
    return CreateBuffer(
        sGPU.GetQueueFamilies()->IsSame() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
        size * GetIndexTypeSize(type), {}, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
}

vk::Buffer RenderSystem::CreateStorageBuffer(vk::DeviceSize size) {
    return CreateBuffer(
        sGPU.GetQueueFamilies()->IsSame() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
        size, {}, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);
}

RenderSystem::UBO RenderSystem::CreateUniformBuffer(vk::DeviceSize size) {
    UBO ubo;
    
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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

RenderSystem::Pipeline RenderSystem::CreatePipeline(Pipeline::Info& info) {
    Pipeline pipeline;

    vk::ShaderModule vertShaderModule = CreateShader(info.vertexShaderPath.c_str());
    vk::ShaderModule fragShaderModule = CreateShader(info.fragmentShaderPath.c_str());

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo = {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = vertShaderModule,
        .pName = "main"
    };
    
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo = {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = fragShaderModule,
        .pName = "main"
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo = {
        .dynamicStateCount = info.dynamicStateCount,
        .pDynamicStates = info.dynamicStates
    };

    // Checks whether the pipeline has a vertex format. If it does, then it'll setup the binding and attribute descriptions.
    vk::VertexInputBindingDescription vertexBindingDesc;
    std::vector<vk::VertexInputAttributeDescription> vertexAttribDescs;
    if (info.useVertexFormat) {
        vertexBindingDesc = {
            .binding = 0,
            .stride = static_cast<uint32_t>(info.vertexFormat->GetStride()),
            .inputRate = vk::VertexInputRate::eVertex // TODO this needs to change to setup instance rendering.
        };

        uint32_t offset = 0;
        for (uint32_t i = 0; i < info.vertexFormat->GetElementsSize(); i++) {
            vk::VertexInputAttributeDescription desc = {
                .location = i,
                .binding = 0,
                .format = GetTypeFormat(info.vertexFormat->GetElements()[i].type),
                .offset = offset
            };

            vertexAttribDescs.push_back(desc);

            offset += GetDataTypeSize(info.vertexFormat->GetElements()[i].type);
        }
    }

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
        .vertexBindingDescriptionCount = info.useVertexFormat ? 1u : 0u,
        .pVertexBindingDescriptions = info.useVertexFormat ? &vertexBindingDesc : VK_NULL_HANDLE,
        .vertexAttributeDescriptionCount = info.useVertexFormat ? static_cast<uint32_t>(vertexAttribDescs.size()) : 0u,
        .pVertexAttributeDescriptions = info.useVertexFormat ? vertexAttribDescs.data() : VK_NULL_HANDLE,
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .topology = info.topology,
        .primitiveRestartEnable = VK_FALSE
    };

    vk::PipelineViewportStateCreateInfo viewportStateInfo = {
        .viewportCount = 1, // Only need 1 viewport and scissor.
        .pViewports = &info.viewport,
        .scissorCount = 1,
        .pScissors = &info.scissor
    };

    vk::PipelineMultisampleStateCreateInfo multisamplingInfo = {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = VK_NULL_HANDLE,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentInfo = {
        .blendEnable = info.blending ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = info.colorSrcFactor,
        .dstColorBlendFactor = info.colorDstFactor,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = info.alphaSrcFactor,
        .dstAlphaBlendFactor = info.alphaDstFactor,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = 
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendingInfo = {
        .logicOpEnable = VK_FALSE,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentInfo
    };
    colorBlendingInfo.blendConstants[0] = 0.0f;
    colorBlendingInfo.blendConstants[1] = 0.0f;
    colorBlendingInfo.blendConstants[2] = 0.0f;
    colorBlendingInfo.blendConstants[3] = 0.0f;

    vk::Result result;
    bool useDescriptorSets = info.bindingCount > 0 && info.bindings;
    if (useDescriptorSets) {
        vk::DescriptorSetLayoutCreateInfo layoutInfo = {
            .bindingCount = info.bindingCount,
            .pBindings = info.bindings
        };

        pipeline.descriptorSetLayout =  sDevice.createDescriptorSetLayout(layoutInfo);
    }

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
        .setLayoutCount = useDescriptorSets ? 1u : 0u,
        .pSetLayouts = useDescriptorSets ? &pipeline.descriptorSetLayout : VK_NULL_HANDLE,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = VK_NULL_HANDLE
    };

    pipeline.layout = sDevice.createPipelineLayout(pipelineLayoutInfo);

    vk::PipelineRasterizationStateCreateInfo rasterizerInfo = {
        .depthClampEnable = VK_FALSE, // Discard off-screen fragments, do not clamp.
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = info.polygonMode,
        .cullMode = info.cullMode,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo = {
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
        .layout = pipeline.layout,
        .renderPass = sRenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    auto res = sDevice.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
    VkResultHandler::CheckResult(res.result, "Failed to create graphics pipeline!");
    pipeline.pipeline = res.value;

    sDevice.destroyShaderModule(fragShaderModule);
    sDevice.destroyShaderModule(vertShaderModule);

    return pipeline;
}

vk::DescriptorPool RenderSystem::CreateDescriptorPool(vk::DescriptorPoolSize* sizes, uint32_t size_count, uint32_t sets) {
    vk::DescriptorPoolCreateInfo info = {
        .maxSets = sets,
        .poolSizeCount = size_count,
        .pPoolSizes = sizes
    };

    return sDevice.createDescriptorPool(info);
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

#ifdef VXL_RENDERSYSTEM_DEBUG
    if (!CheckValidationLayerSupport())
        throw sLogger.RuntimeError("Not all of the requested validation layers are available!");

#ifdef VXL_VERBOSE_LOGGING
    sDebugCreateInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
#endif
#ifdef VXL_INFO_LOGGING
    sDebugCreateInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
#endif
#ifdef VXL_WARNING_LOGGING
    sDebugCreateInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
#endif
#ifdef VXL_ERROR_LOGGING
    sDebugCreateInfo.messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
#endif
#endif

    vk::InstanceCreateInfo info = {
#ifdef VXL_RENDERSYSTEM_DEBUG
        .pNext = &sDebugCreateInfo,
#endif
#ifdef SDL_PLATFORM_APPLE
        .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR, // Apple devices support flag.
#endif
        .pApplicationInfo = &APP_INFO,
#ifdef VXL_RENDERSYSTEM_DEBUG
        .enabledLayerCount = LAYER_COUNT,
        .ppEnabledLayerNames = LAYERS,
#endif
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    
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
    extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.emplace_back("VK_KHR_get_physical_device_properties2");
    #endif

#ifdef VXL_RENDERSYSTEM_DEBUG
    // Add extension for validation layer message callback.
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

#ifdef VXL_RENDERSYSTEM_DEBUG
bool RenderSystem::CheckValidationLayerSupport() {
    // Find the number of validation layers available.
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE);

    // Find the properties of the available validation layers.
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

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
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
        return VK_FALSE;
    }
}

void RenderSystem::CreateDebugMessenger() {
    // Creates the function pointers needed for Vulkan HPP.
    pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                        sInstance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                        sInstance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

    sDebugMessenger = sInstance.createDebugUtilsMessengerEXT(sDebugCreateInfo);
}
#endif

void RenderSystem::CreateDevice() {
    if (!sGPU.GetQueueFamilies()->graphics.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No graphics family found.");

    if (!sGPU.GetQueueFamilies()->present.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No presentation family found.");

    if (!sGPU.GetQueueFamilies()->transfer.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No transfer family found.");

    vk::DeviceQueueCreateInfo queueCreateInfos[sGPU.GetQueueFamilies()->UniqueSize()];
    float queuePriority = 1.0f;

    for (uint32_t i = 0; i < sGPU.GetQueueFamilies()->UniqueSize(); i++) {
        queueCreateInfos[i] = {
            .queueFamilyIndex = sGPU.GetQueueFamilies()->GetUnique(i),
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };
    }

    vk::PhysicalDeviceFeatures features = sGPU.GetFeatures();

    vk::DeviceCreateInfo info = {
        .queueCreateInfoCount = static_cast<uint32_t>(sGPU.GetQueueFamilies()->UniqueSize()),
        .pQueueCreateInfos = queueCreateInfos,
#ifdef VXL_RENDERSYSTEM_DEBUG
        .enabledLayerCount = RenderSystem::LAYER_COUNT,
        .ppEnabledLayerNames = RenderSystem::LAYERS,
#endif
        .enabledExtensionCount = GPUDevice::EXTENSION_COUNT,
        .ppEnabledExtensionNames = GPUDevice::EXTENSIONS.data(),
        .pEnabledFeatures = &features
    };

    // TODO figure out how to clean up this code.
    size_t queueIndicesSize = sGPU.GetQueueFamilies()->UniqueSize();


    sDevice = sGPU.device.createDevice(info);

    sGraphicsQueue = sDevice.getQueue(sGPU.GetQueueFamilies()->graphics.value(), 0);
    sPresentQueue = sDevice.getQueue(sGPU.GetQueueFamilies()->present.value(), queueIndicesSize < 2 ? 0 : 1);

    uint32_t transferIndex = queueIndicesSize < 2 ? 0 : queueIndicesSize < 3 ? 1 : queueIndicesSize - 1;
    sTransferQueue = sDevice.getQueue(sGPU.GetQueueFamilies()->transfer.value(), transferIndex);

    vk::CommandPoolCreateInfo cmdPoolInfo = {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = sGPU.GetQueueFamilies()->graphics.value()
    };

    sGraphicsCmdPool = sDevice.createCommandPool(cmdPoolInfo);

    cmdPoolInfo.queueFamilyIndex = sGPU.GetQueueFamilies()->transfer.value();
    sTransferCmdPool = sDevice.createCommandPool(cmdPoolInfo);
}

void RenderSystem::CreateRenderPass() {
    vk::AttachmentDescription colorAttachment = {
        .format = SwapchainHandler::sImageFormat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR
    };

    vk::AttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::SubpassDescription subpass = {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    vk::SubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits::eNone,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite
    };

    vk::RenderPassCreateInfo createInfo = {
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    sRenderPass = sDevice.createRenderPass(createInfo);
    sLogger.Info("Created Render pass.");
}

void RenderSystem::CreateFramebuffers() {
    sFramebuffers.resize(SwapchainHandler::sViews.size());

    for (size_t i = 0; i < sFramebuffers.size(); i++) {
        vk::FramebufferCreateInfo info = {
            .renderPass = sRenderPass,
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

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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

    // Start drawing by beginning a render pass.
    vk::RenderPassBeginInfo renderPassInfo = {
        .renderPass = sRenderPass,
        .framebuffer = sFramebuffers[imageIndex],
        // Keep the render area the same size as the images for the best performance.
        .renderArea = {
            .offset = {0, 0},
            .extent = SwapchainHandler::sExtent
        },
        .clearValueCount = 1,
        .pClearValues = &sClearColor
    };

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    // Sets the dynamic viewport and scissor.
    vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(SwapchainHandler::sExtent.width),
        .height = static_cast<float>(SwapchainHandler::sExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    commandBuffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor = {
        .offset = {0, 0},
        .extent = SwapchainHandler::sExtent
    };
    commandBuffer.setScissor(0, 1, &scissor);
}

vk::ShaderModule RenderSystem::CreateShader(const char* path) {
    BufferedFile file = BufferedFile::Read(path, true);

    vk::ShaderModuleCreateInfo info = {
        .codeSize = file.Size(),
        .pCode = file.DataAsUInt32()
    };

    return sDevice.createShaderModule(info);
}

uint32_t RenderSystem::GetIndexTypeSize(vk::IndexType type) {
    switch (type) {
    case vk::IndexType::eUint16:
        return sizeof(uint16_t);
    case vk::IndexType::eUint32:
        return sizeof(uint32_t);
    case vk::IndexType::eNoneKHR:
        return 0;
    case vk::IndexType::eUint8KHR:
        return sizeof(uint8_t);
    }
}

vk::PresentModeKHR RenderSystem::GetPresentMode(SwapInterval interval) {
    switch (interval) {
    case SwapInterval::eImmediate:
        return vk::PresentModeKHR::eImmediate;
    case SwapInterval::eVSync:
        return vk::PresentModeKHR::eFifo;
    case SwapInterval::eTripleBuffering:
        return vk::PresentModeKHR::eMailbox;
    }
}

vk::Format RenderSystem::GetTypeFormat(DataType type) {
    switch (type) {
    case DataType::eVec2:
        return vk::Format::eR32G32Sfloat;
    case DataType::eVec3:
        return vk::Format::eR32G32B32Sfloat;
    case DataType::eVec4:
        return vk::Format::eR32G32B32A32Sfloat;
    case DataType::eMat2:
        return vk::Format::eUndefined; // TODO figure out this one
    case DataType::eMat3:
        return vk::Format::eUndefined; // TODO figure out this one
    case DataType::eMat4:
        return vk::Format::eUndefined; // TODO figure out this one
    case DataType::eFloat:
        return vk::Format::eR32Sfloat;
    case DataType::eDouble:
        return vk::Format::eR64Sfloat;
    case DataType::eInt:
        return vk::Format::eR32Sint;
    case DataType::eUint:
        return vk::Format::eR32Sint;
    case DataType::eInt16:
    case DataType::eUint16:
        return vk::Format::eUndefined;
    }
}

// These are wrappers for functions needed by Vulkan HPP.

#ifdef VXL_RENDERSYSTEM_DEBUG
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                const VkDebugUtilsMessengerCreateInfoEXT* info,
                                                const VkAllocationCallbacks* allocator,
                                                VkDebugUtilsMessengerEXT* messenger) {
    return RenderSystem::pfnVkCreateDebugUtilsMessengerEXT(instance, info, allocator, messenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                VkDebugUtilsMessengerEXT messenger,
                                                VkAllocationCallbacks const* allocator) {
    return RenderSystem::pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, allocator);
}
#endif
