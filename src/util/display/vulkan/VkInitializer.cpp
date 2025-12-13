#include "util/display/vulkan/VkInitializer.h"

#include "util/display/device/GPUDevice.h"
#include "util/display/device/SwapchainHandler.h"
#include "util/display/pipeline/VertexFormat.h"
#include "util/display/vulkan/VkConversions.h"
#include "util/display/vulkan/VkResultHandler.h"

Logger VkInitializer::sLogger = Logger("VkInitializer");

vk::Device VkInitializer::CreateDevice(GPUDevice* gpu, vk::Queue& graphics, vk::Queue& present,
    vk::Queue& transfer, vk::CommandPool& graphics_pool, vk::CommandPool& transfer_pool) {
    if (!gpu->GetQueueFamilies()->graphics.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No graphics family found.");

    if (!gpu->GetQueueFamilies()->present.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No presentation family found.");

    if (!gpu->GetQueueFamilies()->transfer.has_value())
        throw sLogger.RuntimeError("Failed to create logical device! No transfer family found.");

    vk::DeviceQueueCreateInfo queueCreateInfos[gpu->GetQueueFamilies()->UniqueSize()];
    float queuePriority = 1.0f;

    for (uint32_t i = 0; i < gpu->GetQueueFamilies()->UniqueSize(); i++) {
        queueCreateInfos[i] = {
            .queueFamilyIndex = gpu->GetQueueFamilies()->GetUnique(i),
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };
    }

    vk::PhysicalDeviceFeatures features = gpu->GetFeatures();

    vk::DeviceCreateInfo info = {
        .queueCreateInfoCount = static_cast<uint32_t>(gpu->GetQueueFamilies()->UniqueSize()),
        .pQueueCreateInfos = queueCreateInfos,
        .enabledExtensionCount = GPUDevice::EXTENSION_COUNT,
        .ppEnabledExtensionNames = GPUDevice::EXTENSIONS.data(),
        .pEnabledFeatures = &features
    };

    // TODO figure out how to clean up this code.
    size_t queueIndicesSize = gpu->GetQueueFamilies()->UniqueSize();

    vk::Device device = gpu->device.createDevice(info);

    graphics = device.getQueue(gpu->GetQueueFamilies()->graphics.value(), 0);
    present = device.getQueue(gpu->GetQueueFamilies()->present.value(), queueIndicesSize < 2 ? 0 : 1);

    uint32_t transferIndex = queueIndicesSize < 2 ? 0 : queueIndicesSize < 3 ? 1 : queueIndicesSize - 1;
    transfer = device.getQueue(gpu->GetQueueFamilies()->transfer.value(), transferIndex);

    vk::CommandPoolCreateInfo cmdPoolInfo = {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = gpu->GetQueueFamilies()->graphics.value()
    };

    graphics_pool = device.createCommandPool(cmdPoolInfo);

    cmdPoolInfo.queueFamilyIndex = gpu->GetQueueFamilies()->transfer.value();
    transfer_pool = device.createCommandPool(cmdPoolInfo);

    return device;
}

VkStructs::Pipeline VkInitializer::CreatePipeline(vk::Device& device, vk::RenderPass& render_pass, VkStructs::Pipeline::Info& info) {
    VkStructs::Pipeline pipeline;

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
            .inputRate = info.vertexInputRate
        };

        uint32_t offset = 0;
        for (uint32_t i = 0; i < info.vertexFormat->GetElementsSize(); i++) {
            vk::VertexInputAttributeDescription desc = {
                .location = i,
                .binding = 0,
                .format = VkConversions::GetTypeFormat(info.vertexFormat->GetElements()[i].type),
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

        pipeline.descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
    }

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
        .setLayoutCount = useDescriptorSets ? 1u : 0u,
        .pSetLayouts = useDescriptorSets ? &pipeline.descriptorSetLayout : VK_NULL_HANDLE,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = VK_NULL_HANDLE
    };

    pipeline.layout = device.createPipelineLayout(pipelineLayoutInfo);

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
        .stageCount = info.shaderStageCount,
        .pStages = info.shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterizerInfo,
        .pMultisampleState = &multisamplingInfo,
        .pDepthStencilState = VK_NULL_HANDLE,
        .pColorBlendState = &colorBlendingInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = pipeline.layout,
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    auto res = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
    VkResultHandler::CheckResult(res.result, "Failed to create graphics pipeline!");
    pipeline.pipeline = res.value;

    return pipeline;
}

vk::RenderPass VkInitializer::CreateSimpleRenderPass(vk::Device& device) {
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
        .srcSubpass = vk::SubpassExternal,
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

    return device.createRenderPass(createInfo);
}
