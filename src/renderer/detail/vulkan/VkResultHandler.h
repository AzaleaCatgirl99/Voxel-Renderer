#pragma once

#include <flat_map>
#include <optional>
#include <vulkan/vulkan.h>
#include "util/Logger.h"
#include <SDL3/SDL_platform.h>

namespace detail {

// Utility class for interpreting VkResult enums.
class VkResultHandler final {
public:
    // Checks the result and either throws or continues the program.
    static void CheckResult(const VkResult result, const char* error, std::optional<const char*> success = std::nullopt);
private:
    // Logger class for printing debug information.
    static Logger sLogger;

    // The result strings used.
    static const std::flat_map<VkResult, const char*> sResultStrings;

    static constexpr bool IsSuccess(const VkResult result) noexcept {
        return result == VK_SUCCESS || result == VK_NOT_READY || result == VK_TIMEOUT || result == VK_EVENT_SET || result == VK_EVENT_RESET ||
                result == VK_INCOMPLETE || result == VK_SUBOPTIMAL_KHR || result == VK_PIPELINE_COMPILE_REQUIRED || result == VK_THREAD_IDLE_KHR ||
                result == VK_THREAD_DONE_KHR || result == VK_OPERATION_DEFERRED_KHR || result == VK_OPERATION_NOT_DEFERRED_KHR ||
#ifdef SDL_PLATFORM_APPLE
                result == VK_INCOMPATIBLE_SHADER_BINARY_EXT;
#else
                result == VK_INCOMPATIBLE_SHADER_BINARY_EXT || result == VK_PIPELINE_BINARY_MISSING_KHR;
#endif
    }

    // Gets the result string to use.
    static std::string GetResultString(const VkResult result);
};

}
