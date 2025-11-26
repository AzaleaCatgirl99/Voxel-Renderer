#pragma once

#include "util/Logger.h"
#include <cstddef>
#include <cstdint>
#include <vector>

// Buffered class utility for handling files.
class BufferedFile final {
public:
    static BufferedFile Read(const std::string& path, bool binary);

    BufferedFile() = default;
    constexpr BufferedFile(const std::vector<char>& data) {
        m_data = data;
    }
    constexpr BufferedFile(const char* data, size_t n) {
        m_data = std::vector<char>(data, data + n);
    }

    // Gets the file data.
    constexpr const char* Data() const noexcept {
        return m_data.data();
    }

    //Gets the file data in the format as uint32_t.
    constexpr const uint32_t* DataAsUInt32() const noexcept {
        return reinterpret_cast<const uint32_t*>(m_data.data());
    }

    // Gets the file size.
    constexpr const size_t Size() const noexcept {
        return m_data.size();
    }
private:
    static Logger sLogger;

    std::vector<char> m_data;
};
