#pragma once

#include <vector>
#include <cstdint>

class ChunkMesh {
public:
    struct Naive {
        std::vector<uint32_t> m_vertices;

        Naive() {
            m_vertices.reserve(32768);
        }
    };

    struct HyperGreedy {
        std::vector<uint32_t> m_vertices;
        std::vector<uint32_t> m_mortonBlocks;

        HyperGreedy() = default;
    };
};
