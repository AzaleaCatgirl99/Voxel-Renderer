#pragma once

#include <flat_map>
#include <vulkan/vulkan.h>
#include <unordered_map>
#include "util/Logger.h"

namespace detail {

// Utility class for interpreting VkResult enums.
class VkResultHandler final {
public:
    // Identifies if a VkResult is an error code. Returns false for success codes.
    static bool TestIfError(const VkResult result);

    // Returns a throwable runtime error with debugging information specific to the error.
    static std::runtime_error InterpretError(const VkResult error, const char* errorMessage);

    // Logs a successful result. Sends a warning for codes other than VK_SUCCESS.
    static void InterpretSuccess(const VkResult success, const char* successMessage);
private:
    // Logger class for printing debug information.
    static Logger sLogger;

    // The error strings used.
    static const std::flat_map<VkResult, const char*> sErrorStrings;

    // The success strings used.
    static const std::flat_map<VkResult, const char*> sSuccessStrings;

    // Gets the relevant string for an error code.
    static std::string GetErrorString(const VkResult error);

    // Gets the relevant string for a success code.
    static std::string GetSuccessString(const VkResult error);
};

}
