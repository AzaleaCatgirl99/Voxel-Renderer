#pragma once

#include <cstdint>

// Simple struct for scissors.
struct RenderScissor {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};
