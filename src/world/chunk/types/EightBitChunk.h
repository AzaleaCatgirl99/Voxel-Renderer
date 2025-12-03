#pragma once

#include <world/chunk/IChunk.h>

class EightBitChunk final : public IChunk {
public:
    EightBitChunk(); // Better, allocate memory before assignment.

    EightBitChunk(std::array<uint8_t, 32768>& blockData); // Has to copy, less efficient than building here directly.

    // Efficient load data functions.
// protected:
    inline uint16_t RawGetBlock(const uint16_t index) const override;

    inline void RawSetBlock(const uint16_t index, const uint16_t newBlock) override;

    void GetAirBitmap(std::array<uint32_t, 1024>& bitmap) const override;
private:
    static Logger sLogger;

    std::array<uint8_t, 32768> m_blockData{};
};
