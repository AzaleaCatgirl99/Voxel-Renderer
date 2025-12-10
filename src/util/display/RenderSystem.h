#pragma once

#include "util/display/pipeline/Type.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <optional>
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "util/Logger.h"
#include "util/display/device/VkDeviceHandler.h"
#include "util/display/device/GPUDevice.h"
#include <vector>

class VertexFormat;

// Rendering implementation that uses Vulkan.
// TODO add anisotropic filtering check
class RenderSystem final {
public:
    static VXL_INLINE const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
#ifdef VXL_RENDERSYSTEM_DEBUG
    static VXL_INLINE uint32_t LAYER_COUNT = 1;
    static VXL_INLINE const char* LAYERS[LAYER_COUNT] = {
        "VK_LAYER_KHRONOS_validation"
    };
    static PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
    static PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;
#endif

    // Various different rendering swap intervals.
    // https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentModeKHR.html
    enum class SwapInterval {
        eImmediate,
        eVSync,
        eTripleBuffering
    };

    struct Settings {
        SwapInterval swapInterval;
        std::function<void(vk::CommandBuffer*)> cmdCallback;
    };

    // Struct for storing uniform buffer data.
    struct UBO {
        uint32_t size = 0;
        vk::Buffer buffers[MAX_FRAMES_IN_FLIGHT];
        vk::DeviceMemory memory[MAX_FRAMES_IN_FLIGHT];
        void* ptrs[MAX_FRAMES_IN_FLIGHT];

        VXL_INLINE void Update(const void* data) {
            memcpy(ptrs[sCurrentFrame], data, size);
        }

        VXL_INLINE void Destroy() {
            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                sDevice.destroyBuffer(buffers[i]);
                sDevice.freeMemory(memory[i]);
            }
        }
    };

    // Struct for storing pipeline data.
    struct Pipeline {
        // Info struct for creating a pipeline.
        struct Info {
            std::string vertexShaderPath;
            std::string fragmentShaderPath;
            vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
            vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
            bool blending = false;
            vk::BlendFactor colorSrcFactor = vk::BlendFactor::eOne;
            vk::BlendFactor colorDstFactor = vk::BlendFactor::eZero;
            vk::BlendFactor alphaSrcFactor = vk::BlendFactor::eOne;
            vk::BlendFactor alphaDstFactor = vk::BlendFactor::eZero;
            vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
            bool useVertexFormat = false;
            const VertexFormat* vertexFormat;
            vk::Viewport viewport;
            vk::Rect2D scissor;
            vk::DescriptorSetLayoutBinding* bindings = nullptr;
            uint32_t bindingCount = 0;
            vk::DynamicState* dynamicStates = nullptr;
            uint32_t dynamicStateCount = 0;
        };

        VXL_INLINE operator vk::Pipeline&() noexcept {
            return pipeline;
        }

        VXL_INLINE void Destroy() {
            if (descriptorSetLayout != VK_NULL_HANDLE)
                sDevice.destroyDescriptorSetLayout(descriptorSetLayout);

            sDevice.destroyPipeline(pipeline);
            sDevice.destroyPipelineLayout(layout);
        }

        
        vk::Pipeline pipeline;
        vk::PipelineLayout layout;
        vk::DescriptorSetLayout descriptorSetLayout;
    };

    static void Initialize(const Settings& settings);
    static void Destroy();
    static void RecreateSwapchain();
    static void UpdateDisplay();

    // ========== Commands ==========

    static std::vector<vk::CommandBuffer> CreateCmdBuffers(uint32_t count, std::optional<vk::CommandPool> pool = std::nullopt);
    static vk::CommandBuffer CreateCmdBuffer(std::optional<vk::CommandPool> pool = std::nullopt);
    static vk::CommandBuffer BeginDataTransfer();
    static void EndDataTransfer(vk::CommandBuffer& buffer);

    // ========== Memory & Buffers ==========

    static vk::Buffer CreateBuffer(vk::SharingMode mode, uint32_t size, vk::BufferCreateFlags flags, vk::BufferUsageFlags usages);
    static vk::Buffer CreateVertexBuffer(vk::DeviceSize size, const VertexFormat format);
    static vk::Buffer CreateIndexBuffer(vk::DeviceSize size, vk::IndexType type = vk::IndexType::eUint16);
    static vk::Buffer CreateStorageBuffer(vk::DeviceSize size);
    static UBO CreateUniformBuffer(vk::DeviceSize size);
    static vk::DeviceMemory CreateMemory(vk::Buffer& buffer, vk::MemoryPropertyFlags properties, bool bind = true);
    static void AllocateStagedMemory(vk::Buffer& buffer, vk::DeviceMemory& memory, const void* data, vk::DeviceSize size);
    static void CopyBuffer(vk::Buffer& dst, vk::Buffer& src, vk::DeviceSize size, vk::DeviceSize src_offset = 0, vk::DeviceSize dst_offset = 0);

    static VXL_INLINE void BindMemory(vk::Buffer& buffer, vk::DeviceMemory& memory, vk::DeviceSize offset = 0) {
        sDevice.bindBufferMemory(buffer, memory, offset);
    }

    static VXL_INLINE void* MapMemory(vk::DeviceMemory& memory, vk::DeviceSize size, vk::DeviceSize offset = 0) {
        return sDevice.mapMemory(memory, offset, size);
    }

    static VXL_INLINE void UnmapMemory(vk::DeviceMemory& memory) {
        sDevice.unmapMemory(memory);
    }

    static VXL_INLINE void AllocateMemory(vk::DeviceMemory& memory, const void* data, vk::DeviceSize size, vk::DeviceSize offset = 0) {
        void* ptr = MapMemory(memory, size, offset);
        memcpy(ptr, data, size);
        UnmapMemory(memory);
    }

    // ========== Pipeline & Descriptors ==========

    static Pipeline CreatePipeline(Pipeline::Info& info);
    static vk::DescriptorPool CreateDescriptorPool(vk::DescriptorPoolSize* sizes, uint32_t size_count, uint32_t sets);
    static void CreateDescriptorSets(vk::DescriptorSet* sets, uint32_t count, vk::DescriptorPool& pool, vk::DescriptorSetLayout* layouts);

    // ========== Getters & Setters ==========

    static VXL_INLINE const uint32_t GetCurrentFrame() noexcept {
        return sCurrentFrame;
    }

    static VXL_INLINE vk::RenderPass& GetRenderPass() noexcept {
        return sRenderPass;
    }

    static VXL_INLINE vk::CommandBuffer& GetCurrentCmdBuffer() noexcept {
        return sCommandBuffers[sCurrentFrame];
    }

    static VXL_INLINE vk::Device& GetDevice() noexcept {
        return sDevice;
    }
