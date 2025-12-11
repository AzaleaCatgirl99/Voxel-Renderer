#pragma once

// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

class Renderer;

// App utility.
class App final {
public:
    static void Run();

    static constexpr void Close() noexcept {
        sRunning = false;
    }

    static constexpr const float DeltaTime() noexcept {
        return sDeltaTime;
    }
private:
    static bool sRunning;
    static float sDeltaTime;
    static float sLastFrame;

    static void Init();
    static void MainLoop();
    static void RenderLoop(vk::CommandBuffer& buffer);
    static void Cleanup();
};
