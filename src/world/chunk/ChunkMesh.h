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

    struct Greedy {
        std::vector<uint32_t> m_vertices;

        Greedy() = default;
    };
};