private:
    static VXL_INLINE const vk::ApplicationInfo APP_INFO = {
        .pApplicationName = VXL_PROJECT_NAME,
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "RenderSystem",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    static Settings sSettings;

    // Physical Device variables.
    static GPUDevice sGPU;

    // Logical Device variables.
    static vk::Device sDevice;
    static vk::Queue sGraphicsQueue;
    static vk::Queue sPresentQueue;
    static vk::Queue sTransferQueue;
    static vk::CommandPool sGraphicsCmdPool;
    static vk::CommandPool sTransferCmdPool;

    static vk::Instance sInstance;
#ifdef VXL_RENDERSYSTEM_DEBUG
    static vk::DebugUtilsMessengerEXT sDebugMessenger;
    static vk::DebugUtilsMessengerCreateInfoEXT sDebugCreateInfo;
#endif
    static vk::SurfaceKHR sSurface;
    static vk::RenderPass sRenderPass;
    static uint32_t sCurrentFrame;

    static vk::CommandBuffer sCommandBuffers[MAX_FRAMES_IN_FLIGHT];
    static vk::Semaphore sImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    static vk::Fence sInFlightFences[MAX_FRAMES_IN_FLIGHT];
    static uint32_t sImageIndex;

    static std::vector<vk::Semaphore> sRenderFinishedSemaphores;
    static std::vector<vk::Framebuffer> sFramebuffers;

    static Logger sLogger;

    static vk::ClearValue sClearColor;

    static void CreateInstance();
    static std::vector<const char*> GetRequiredExtensions();
#ifdef VXL_RENDERSYSTEM_DEBUG
    static bool CheckValidationLayerSupport();
    // Annoyingly this needs to be placed here for it to be used in 'DEBUG_MESSENGER_INFO'.
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);
    static void CreateDebugMessenger();
#endif
    static void CreateDevice();
    static void CreateRenderPass();
    static void CreateFramebuffers();
    static void CreateCommandBuffer();
    static void CreateSyncObjects();
    static void BeginRecordCmdBuffer(vk::CommandBuffer& buffer, uint32_t imageIndex);
    static vk::ShaderModule CreateShader(const char* path);
    static uint32_t GetIndexTypeSize(vk::IndexType type);
    static vk::PresentModeKHR GetPresentMode(SwapInterval interval);
    static vk::Format GetTypeFormat(DataType type);
};
