#pragma once

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <cstddef>
#include <cstdint>
#include <string>

// Buffered class utility for handling files.
class BufferedFile final {
public:
    VXL_INLINE BufferedFile() = default;
    VXL_INLINE BufferedFile(const std::string& path) {
        m_data = SDL_LoadFile(path.c_str(), &m_size);
    }
    VXL_INLINE ~BufferedFile() {
        SDL_free(m_data);
    }

    VXL_INLINE const char* Data() const noexcept {
        return static_cast<const char*>(m_data);
    }

    VXL_INLINE const uint32_t* DataAsUInt32() const noexcept {
        return reinterpret_cast<const uint32_t*>(m_data);
    }

    VXL_INLINE const size_t Size() const noexcept {
        return m_size;
    }
private:
    void* m_data = nullptr;
    size_t m_size = 0;
};
