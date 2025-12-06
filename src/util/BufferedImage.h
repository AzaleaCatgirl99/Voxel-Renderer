#pragma once

#include "util/Logger.h"
#include <cstddef>
#include <optional>
#include <stb_image.h>
#include <stb_image_write.h>
#include <string>

enum eImageChannels : int {
    IMAGE_CHANNELS_RGBA = STBI_rgb_alpha,
    IMAGE_CHANNELS_RGB = STBI_rgb,
    IMAGE_CHANNELS_GREY_ALPHA = STBI_grey_alpha,
    IMAGE_CHANNELS_GREY = STBI_grey,
};

// Buffered class for loading/writing/modifying images.
class BufferedImage final {
public:
    // ========== Constructors/Destructors ==========

    constexpr BufferedImage() = default;
    constexpr BufferedImage(const std::string& path, std::optional<eImageChannels> channels = std::nullopt) {
        m_path = path;

        int stride = 0;
        m_pixels = stbi_load(m_path.c_str(), &m_width, &m_height, &stride, channels.has_value() ? channels.value() : 0);
        if (m_pixels == nullptr)
            throw sLogger.RuntimeError("Failed to load pixels of image with file path '", path, "'!");
    }
    constexpr ~BufferedImage() {
        if (m_pixels != nullptr)
            stbi_image_free(m_pixels);
    }

    // ========== Getters ==========

    constexpr const std::string GetFilePath() const noexcept {
        return m_path;
    }

    constexpr const unsigned char* GetPixels() const noexcept {
        return m_pixels;
    }

    constexpr const int GetWidth() const noexcept {
        return m_width;
    }

    constexpr const int GetHeight() const noexcept {
        return m_height;
    }

    constexpr const eImageChannels GetChannels() const noexcept {
        return m_channels;
    }

    constexpr const uint32_t GetTotalSize() const noexcept {
        return m_width * m_height * m_channels;
    }

    // ========== Write Functions ==========

    constexpr void WriteAsPNG(const std::string& path) {
        stbi_write_png(path.c_str(), m_width, m_height, m_channels, m_pixels, m_width * m_channels);
    }

    constexpr void WriteAsJPG(const std::string& path) {
        stbi_write_jpg(path.c_str(), m_width, m_height, m_channels, m_pixels, m_width * m_channels);
    }

    constexpr void WriteAsBMP(const std::string& path) {
        stbi_write_bmp(path.c_str(), m_width, m_height, m_channels, m_pixels);
    }

    constexpr void WriteAsTGA(const std::string& path) {
        stbi_write_tga(path.c_str(), m_width, m_height, m_channels, m_pixels);
    }
private:
    static Logger sLogger;

    std::string m_path;
    unsigned char* m_pixels = nullptr;
    int m_width = 0;
    int m_height = 0;
    eImageChannels m_channels;
};
