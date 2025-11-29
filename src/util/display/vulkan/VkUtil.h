#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include <SDL3/SDL_platform_defines.h>

// Utility class for making certain Vulkan tasks easier.
class VkUtil final {
public:
    // Creates a Vulkan Instance.
    static constexpr VkResult CreateInstance(VkInstance* instance, const std::vector<const char*>& extensions, const std::vector<const char*>& layers, const VkApplicationInfo& app_info, std::optional<VkDebugUtilsMessengerCreateInfoEXT> messenger_info) {
        VkInstanceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = messenger_info.has_value() ? &messenger_info.value() : VK_NULL_HANDLE,
#ifdef SDL_PLATFORM_MACOS
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR, // macOS support flag
#endif
            .pApplicationInfo = &app_info,
            .enabledLayerCount = static_cast<uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        return vkCreateInstance(&info, VK_NULL_HANDLE, instance);
    }
};
