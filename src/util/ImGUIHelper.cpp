#include "util/ImGUIHelper.h"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include "util/display/RenderSystem.h"
#include "util/display/Window.h"

void ImGUIHelper::Initialize() {
    // Create ImGUI context.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForVulkan(Window::sContext);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = RenderSystem::sInstance;
    init_info.PhysicalDevice = RenderSystem::sPhysicalDevice;
    init_info.Device = RenderSystem::sDevice;
    init_info.QueueFamily = RenderSystem::sQueueFamilies.m_graphics.value();
    init_info.Queue = RenderSystem::sGraphicsQueue;
    // init_info.PipelineCache = YOUR_PIPELINE_CACHE;
    // init_info.DescriptorPool = YOUR_DESCRIPTOR_POOL;
    init_info.MinImageCount = RenderSystem::sSwapChainImages.size();
    init_info.ImageCount = RenderSystem::sSwapChainImages.size();
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.PipelineInfoMain.RenderPass = RenderSystem::sRenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&init_info);
}

void ImGUIHelper::ProcessEvents() {
    ImGui_ImplSDL3_ProcessEvent(Window::GetEvent());
}

void ImGUIHelper::BeginDraw() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
}

void ImGUIHelper::CmdDraw() {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), RenderSystem::sCommandBuffers[RenderSystem::sCurrentFrame]);
}

void ImGUIHelper::Destroy() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
