#pragma once

#include "world/chunk/IChunk.h"
#include "world/chunk/ChunkBitmap.h"

class EightBitChunk final : public IChunk {
public:
    EightBitChunk(); // Better, allocate memory before assignment.

    EightBitChunk(std::array<uint8_t, 32768>& blockData); // Has to copy, less efficient than building here directly.

    // Efficient load data functions TODO.
// protected:
    uint16_t RawGetBlock(const uint16_t index) const override;

    void RawSetBlock(const uint16_t index, const uint16_t newBlock) override;

    ChunkBitmap GetBlockBitmap(const BlockTypes block, const bool invert = false) const override;
private:
    static Logger sLogger;

    alignas(16) std::array<uint8_t, 32768> m_blockData{};
};
