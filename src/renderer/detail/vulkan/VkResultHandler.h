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

    // Gets the result string to use.
    static std::string GetResultString(const VkResult result);
};

}
