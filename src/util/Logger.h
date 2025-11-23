#pragma once

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>

// Logger utility for better handling logging. Uses a name for the specific caller.
class Logger final {
public:
    constexpr Logger() = default;
    constexpr Logger(const char* name) {
        m_name = name;
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
