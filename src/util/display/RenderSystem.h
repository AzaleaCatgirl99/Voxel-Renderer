#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vulkan/vulkan.h>
#include "util/Constants.h"
#include "util/Features.h"
#include "util/Logger.h"
#include "util/display/Window.h"
#include "util/display/device/GPUDevice.h"
#include "util/display/device/RenderDevice.h"
#include <vector>

class VertexBuffer;
class IndexBuffer;
class UniformBuffer;
class GraphicsPipeline;

// Renderer implementation that uses Vulkan.
// TODO add anisotropic filtering check
class RenderSystem final {
public:
    static constexpr const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    #ifdef VXL_DEBUG
        static constexpr const bool ENABLE_VALIDATION_LAYERS = true;
#else
        static constexpr const bool ENABLE_VALIDATION_LAYERS = false;
#endif
    static constexpr const uint32_t ENABLED_SEVERITY_FLAGS =
#if VXL_RENDERER_VERBOSE_LOG == VXL_TRUE
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
#endif
#if VXL_RENDERER_INFO_LOG == VXL_TRUE
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
#endif
#if VXL_RENDERER_WARNING_LOG == VXL_TRUE
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
#endif
#if VXL_RENDERER_ERROR_LOG == VXL_TRUE
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
#endif
    0;

    static constexpr uint32_t LAYER_COUNT = 1;
    static constexpr const char* LAYERS[LAYER_COUNT] = {
        "VK_LAYER_KHRONOS_validation"
    };

    struct Settings {
        eRenderSwapInterval m_swapInterval;
    };

    static void Initialize(const Settings& settings);
    static void Destroy();
    static void RecreateSwapChain();
    static void UpdateDisplay();

    static constexpr VkCommandBuffer BeginDataTransfer() {
        VkCommandBuffer buffer = sDevice.CreateCmdBuffer(sDevice.GetTransferCmdPool());

        VkCommandBufferBeginInfo info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        vkBeginCommandBuffer(buffer, &info);

        return buffer;
    }

    static constexpr void EndDataTransfer(VkCommandBuffer buffer) {
        vkEndCommandBuffer(buffer);

        VkSubmitInfo info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &buffer
        };

        vkQueueSubmit(sDevice.GetTransferQueue(), 1, &info, VK_NULL_HANDLE);
        vkQueueWaitIdle(sDevice.GetTransferQueue());

        sDevice.FreeCmdBuffer(buffer, sDevice.GetTransferCmdPool());
    }

    static constexpr RenderDevice* GetDevice() noexcept {
        return &sDevice;
    }

    static constexpr GPUDevice* GetGPU() noexcept {
        return &sGPU;
    }

    static constexpr void BindPipeline(GraphicsPipeline& pipeline) {
        sQueuedCommands.push_back({
            .m_type = CMD_TYPE_BIND_PIPELINE,
            .m_data = &pipeline
        });
    }

    static constexpr void BindVertexBuffer(VertexBuffer& buffer) {
        sQueuedCommands.push_back({
            .m_type = CMD_TYPE_BIND_VERTEX_BUFFER,
            .m_data = &buffer
        });
    }

    static constexpr void BindIndexBuffer(IndexBuffer& buffer) {
        sQueuedCommands.push_back({
            .m_type = CMD_TYPE_BIND_INDEX_BUFFER,
            .m_data = &buffer
        });
    }

    static constexpr void Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex = 0, uint32_t first_instance = 0) {
        sQueuedCommands.push_back({
            .m_type = CMD_TYPE_DRAW,
            .m_draw = {
                .m_drawCount = vertex_count,
                .m_instanceCount = instance_count,
                .m_drawOffset = first_vertex,
                .m_instanceOffset = first_instance
            }
        });
    }

    static constexpr void DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index = 0, uint32_t first_instance = 0) {
        sQueuedCommands.push_back({
            .m_type = CMD_TYPE_DRAW_INDEXED,
            .m_draw = {
                .m_drawCount = index_count,
                .m_instanceCount = instance_count,
                .m_drawOffset = first_index,
                .m_instanceOffset = first_instance
            }
        });
    }

    static constexpr void WaitForDeviceIdle() {
        vkDeviceWaitIdle(sDevice);
    }
