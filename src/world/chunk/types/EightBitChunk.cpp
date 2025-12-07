#include <iomanip>
#include <bit>
#include <cstring>
#include "world/chunk/types/EightBitChunk.h"

// ========== SIMD ==========

#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();

namespace HWY_NAMESPACE {
namespace hw = hwy::HWY_NAMESPACE;

void GetSolidBitmapImpl(const uint8_t* blockData, const uint8_t air, uint8_t* bitmap) {
    const hw::CappedTag<uint8_t, 64> u8Tag;
    const size_t numLanes = hw::Lanes(u8Tag);

    auto airVec = hw::Set(u8Tag, air);
    for (int i = 0; i < 32768; i += numLanes) {
        auto blockVec = hw::Load(u8Tag, blockData + i);
        auto resultMask = hw::Eq(blockVec, airVec);
        hw::StoreMaskBits(u8Tag, resultMask, bitmap + (i / 8));
    }
}

}

HWY_AFTER_NAMESPACE();

// ========== SIMD Wrappers ==========

#if HWY_ONCE

void EightBitChunk::GetSolidBitmap(std::array<uint32_t, 1024>& bitmap) const {
    HWY_STATIC_DISPATCH(GetSolidBitmapImpl)(m_blockData.data(), m_blockPaletteIndices[BlockTypes::Air], reinterpret_cast<uint8_t*>(bitmap.data()));
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
