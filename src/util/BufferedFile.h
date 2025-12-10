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
    VXL_INLINE BufferedFile(const std::vector<char>& data) {
        m_data = data.data();
        m_size = data.size();
    }
    VXL_INLINE BufferedFile(const char* data, size_t n) {
        m_data = data;
        m_size = n;
    }

    VXL_INLINE const char* Data() const noexcept {
        return m_data;
    }

    VXL_INLINE const uint32_t* DataAsUInt32() const noexcept {
        return reinterpret_cast<const uint32_t*>(m_data);
    }

    VXL_INLINE const size_t Size() const noexcept {
        return m_size;
    }
private:
    static Logger sLogger;

    const char* m_data = nullptr;
    size_t m_size = 0;
};