private:
    friend class GraphicsPipeline;
    friend class GPUBuffer;
    friend class UniformBuffer;
    friend class GPUImage;
    friend class GPUImageView;
    friend class GPUImageSampler;

    // Annoyingly this needs to be placed here for it to be used in 'DEBUG_MESSENGER_INFO'.
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);

    static constexpr const VkApplicationInfo APP_INFO = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Voxel Renderer",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "Voxel Engine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    static constexpr const VkDebugUtilsMessengerCreateInfoEXT DEBUG_MESSENGER_INFO = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = ENABLED_SEVERITY_FLAGS,
        .messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = DebugCallback,
        .pUserData = VK_NULL_HANDLE
    };

    struct DrawData {
        uint32_t m_drawCount = 0;
        uint32_t m_instanceCount = 0;
        uint32_t m_drawOffset = 0;
        uint32_t m_instanceOffset = 0;
    };

    enum eCmdType {
        CMD_TYPE_BIND_VERTEX_BUFFER,
        CMD_TYPE_BIND_INDEX_BUFFER,
        CMD_TYPE_BIND_PIPELINE,
        CMD_TYPE_DRAW,
        CMD_TYPE_DRAW_INDEXED
    };

    struct Command {
        eCmdType m_type;
        void* m_data;
        DrawData m_draw;
    };

    static Settings sSettings;
    static RenderDevice sDevice;
    static GPUDevice sGPU;

    static VkInstance sInstance;
    static VkDebugUtilsMessengerEXT sDebugMessenger;
    static VkSurfaceKHR sSurface;
    static VkSwapchainKHR sSwapChain;
    static std::vector<VkImage> sSwapChainImages;
    static VkFormat sSwapChainImageFormat;
    static VkExtent2D sSwapChainExtent;
    static std::vector<VkImageView> sSwapChainImageViews;
    static VkRenderPass sRenderPass;
    static std::vector<VkFramebuffer> sSwapChainFramebuffers;
    static uint32_t sCurrentFrame;

    static VkCommandBuffer sCommandBuffers[MAX_FRAMES_IN_FLIGHT];
    static VkSemaphore sImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    static VkFence sInFlightFences[MAX_FRAMES_IN_FLIGHT];

    static std::vector<VkSemaphore> sRenderFinishedSemaphores;

    static uint32_t sImageIndex;

    static std::vector<const char*> sInstanceExtensions;
    static Logger sLogger;

    static std::deque<Command> sQueuedCommands;

    static constexpr void DestroyDebugUtilsMessengerEXT() {
        // Loads the function with instance proc.
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(sInstance, "vkDestroyDebugUtilsMessengerEXT");

        // Failed to load if it returns nullptr.
        if (func)
            func(sInstance, sDebugMessenger, VK_NULL_HANDLE); // Run normally.
        else
            sLogger.Error("Failed to destroy debug messenger! Unable to load vkDestroyDebugUtilsMessengerEXT.");
    }

    static constexpr void CreateSurface() {
        if (!Window::CreateSurface(sInstance, sSurface))
            throw sLogger.RuntimeError("Failed to create surface!", SDL_GetError());
        sLogger.Info("Created surface.");
    }

    static void SetupRequiredExtensions();
    static bool CheckValidationLayerSupport();
    static void CreateInstance();
    static void CreateDebugMessenger();
    static void CreateSwapChain();
    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    static VkResult CreateDebugUtilsMessengerEXT(
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        const VkAllocationCallbacks* allocator,
        VkDebugUtilsMessengerEXT* debug_messenger);
    static void CreateImageViews();
    static void CreateRenderPass();
    static void CreateFramebuffers();
    static void CreateCommandBuffer();
    static void CreateSyncObjects();
    static void BeginRecordCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    static void EndRecordCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};
