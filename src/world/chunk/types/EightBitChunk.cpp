#include "world/chunk/types/EightBitChunk.h"

#include <cstring>

// ========== SIMD ==========

#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();

namespace HWY_NAMESPACE {
namespace hw = hwy::HWY_NAMESPACE;

const hw::CappedTag<uint8_t, 64> u8Tag;
const size_t numLanes = hw::Lanes(u8Tag);

void GetSolidBitmapImpl(const uint8_t* blockData, const BlockTypes block, ChunkBitmap& bitmap, const bool invert) {
    uint8_t* bitmapPtr = reinterpret_cast<uint8_t*>(bitmap.Data());

    auto typeVec = hw::Set(u8Tag, block);
    
    if (invert) {
        for (int i = 0; i < 32768; i += numLanes) {
            auto dataVec = hw::Load(u8Tag, blockData + i);
            auto resultMask = hw::Ne(dataVec, typeVec);
            hw::StoreMaskBits(u8Tag, resultMask, bitmapPtr + (i / 8));
        }
    } else {
        for (int i = 0; i < 32768; i += numLanes) {
            auto dataVec = hw::Load(u8Tag, blockData + i);
            auto resultMask = hw::Eq(dataVec, typeVec);
            hw::StoreMaskBits(u8Tag, resultMask, bitmapPtr + (i / 8));
        }
    }
}

}

HWY_AFTER_NAMESPACE();

// ========== SIMD Wrappers ==========

#if HWY_ONCE

ChunkBitmap EightBitChunk::GetBlockBitmap(const BlockTypes block, const bool invert) const {
    ChunkBitmap bitmap;
    HWY_STATIC_DISPATCH(GetSolidBitmapImpl)(m_blockData.data(), block, bitmap, invert);
    return bitmap;
}

// ========== Scalar ==========

Logger EightBitChunk::sLogger = Logger("EightBitChunk");

EightBitChunk::EightBitChunk() {
    Initialize(ChunkPacking::Eight);
}

EightBitChunk::EightBitChunk(std::array<uint8_t, 32768>& blockData) {
    std::memcpy(m_blockData.data(), blockData.data(), sizeof(m_blockData));
}

uint16_t EightBitChunk::RawGetBlock(const uint16_t index) const {
    return m_blockData[index];
}

void EightBitChunk::RawSetBlock(const uint16_t index, const uint16_t newBlock) {
    m_blockData[index] = newBlock;
}

#endif
