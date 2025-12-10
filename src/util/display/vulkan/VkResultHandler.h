#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>
#include "util/Logger.h"
#include <SDL3/SDL_platform.h>

// Utility class for interpreting VkResult enums.
class VkResultHandler final {
public:
    // Checks the result and either throws or continues the program.
    static void CheckResult(const vk::Result result, const char* error, std::optional<const char*> success = std::nullopt);
private:
    // Logger class for printing debug information.
    static Logger sLogger;

    // Gets the result's description.
    static const char* GetResultDescription(const vk::Result result);
};
