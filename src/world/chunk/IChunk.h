#pragma once

#include <vector>
#include <cstdint>
#include <bitset>
#include <unordered_map>
#include <variant>
#include <array>
#include "util/SparseVector.h"
#include "util/Logger.h"
#include "world/chunk/ChunkMesh.h"
#include "world/chunk/ChunkPacking.h"
#include "world/chunk/ChunkBitmap.h"
#include "world/Block.h"

class IChunk {
public:
    IChunk() = default;

    void Initialize(ChunkPacking packingType);

    uint16_t GetBlock(const uint8_t x, const uint8_t y, const uint8_t z) const;

    uint16_t SetBlock(const uint16_t newBlock, const uint8_t x, const uint8_t y, const uint8_t z);

    ChunkMesh::Naive MeshNaive();

    ChunkMesh::HyperGreedy MeshHyperGreedy();

    void GreedyMeshBitmap(std::vector<uint32_t>& vertices, std::array<uint32_t, 1024>& bitmap, int normal) const;
protected:
    ChunkPacking m_packingMode;

    SparseVector<uint16_t, uint16_t> m_blockPalette; // List of block IDs.

    // Sacrifice a little bit of memory for faster deletes. Hash maps are way too slow.
    std::array<uint8_t, 64> m_blockPaletteIndices;
    std::array<uint16_t, 64> m_blockPaletteCounts;

    bool m_hasAir;

    virtual inline uint16_t RawGetBlock(const uint16_t index) const = 0;

    virtual inline void RawSetBlock(const uint16_t index, const uint16_t newBlock) = 0;

    virtual ChunkBitmap GetBlockBitmap(const BlockTypes block, const bool invert = false) const = 0;
private:
    static Logger sLogger;
};
