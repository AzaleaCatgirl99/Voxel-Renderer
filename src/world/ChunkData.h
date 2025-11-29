#pragma once

#include <vector>
#include <cstdint>
#include <world/ChunkPacking.h>
#include <util/Logger.h>

class ChunkData {
public:
    ChunkData() = default;
    ChunkData(ChunkPacking packingType);

    uint16_t Get (const uint16_t index) const;

    void Set(const uint16_t index, const uint16_t value);

    uint16_t GetSet(const uint16_t index, const uint16_t value);
private:
    static Logger sLogger;

    ChunkPacking m_packingType;
    std::vector<bool> m_oneBitBlocks;
    std::vector<uint32_t> m_twoBitBlocks;
    std::vector<uint32_t> m_fourBitBlocks;
    std::vector<uint8_t> m_eightBitBlocks;
    std::vector<uint16_t> m_sixteenBitBlocks;
};
