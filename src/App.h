#pragma once

#include <SDL3/SDL_filesystem.h>
// Makes sure to remove constructors for structs.
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

class Renderer;

// App utility.
class App final {
public:
    static void Run();

    static VXL_INLINE void Close() noexcept {
        sRunning = false;
    }

    static VXL_INLINE const float DeltaTime() noexcept {
        return sDeltaTime;
    }

    static VXL_INLINE std::string GetRootPath() {
        return std::string(SDL_GetBasePath());
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
