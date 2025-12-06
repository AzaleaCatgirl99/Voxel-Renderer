#pragma once

#include "util/BufferedImage.h"
#include "util/Constants.h"
#include "util/display/buffer/GPUBuffer.h"
#include "util/display/image/GPUImage.h"
#include "util/display/image/GPUImageSampler.h"
#include "util/display/image/GPUImageView.h"
#include <SDL3/SDL_filesystem.h>
#include <string>
#include <stb_image.h>

// Class for making texture management easier.
class Texture final {
public:
    constexpr Texture(const char* path, uint32_t mipmap) {
        m_path = std::string(SDL_GetBasePath()) + "assets/textures/" + path;
        m_mipmap = mipmap;
        m_view = GPUImageView(&m_image);
        m_sampler = GPUImageSampler(RENDER_FILTER_NEAREST, RENDER_FILTER_NEAREST, RENDER_WRAP_MODE_REPEAT, mipmap);
    }

    constexpr void Build() {
        BufferedImage img(m_path, IMAGE_CHANNELS_RGBA);

        m_image = GPUImage({static_cast<uint32_t>(img.GetWidth()), static_cast<uint32_t>(img.GetHeight()),
                    1, m_mipmap + 1}, RENDER_IMAGE_TYPE_2D, RENDER_COLOR_FORMAT_RGBA,
                    RENDER_SAMPLE_COUNT_1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, // TODO fix this
                    RENDER_IMAGE_TILING_OPTIMAL, SHARING_MODE, RENDER_IMAGE_LAYOUT_UNDEFINED);
        m_image.Build();
        m_view.Build();
        m_sampler.Build();

        // A staging buffer is needed in order to move over the data to the GPU.
        GPUBuffer staging = GPUBuffer(SHARING_MODE, img.GetTotalSize())
                        .UsageFlag(GPU_BUFFER_USAGE_TRANSFER_SRC)
                        .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                        .MemoryPropertiesFlag(GPU_BUFFER_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        staging.Build();
        staging.Allocate(img.GetPixels(), img.GetTotalSize());

        m_image.CopyBuffer(staging);

        // The staging buffer is deleted once the copy is done because it is not needed anymore.
        staging.Delete();
    }

    constexpr void Delete() {
        m_sampler.Delete();
        m_view.Delete();
        m_image.Delete();
    }
private:
    friend class GPUBuffer;

#ifdef SDL_PLATFORM_APPLE // Fix for Apple devices only having one queue family.
    static constexpr const eRenderSharingMode SHARING_MODE = RENDER_SHARING_MODE_EXCLUSIVE;
#else
    static constexpr const eRenderSharingMode SHARING_MODE = RENDER_SHARING_MODE_CONCURRENT;
#endif

    std::string m_path;
    uint32_t m_mipmap = 0;
    GPUImage m_image;
    GPUImageView m_view;
    GPUImageSampler m_sampler;
};
