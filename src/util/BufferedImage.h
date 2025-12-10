#pragma once

#include "util/Logger.h"
#include <cstddef>
#include <optional>
#include <stb_image.h>
#include <stb_image_write.h>
#include <string>

enum ImageChannels : int {
    eRGBA = STBI_rgb_alpha,
    eRGB = STBI_rgb,
    eGreyAlpha = STBI_grey_alpha,
    eGrey = STBI_grey,
};

// Buffered class for loading/writing/modifying images.
class BufferedImage final {
public:
    // ========== Constructors/Destructors ==========

    VXL_INLINE BufferedImage() = default;
    VXL_INLINE BufferedImage(const std::string& path, std::optional<ImageChannels> channels = std::nullopt) {
        m_path = path;

        int stride = 0;
        m_pixels = stbi_load(m_path.c_str(), &m_width, &m_height, &stride, channels.has_value() ? channels.value() : 0);
        if (m_pixels == nullptr)
            throw sLogger.RuntimeError("Failed to load pixels of image with file path '", path, "'!");
    }
    VXL_INLINE ~BufferedImage() {
        if (m_pixels != nullptr)
            stbi_image_free(m_pixels);
    }

    // ========== Getters ==========

    VXL_INLINE const std::string GetFilePath() const noexcept {
        return m_path;
    }

    VXL_INLINE const unsigned char* GetPixels() const noexcept {
        return m_pixels;
    }

    VXL_INLINE const int GetWidth() const noexcept {
        return m_width;
    }

    VXL_INLINE const int GetHeight() const noexcept {
        return m_height;
    }

    VXL_INLINE const ImageChannels GetChannels() const noexcept {
        return m_channels;
    }

    VXL_INLINE const uint32_t GetTotalSize() const noexcept {
        return m_width * m_height * m_channels;
    }

    // ========== Write Functions ==========

    VXL_INLINE void WriteAsPNG(const std::string& path) {
        stbi_write_png(path.c_str(), m_width, m_height, m_channels, m_pixels, m_width * m_channels);
    }

    VXL_INLINE void WriteAsJPG(const std::string& path) {
        stbi_write_jpg(path.c_str(), m_width, m_height, m_channels, m_pixels, m_width * m_channels);
    }

    VXL_INLINE void WriteAsBMP(const std::string& path) {
        stbi_write_bmp(path.c_str(), m_width, m_height, m_channels, m_pixels);
    }

    VXL_INLINE void WriteAsTGA(const std::string& path) {
        stbi_write_tga(path.c_str(), m_width, m_height, m_channels, m_pixels);
    }
private:
    static Logger sLogger;

    std::string m_path;
    unsigned char* m_pixels = nullptr;
    int m_width = 0;
    int m_height = 0;
    ImageChannels m_channels;
};
