#pragma once

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>

// Logger utility for better handling logging. Uses a name for the specific caller.
class Logger final {
public:
    VXL_INLINE Logger() = default;
    VXL_INLINE Logger(const char* name) {
        m_name = name;
    }

    template<typename... Args>
    VXL_INLINE void Print(const Args&... args) {
        (std::cout << ... << args);
    }

    template<typename... Args>
    VXL_INLINE void Println(const Args&... args) {
        (std::cout << ... << args) << std::endl;
    }

    template<typename... Args>
    VXL_INLINE void Verbose(const Args&... args) {
#ifdef VXL_VERBOSE_LOGGING
        InternalLog<Args...>(args..., "VERBOSE");
#endif
    }

    template<typename... Args>
    VXL_INLINE void Info(const Args&... args) {
#ifdef VXL_INFO_LOGGING
        InternalLog<Args...>(args..., "INFO");
#endif
    }

    template<typename... Args>
    VXL_INLINE void Warning(const Args&... args) {
#ifdef VXL_WARNING_LOGGING
        InternalLog<Args...>(args..., "WARNING");
#endif
    }

    template<typename... Args>
    VXL_INLINE void Error(const Args&... args) {
#ifdef VXL_ERROR_LOGGING
        InternalLog<Args...>(args..., "ERROR");
#endif
    }

    template<typename... Args>
    VXL_INLINE std::exception Exception(const Args&... args) {
        return std::exception(GetStr<Args...>(args..., "EXCEPTION"));
    }

    template<typename... Args>
    VXL_INLINE std::runtime_error RuntimeError(const Args&... args) {
        return std::runtime_error(GetStr<Args...>(args..., "RUNTIME_ERROR"));
    }
private:
    // Internal log function.
    template<typename... Args>
    VXL_INLINE void InternalLog(const Args&... args, const char* level) {
        std::cout << "[" << m_name << "/" << level << "] ";
        (std::cout << ... << args) << std::endl;
    }

    // Gets the string for the logging.
    template<typename... Args>
    VXL_INLINE std::string GetStr(const Args&... args, const char* level) {
        std::stringstream stream;

        stream << "[" << m_name << "/" << level << "] ";
        (stream << ... << args) << std::endl;

        return stream.str();
    }

    const char* m_name;
};
