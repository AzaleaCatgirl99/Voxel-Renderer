#pragma once

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vulkan/vulkan.h>

// Logger utility for better handling logging. Uses a name for the specific caller.
class Logger final {
public:
    constexpr Logger() = default;
    constexpr Logger(const char* name) {
        m_name = name;
    }

    // Prints a generic log with a prefix or new line.
    template<typename... Args>
    constexpr void Print(const Args&... args) {
        (std::cout << ... << args);
    }

    // Prints a generic log with a prefix.
    template<typename... Args>
    constexpr void Println(const Args&... args) {
        (std::cout << ... << args) << std::endl;
    }

    // Logs at a verbose level.
    template<typename... Args>
    constexpr void Verbose(const Args&... args) {
        InternalLog<Args...>(args..., "VERBOSE");
    }

    // Logs at an info level.
    template<typename... Args>
    constexpr void Info(const Args&... args) {
        InternalLog<Args...>(args..., "INFO");
    }

    // Logs at an error level.
    template<typename... Args>
    constexpr void Error(const Args&... args) {
        InternalLog<Args...>(args..., "ERROR");
    }

    // Logs at a warning level.
    template<typename... Args>
    constexpr void Warning(const Args&... args) {
        InternalLog<Args...>(args..., "WARNING");
    }

    // Gets an exception.
    template<typename... Args>
    constexpr std::exception Exception(const Args&... args) {
        return std::exception(GetStr<Args...>(args..., "EXCEPTION"));
    }

    // Gets a runtime error.
    template<typename... Args>
    constexpr std::runtime_error RuntimeError(const Args&... args) {
        return std::runtime_error(GetStr<Args...>(args..., "RUNTIME_ERROR"));
    }

    // Supports:
    // Instance - https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateInstance.html
    // Debug messenger - https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/vkCreateDebugUtilsMessengerEXT.html
    // Device - https://docs.vulkan.org/refpages/latest/refpages/source/vkCreateDevice.html
    template<typename... Args>
    constexpr void InterpretVkResult(VkResult my_result, const char* my_successStr, const char* my_genericErrorStr) {
        switch(my_result) {
            case VK_SUCCESS:
                Info(my_successStr);
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                throw RuntimeError("Extension not present! ", my_genericErrorStr);
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                throw RuntimeError("Driver is not compatible! ", my_genericErrorStr);
            case VK_ERROR_INITIALIZATION_FAILED:
                throw RuntimeError("Initialization failed! ", my_genericErrorStr);
            case VK_ERROR_LAYER_NOT_PRESENT:
                throw RuntimeError("Validation layer is not present! ", my_genericErrorStr);
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                throw RuntimeError("Ran out of device memory! ", my_genericErrorStr);
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                throw RuntimeError("Ran out of host memory! ", my_genericErrorStr);
            case VK_ERROR_DEVICE_LOST:
                throw RuntimeError("Device lost! ", my_genericErrorStr);
            case VK_ERROR_FEATURE_NOT_PRESENT:
                throw RuntimeError("Feature not present! ", my_genericErrorStr);
            case VK_ERROR_TOO_MANY_OBJECTS:
                throw RuntimeError("Too many objects! ", my_genericErrorStr);
            case VK_ERROR_UNKNOWN:
                throw RuntimeError("Unknown error occurred! ", my_genericErrorStr);
            #ifndef SDL_PLATFORM_MACOS
            case VK_ERROR_VALIDATION_FAILED: // Error code does not exist on macOS
                throw RuntimeError("Validation failed! ", my_genericErrorStr);
            #endif
            default:
                throw RuntimeError("Uknown error code [", my_result, "]! ", my_genericErrorStr);
        }
    };
private:
    // Internal log function.
    template<typename... Args>
    constexpr void InternalLog(const Args&... args, const char* level) {
        std::cout << "[" << m_name << "/" << level << "] ";
        (std::cout << ... << args) << std::endl;
    }

    // Gets the string for the logging.
    template<typename... Args>
    constexpr std::string GetStr(const Args&... args, const char* level) {
        std::stringstream stream;

        stream << "[" << m_name << "/" << level << "] ";
        (stream << ... << args) << std::endl;

        return stream.str();
    }

    const char* m_name;
};
