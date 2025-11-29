#include "world/ChunkData.h"

Logger ChunkData::sLogger = Logger("ChunkData");

ChunkData::ChunkData(ChunkPacking packingType) {
    m_packingType = packingType;
    
    switch(packingType) {
        case ChunkPacking::Zero:
            throw sLogger.RuntimeError("Single block chunks not implemented!");
        case ChunkPacking::One:
            m_oneBitBlocks = std::vector<bool>(32768, 0);
            break;
        case ChunkPacking::Two:
        case ChunkPacking::Four:
            throw sLogger.RuntimeError("Only Four and Eight bit chunk packing implemented!");
        case ChunkPacking::Eight:
            m_eightBitBlocks = std::vector<uint8_t>(32768, 0);
            break;
        case ChunkPacking::Sixteen:
            m_sixteenBitBlocks = std::vector<uint16_t>(32768, 0);
            break;
    }
}

uint16_t ChunkData::Get(const uint16_t index) const {
    switch(m_packingType) {
        case ChunkPacking::Zero:
            throw sLogger.RuntimeError("Single block chunks not implemented!");
        case ChunkPacking::One:
            return m_oneBitBlocks[index];
        case ChunkPacking::Two:
            throw sLogger.RuntimeError("Two bit chunk packing not implemented!");
        case ChunkPacking::Four:
            throw sLogger.RuntimeError("Four bit chunk packing not implemented!");
        case ChunkPacking::Eight:
            return m_eightBitBlocks[index];
        case ChunkPacking::Sixteen:
            return m_sixteenBitBlocks[index];
    }
}

void ChunkData::Set(const uint16_t index, const uint16_t value) {
    switch(m_packingType) {
        case ChunkPacking::Zero:
            throw sLogger.RuntimeError("Single block chunks not implemented!");
        case ChunkPacking::One:
            m_oneBitBlocks[index] = value;
            break;
        case ChunkPacking::Two:
            throw sLogger.RuntimeError("Two bit chunk packing not implemented!");
        case ChunkPacking::Four:
            throw sLogger.RuntimeError("Four bit chunk packing not implemented!");
        case ChunkPacking::Eight:
            m_eightBitBlocks[index] = value;
            break;
        case ChunkPacking::Sixteen:
            m_sixteenBitBlocks[index] = value;
            break;
    }
}

uint16_t ChunkData::GetSet(const uint16_t index, const uint16_t value) {
    uint16_t oldValue;
    switch(m_packingType) {
        case ChunkPacking::Zero:
            throw sLogger.RuntimeError("Single block chunks not implemented!");
        case ChunkPacking::One:
            oldValue = m_oneBitBlocks[index];
            m_oneBitBlocks[index] = value;
            break;
        case ChunkPacking::Two:
            throw sLogger.RuntimeError("Two bit chunk packing not implemented!");
        case ChunkPacking::Four:
            throw sLogger.RuntimeError("Four bit chunk packing not implemented!");
        case ChunkPacking::Eight:
            oldValue = m_eightBitBlocks[index];
            m_eightBitBlocks[index] = value;
            break;
        case ChunkPacking::Sixteen:
            oldValue = m_sixteenBitBlocks[index];
            m_sixteenBitBlocks[index] = value;
            break;
    }
}
