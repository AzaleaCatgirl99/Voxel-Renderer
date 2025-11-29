#pragma once

#include <cstdint>

enum class ChunkPacking : uint8_t {
    Zero = 0,
    One = 1,
    Two = 2,
    Four = 4,
    Eight = 8,
    Sixteen = 16
};
