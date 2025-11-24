#pragma once

#include "renderer/detail/IRenderer.h"
#include <vulkan/vulkan.h>
#include <util/Logger.h>
#include <vector>

namespace detail {

// Renderer implementation that uses Vulkan.
class VkRenderer : public IRenderer {
public:
    constexpr VkRenderer(Window* window) noexcept : IRenderer(window) {
    }

    virtual void Initialize() override;

    virtual void Destroy() override;
private:
    VkApplicationInfo CreateAppInfo();

    VkInstanceCreateInfo CreateInstanceInfo();

    bool CheckValidationLayerSupport();

    VkInstance m_instance;

    static Logger sLogger;
    static bool sEnableValidationLayers;
    static std::vector<const char*> sValidationLayers;
};

}
