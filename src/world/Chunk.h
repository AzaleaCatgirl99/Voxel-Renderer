#pragma once

#include <vector>
#include <cstdint>
#include <bitset>
#include <unordered_map>
#include "util/Logger.h"
#include <variant>
#include <util/SparseVector.h>
#include <world/ChunkMesh.h>
#include <world/ChunkPacking.h>
#include <world/ChunkData.h>

class Chunk {
public:
    Chunk(ChunkPacking packingType);

    uint16_t GetBlock(const uint8_t x, const uint8_t y, const uint8_t z) const;

    uint16_t SetBlock(const uint16_t newBlock, const uint8_t x, const uint8_t y, const uint8_t z);

    ChunkMesh::Naive MeshNaive();
private:
    static Logger sLogger;

    ChunkPacking m_packingMode;

    SparseVector<uint16_t, uint8_t> m_blockPalette; // List of block IDs.

    // Sacrifice a little bit of memory for faster deletes. Hash maps are way too slow.
    std::vector<uint8_t> m_blockPaletteIndices;
    std::vector<uint16_t> m_blockPaletteCounts;

    bool m_hasAir;

    std::vector<bool> m_airBlocks;

    ChunkData m_blockData;
};
