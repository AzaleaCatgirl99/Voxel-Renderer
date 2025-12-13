#include "world/chunk/ChunkBitmap.h"

#include <bitset>

// ========== SIMD ==========

#include <hwy/highway.h>
#include "ChunkBitmap.h"

HWY_BEFORE_NAMESPACE();

namespace HWY_NAMESPACE {
namespace hw = hwy::HWY_NAMESPACE;

void AndImpl(uint32_t* bitmap, const uint32_t* otherBitmap) {
    const hw::ScalableTag<uint32_t> u32Tag;
    const uint32_t numLanes = hw::Lanes(u32Tag);

    for (uint32_t i = 0; i < 1024; i += numLanes) {
        auto vec1 = hw::Load(u32Tag, bitmap + i);
        auto vec2 = hw::Load(u32Tag, otherBitmap + i);
        auto resultVec = hw::And(vec1, vec2);
        hw::Store(resultVec, u32Tag, bitmap + i);
    }
}

std::array<uint32_t, 32> GetActiveRows(uint32_t* bitmap) {
    std::array<uint32_t, 32> activeRows;
    const hw::CappedTag<uint32_t, 32> u32Tag;
    const uint32_t numLanes = hw::Lanes(u32Tag);

    const auto zero = hw::Zero(u32Tag);
    for (uint32_t i = 0; i < 32; i++) {
        uint32_t slice = 0;
        for (uint32_t j = 0; j < 32; j += numLanes) {
            auto data = hw::Load(u32Tag, bitmap + (i * 32) + j);
            auto results = hw::Ne(data, zero);
            uint32_t maskBits;
            hw::StoreMaskBits(u32Tag, results, reinterpret_cast<uint8_t*>(&maskBits));
            slice |= maskBits << j;
        }
        activeRows[i] = slice;
    }

    return activeRows;
}

uint32_t GetActiveSlices(std::array<uint32_t, 32>& activeRows) {
    const hw::CappedTag<uint32_t, 32> u32Tag;
    const uint32_t numLanes = hw::Lanes(u32Tag);

    uint32_t activeSlices = 0;
    const auto zero = hw::Zero(u32Tag);
    for (uint32_t i = 0; i < 32; i += numLanes) {
        auto data = hw::Load(u32Tag, activeRows.data() + i);
        auto results = hw::Ne(data, zero);
        uint32_t maskBits;
        hw::StoreMaskBits(u32Tag, results, reinterpret_cast<uint8_t*>(&maskBits));
        activeSlices |= maskBits << i;
    }

    return activeSlices;
};

// Potential future optimization: parallelize width expansion.
template<AxisOrder order>
void GreedyMeshBitmapImpl(uint32_t* bitmap, std::vector<uint32_t>& vertices) {
    const hw::FixedTag<uint32_t, 4> u32Tag;
    const uint32_t numLanes = hw::Lanes(u32Tag);

    alignas(16) std::array<uint32_t, 32> activeRows = GetActiveRows(bitmap);
    uint32_t activeSlices = GetActiveSlices(activeRows);

    // alignas(16) std::array<uint32_t, 1025> endPoints = GetGreedyEnds(bitmap, activeSlices);

    uint32_t code = 0;
    while (activeSlices != 0) {
        const uint32_t slice = std::countr_zero(activeSlices);
        // const uint32_t slicePacked = slice << sliceShift;
        while (activeRows[slice] != 0) {
            const uint32_t row = std::countr_zero(activeRows[slice]);
            // const uint32_t rowPacked = row << rowShift;
            const uint32_t index = slice * 32 + row;
            uint32_t bits = bitmap[index];

            while (bits != 0) {

                const uint32_t bottom = std::countr_zero(bits);
                const uint32_t height = std::countr_one(bits >> bottom);
                uint32_t width = 1;

                const uint32_t mask = (uint32_t)((1ULL << height) - 1) << bottom;
                bits ^= mask;

                for (int i = 1; i < 32 - row; i++) {
                    if ((bitmap[index + i] & mask) != mask)
                        break;
                    width++;
                    bitmap[index + i] ^= mask;
                }

                if constexpr (order == AxisOrder::eXYZ)
                    vertices.push_back(((height - 1) << 20) | ((width - 1) << 15) | (slice << 10) | (row << 5) | (bottom << 0));
                else if constexpr (order == AxisOrder::eXZY)
                    vertices.push_back(((height - 1) << 20) | ((width - 1) << 15) | (slice << 10) | (row << 0) | (bottom << 5));
                else if constexpr (order == AxisOrder::eYXZ)
                    vertices.push_back(((height - 1) << 20) | ((width - 1) << 15) | (slice << 5) | (row << 10) | (bottom << 0));
                else if constexpr (order == AxisOrder::eYZX)
                    vertices.push_back(((height - 1) << 20) | ((width - 1) << 15) | (slice << 5) | (row << 0) | (bottom << 10));
                else if constexpr (order == AxisOrder::eZXY)
                    vertices.push_back(((height - 1) << 20) | ((width - 1) << 15) | (slice << 0) | (row << 10) | (bottom << 5));
                else if constexpr (order == AxisOrder::eZYX)
                    vertices.push_back(((height - 1) << 20) | ((width - 1) << 15) | (slice << 0) | (row << 5) | (bottom << 10));
            }
            activeRows[slice] ^= 1U << row;
        }
        activeSlices ^= 1U << slice;
    }
}

void CullLeastSigBitsImpl(uint32_t* bitmap) {
    const hw::ScalableTag<uint32_t> u32Tag;
    const size_t numLanes = hw::Lanes(u32Tag);

    for (int i = 0; i < 1024; i += numLanes) {
        auto dataVec = hw::Load(u32Tag, bitmap + i);
        auto culledVec = hw::And(dataVec, hw::Not(hw::ShiftRight<1>(dataVec)));
        hw::Store(culledVec, u32Tag, bitmap + i);
    }
}

void CullMostSigBitsImpl(uint32_t* bitmap) {
    const hw::ScalableTag<uint32_t> u32Tag;
    const size_t numLanes = hw::Lanes(u32Tag);

    for (int i = 0; i < 1024; i += numLanes) {
        auto dataVec = hw::Load(u32Tag, bitmap + i);
        auto culledVec = hw::And(dataVec, hw::Not(hw::ShiftLeft<1>(dataVec)));
        hw::Store(culledVec, u32Tag, bitmap + i);
    }
}

constexpr inline void SwapBits32(uint32_t& a, uint32_t& b, uint32_t mask, uint32_t shift) {
    uint32_t t = ((a >> shift) ^ b) & mask;
    b ^= t;
    a ^= (t << shift);
}

constexpr inline void SwapBits64(uint64_t& a, uint64_t& b, uint64_t mask, uint64_t shift) {
    uint64_t t = ((a >> shift) ^ b) & mask;
    b ^= t;
    a ^= (t << shift);
}

void InnerTranspose128Impl(uint32_t* bitmap) {
    uint64_t* bitmap64 = reinterpret_cast<uint64_t*>(bitmap);

    const hw::FixedTag<uint8_t, 16> u8Tag;
    const hw::FixedTag<uint16_t, 8> u16Tag;
    const hw::FixedTag<uint32_t, 4> u32Tag;
    const hw::FixedTag<uint64_t, 2> u64Tag;

    const hw::Repartition<uint32_t, decltype(u8Tag)> u8Tou32;
    const hw::Repartition<uint8_t, decltype(u16Tag)> u16Tou8;
    const hw::Repartition<uint16_t, decltype(u32Tag)> u32Tou16;
    const hw::Repartition<uint64_t, decltype(u32Tag)> u32Tou64;
    const hw::Repartition<uint32_t, decltype(u64Tag)> u64Tou32;

    auto bitMask1 = hw::Set(u8Tag, 0xF0);
    auto bitMask2 = hw::Set(u64Tag, 0x0000'0000'CCCC'CCCCULL); // 1100 12 -> C
    auto bitMask3 = hw::Set(u64Tag, 0x0000'0000'AAAA'AAAAULL); // 1010 10 -> A

    constexpr int chunkSize = 4; // 4 was profiled to have the best performance.
    
    for (int chunkStart = 0; chunkStart < 32; chunkStart += chunkSize) {
        int chunkEnd = std::min(chunkStart + chunkSize, 32);
        
        // Pipeline stages 1-3 for the chunk with SIMD.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint32_t* layerPtr = bitmap + layer * 32;

            // Load the slice.
            auto firstVec1 = hw::BitCast(u32Tou16, hw::Load(u32Tag, layerPtr)); // 1-8.
            auto firstVec2 = hw::BitCast(u32Tou16, hw::Load(u32Tag, layerPtr + 4)); // 1-8.
            auto firstVec3 = hw::BitCast(u32Tou16, hw::Load(u32Tag, layerPtr + 8)); // 9-16.
            auto firstVec4 = hw::BitCast(u32Tou16, hw::Load(u32Tag, layerPtr + 12)); // 9-16.
            auto firstVec5 = hw::BitCast(u32Tou16, hw::Load(u32Tag, layerPtr + 16)); // 17-24.
            auto firstVec6 = hw::BitCast(u32Tou16, hw::Load(u32Tag, layerPtr + 20)); // 17-24.
            auto firstVec7 = hw::BitCast(u32Tou16, hw::Load(u32Tag, layerPtr + 24)); // 25-32.
            auto firstVec8 = hw::BitCast(u32Tou16, hw::Load(u32Tag, layerPtr + 28)); // 25-32.

            // Stage 1.   
            auto secondVec1 = hw::BitCast(u16Tou8, hw::InterleaveEven(u16Tag, firstVec1, firstVec5));
            auto secondVec2 = hw::BitCast(u16Tou8, hw::InterleaveEven(u16Tag, firstVec2, firstVec6));
            auto secondVec3 = hw::BitCast(u16Tou8, hw::InterleaveEven(u16Tag, firstVec3, firstVec7));
            auto secondVec4 = hw::BitCast(u16Tou8, hw::InterleaveEven(u16Tag, firstVec4, firstVec8));
            auto secondVec5 = hw::BitCast(u16Tou8, hw::InterleaveOdd(u16Tag, firstVec1, firstVec5));
            auto secondVec6 = hw::BitCast(u16Tou8, hw::InterleaveOdd(u16Tag, firstVec2, firstVec6));
            auto secondVec7 = hw::BitCast(u16Tou8, hw::InterleaveOdd(u16Tag, firstVec3, firstVec7));
            auto secondVec8 = hw::BitCast(u16Tou8, hw::InterleaveOdd(u16Tag, firstVec4, firstVec8));

            // Stage 2.
            auto thirdVec1 = hw::InterleaveEven(u8Tag, secondVec1, secondVec3);
            auto thirdVec2 = hw::InterleaveEven(u8Tag, secondVec2, secondVec4);
            auto thirdVec3 = hw::InterleaveOdd(u8Tag, secondVec1, secondVec3); 
            auto thirdVec4 = hw::InterleaveOdd(u8Tag, secondVec2, secondVec4);
            auto thirdVec5 = hw::InterleaveEven(u8Tag, secondVec5, secondVec7);
            auto thirdVec6 = hw::InterleaveEven(u8Tag, secondVec6, secondVec8);
            auto thirdVec7 = hw::InterleaveOdd(u8Tag, secondVec5, secondVec7);
            auto thirdVec8 = hw::InterleaveOdd(u8Tag, secondVec6, secondVec8); 

            // Stage 3.
            auto firstSwap1 = hw::And(hw::Xor(thirdVec1, hw::ShiftLeft<4>(thirdVec2)), bitMask1);
            auto fourthVec1 = hw::Xor(thirdVec1, firstSwap1);
            auto fourthVec2 = hw::Xor(thirdVec2, hw::ShiftRight<4>(firstSwap1));
            auto firstSwap2 = hw::And(hw::Xor(thirdVec3, hw::ShiftLeft<4>(thirdVec4)), bitMask1);
            auto fourthVec3 = hw::Xor(thirdVec3, firstSwap2);
            auto fourthVec4 = hw::Xor(thirdVec4, hw::ShiftRight<4>(firstSwap2));
            auto firstSwap3 = hw::And(hw::Xor(thirdVec5, hw::ShiftLeft<4>(thirdVec6)), bitMask1);
            auto fourthVec5 = hw::Xor(thirdVec5, firstSwap3);
            auto fourthVec6 = hw::Xor(thirdVec6, hw::ShiftRight<4>(firstSwap3));
            auto firstSwap4 = hw::And(hw::Xor(thirdVec7, hw::ShiftLeft<4>(thirdVec8)), bitMask1);
            auto fourthVec7 = hw::Xor(thirdVec7, firstSwap4);
            auto fourthVec8 = hw::Xor(thirdVec8, hw::ShiftRight<4>(firstSwap4));

            // Store the results.
            hw::Store(hw::BitCast(u8Tou32, fourthVec1), u32Tag, layerPtr);
            hw::Store(hw::BitCast(u8Tou32, fourthVec2), u32Tag, layerPtr + 4);
            hw::Store(hw::BitCast(u8Tou32, fourthVec3), u32Tag, layerPtr + 8);
            hw::Store(hw::BitCast(u8Tou32, fourthVec4), u32Tag, layerPtr + 12);
            hw::Store(hw::BitCast(u8Tou32, fourthVec5), u32Tag, layerPtr + 16);
            hw::Store(hw::BitCast(u8Tou32, fourthVec6), u32Tag, layerPtr + 20);
            hw::Store(hw::BitCast(u8Tou32, fourthVec7), u32Tag, layerPtr + 24);
            hw::Store(hw::BitCast(u8Tou32, fourthVec8), u32Tag, layerPtr + 28);
        }

        // Pipeline Stage 4 for the chunk. SIMD more expensive.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = bitmap64 + layer * 16;
            for (int i = 0; i < 16; i += 2)
                SwapBits64(slice64[i], slice64[i + 1], 0x3333333333333333ULL, 2); // 00110011
        }

        // Pipeline Stage 5 for the chunk. SIMD more expensive.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint32_t* slice32 = bitmap + layer * 32;
            for (int i = 0; i < 32; i += 2)
                SwapBits32(slice32[i], slice32[i+1], 0x55555555U, 1); // 0101
        }
    }
}

void TransposeFour128(uint32_t* bitmap) {
    const hw::FixedTag<uint32_t, 4> u32Tag;
    const hw::FixedTag<uint64_t, 2> u64Tag;

    const hw::Repartition<uint64_t, decltype(u32Tag)> u32Tou64;
    const hw::Repartition<uint32_t, decltype(u64Tag)> u64Tou32;

    auto firstVec1 = hw::BitCast(u32Tou64, hw::Load(u32Tag, bitmap));
    auto firstVec2 = hw::BitCast(u32Tou64, hw::Load(u32Tag, bitmap + 32));
    auto firstVec3 = hw::BitCast(u32Tou64, hw::Load(u32Tag, bitmap + 64));
    auto firstVec4 = hw::BitCast(u32Tou64, hw::Load(u32Tag, bitmap + 96));

    auto secondVec1 = hw::BitCast(u64Tou32, hw::InterleaveEven(u64Tag, firstVec1, firstVec3));
    auto secondVec2 = hw::BitCast(u64Tou32, hw::InterleaveEven(u64Tag, firstVec2, firstVec4));
    auto secondVec3 = hw::BitCast(u64Tou32, hw::InterleaveOdd(u64Tag, firstVec1, firstVec3));
    auto secondVec4 = hw::BitCast(u64Tou32, hw::InterleaveOdd(u64Tag, firstVec2, firstVec4));

    auto thirdVec1 = hw::InterleaveEven(u32Tag, secondVec1, secondVec2);
    auto thirdVec2 = hw::InterleaveOdd(u32Tag, secondVec1, secondVec2);
    auto thirdVec3 = hw::InterleaveEven(u32Tag, secondVec3, secondVec4);
    auto thirdVec4 = hw::InterleaveOdd(u32Tag, secondVec3, secondVec4);

    hw::Store(thirdVec1, u32Tag, bitmap);
    hw::Store(thirdVec2, u32Tag, bitmap + 32);
    hw::Store(thirdVec3, u32Tag, bitmap + 64);
    hw::Store(thirdVec4, u32Tag, bitmap + 96);
}

void SwapBlock128(uint32_t* source, uint32_t* destination) { 
    const hw::FixedTag<uint32_t, 4> u32Tag;

    auto srcVec1 = hw::Load(u32Tag, source);
    auto srcVec2 = hw::Load(u32Tag, source + 32);
    auto srcVec3 = hw::Load(u32Tag, source + 64);
    auto srcVec4 = hw::Load(u32Tag, source + 96);

    auto destVec1 = hw::Load(u32Tag, destination);
    auto destVec2 = hw::Load(u32Tag, destination + 32);
    auto destVec3 = hw::Load(u32Tag, destination + 64);
    auto destVec4 = hw::Load(u32Tag, destination + 96);

    hw::Store(srcVec1, u32Tag, destination);
    hw::Store(srcVec2, u32Tag, destination + 32);
    hw::Store(srcVec3, u32Tag, destination + 64);
    hw::Store(srcVec4, u32Tag, destination + 96);

    hw::Store(destVec1, u32Tag, source);
    hw::Store(destVec2, u32Tag, source + 32);
    hw::Store(destVec3, u32Tag, source + 64);
    hw::Store(destVec4, u32Tag, source + 96);
}

void OuterTranspose128Impl(uint32_t* bitmap) {
    // 4x4 block swaps.
    for (int y = 0; y < 8; y++) {
        for (int x = y + 1; x < 8; x++) {
            uint32_t* topLeft = bitmap + (y * 128) + (x * 4);
            uint32_t* transposedTopLeft = bitmap + (y * 4) + (x * 128); 
            SwapBlock128(topLeft, transposedTopLeft);
        }
    }

    // Individual 4x4 block transposes.
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            TransposeFour128(bitmap + (y * 128) + (x * 4));
        }
    }
}

}

HWY_AFTER_NAMESPACE();

// ========== SIMD Wrappers ==========

#if HWY_ONCE

ChunkBitmap& ChunkBitmap::And(const ChunkBitmap& otherBitmap) {
    HWY_STATIC_DISPATCH(AndImpl)(m_bitmap.data(), otherBitmap.m_bitmap.data());
    return *this;
}

void ChunkBitmap::GreedyMeshBitmap(std::vector<uint32_t>& vertices) {
    switch (m_axisOrder) {
        case AxisOrder::eXYZ: return HWY_STATIC_DISPATCH(GreedyMeshBitmapImpl<AxisOrder::eXYZ>)(m_bitmap.data(), vertices);
        case AxisOrder::eXZY: return HWY_STATIC_DISPATCH(GreedyMeshBitmapImpl<AxisOrder::eXZY>)(m_bitmap.data(), vertices);
        case AxisOrder::eYXZ: return HWY_STATIC_DISPATCH(GreedyMeshBitmapImpl<AxisOrder::eYXZ>)(m_bitmap.data(), vertices);
        case AxisOrder::eYZX: return HWY_STATIC_DISPATCH(GreedyMeshBitmapImpl<AxisOrder::eYZX>)(m_bitmap.data(), vertices);
        case AxisOrder::eZXY: return HWY_STATIC_DISPATCH(GreedyMeshBitmapImpl<AxisOrder::eZXY>)(m_bitmap.data(), vertices);
        case AxisOrder::eZYX: return HWY_STATIC_DISPATCH(GreedyMeshBitmapImpl<AxisOrder::eZYX>)(m_bitmap.data(), vertices);
    }
};

ChunkBitmap& ChunkBitmap::CullMostSigBits() {
    HWY_STATIC_DISPATCH(CullMostSigBitsImpl)(m_bitmap.data());
    return *this;
}

ChunkBitmap& ChunkBitmap::CullLeastSigBits() {
    HWY_STATIC_DISPATCH(CullLeastSigBitsImpl)(m_bitmap.data());
    return *this;
}

ChunkBitmap& ChunkBitmap::InnerTranspose() {
    HWY_STATIC_DISPATCH(InnerTranspose128Impl)(m_bitmap.data());
    UpdateAxisAfterInner();
    return *this;
}

ChunkBitmap& ChunkBitmap::OuterTranspose() {
    HWY_STATIC_DISPATCH(OuterTranspose128Impl)(m_bitmap.data());
    UpdateAxisAfterOuter();
    return *this;
}

// ========== Scalar ==========

std::array<AxisOrder, 6> ChunkBitmap::sAxisOrderAfterOuter = {
    AxisOrder::eYXZ, // From eXYZ 
    AxisOrder::eZXY, // From eXZY 
    AxisOrder::eXYZ, // From eYXZ 
    AxisOrder::eZYX, // From eYZX 
    AxisOrder::eXZY, // From eZXY 
    AxisOrder::eYZX  // From eZYX
};

std::array<AxisOrder, 6> ChunkBitmap::sAxisOrderAfterInner = {
    AxisOrder::eXZY, // From eXYZ 
    AxisOrder::eXYZ, // From eXZY 
    AxisOrder::eYZX, // From eYXZ 
    AxisOrder::eYXZ, // From eYZX 
    AxisOrder::eZYX, // From eZXY 
    AxisOrder::eZXY  // From eZYX
};

Logger ChunkBitmap::sLogger = Logger("ChunkBitmap");

void ChunkBitmap::OuterTransposeNaive(ChunkBitmap& destinationMap) {
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            destinationMap[(y << 5) | x] = m_bitmap[(x << 5) | y];
        }
    }
    UpdateAxisAfterOuter();
}

void ChunkBitmap::OuterTransposeScalar() {
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = x + 1; y < 32; y++) {
            std::swap(m_bitmap[(y << 5) | x], m_bitmap[(x << 5) | y]);
        }
    }
    UpdateAxisAfterOuter();
}

void ChunkBitmap::InnerTransposeNaive(ChunkBitmap& destinationMap) {
    destinationMap = {};
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            for (int z = 0; z < 32; z++) {
                uint8_t bit = (m_bitmap[x << 5 | y] >> z) & 1u;
                destinationMap[x << 5 | z] ^= bit << y;
            }
        }
    }
    UpdateAxisAfterInner();
}

void ChunkBitmap::InnerTransposeScalar() {
    uint64_t* data64 = reinterpret_cast<uint64_t*>(m_bitmap.data());
    uint32_t* data32 = m_bitmap.data();
    
    // Process in specific chunk sizes to optimize locality.
    constexpr int chunkSize = 4; // 4 is best for me, very slightly better than not chunking.
    
    for (int chunkStart = 0; chunkStart < 32; chunkStart += chunkSize) {
        int chunkEnd = std::min(chunkStart + chunkSize, 32);
        
        // Pipeline Stage 1 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = data64 + layer * 16;
            for (int i = 0; i < 8; i++) {
                SwapBits64(slice64[i], slice64[i+8], 0x0000FFFF0000FFFF, 16);
            }
        }
        
        // Pipeline Stage 2 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = data64 + layer * 16;
            for (int i = 0; i < 4; i++) {
                SwapBits64(slice64[i], slice64[i+4], 0x00FF00FF00FF00FF, 8);
                SwapBits64(slice64[i+8], slice64[i+12], 0x00FF00FF00FF00FF, 8);
            }
        }
        
        // Pipeline Stage 3 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = data64 + layer * 16;
            for (int i = 0; i < 2; i++) {
                SwapBits64(slice64[i], slice64[i+2], 0x0F0F0F0F0F0F0F0F, 4);
                SwapBits64(slice64[i+4], slice64[i+6], 0x0F0F0F0F0F0F0F0F, 4);
                SwapBits64(slice64[i+8], slice64[i+10], 0x0F0F0F0F0F0F0F0F, 4);
                SwapBits64(slice64[i+12], slice64[i+14], 0x0F0F0F0F0F0F0F0F, 4);
            }
        }
        
        // Pipeline Stage 4 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = data64 + layer * 16;
            for (int i = 0; i < 8; i++) {
                SwapBits64(slice64[i*2], slice64[i*2+1], 0x3333333333333333ULL, 2); // 00110011
            }
        }

        // Pipeline Stage 5 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint32_t* slice32 = data32 + layer * 32;
            for (int i = 0; i < 32; i += 2) {
                SwapBits32(slice32[i], slice32[i+1], 0x55555555U, 1); // 0101
            }
        }
    }
    UpdateAxisAfterInner();
}

bool ChunkBitmap::TestInnerTransposes() const {
    ChunkBitmap naive;
    ChunkBitmap butterfly = Copy();

    (*this).Copy().InnerTransposeNaive(naive);
    butterfly.InnerTranspose();

    if (naive != butterfly) {
        sLogger.Verbose("Source:");
        LogInnerSlice();

        sLogger.Verbose("Naive:");
        naive.LogInnerSlice();

        sLogger.Verbose("Butterfly:");
        butterfly.LogInnerSlice();

        sLogger.Warning("Naive and butterfly simd inner transposes do not match!");

        return false;
    }

    sLogger.Verbose("Verified the accuracy of inner transpositions!");

    return true;
}

bool ChunkBitmap::TestOuterTransposes() const {
    ChunkBitmap naive;
    ChunkBitmap simd = Copy();

    (*this).Copy().InnerTransposeNaive(naive);
    simd.InnerTranspose();

    if (naive != simd) {
        sLogger.Verbose("Source:");
        LogOuterSlice();

        sLogger.Verbose("Naive:");
        naive.LogOuterSlice();

        sLogger.Verbose("Simd:");
        simd.LogOuterSlice();

        sLogger.Warning("Outer transposes do not match!");
        return false;
    }

    sLogger.Warning("Verified the accuracy of outer transpositions!");

    return true;
}

bool ChunkBitmap::operator==(const ChunkBitmap& otherBitmap) const {
    for (int i = 0; i < 1024; i++) {
        if (m_bitmap[i] != otherBitmap[i])
            return false;
    }
    return true;
}

void ChunkBitmap::LogInnerSlice(uint8_t layer) const {
    for (int i = 0; i < 32; i++) {
        sLogger.Verbose(std::bitset<32>(m_bitmap[i + (32 * layer)]));
    }
}

void ChunkBitmap::LogOuterSlice(uint8_t layer) const {
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            std::cout << ((m_bitmap[i * 32 + j] >> (31 - layer)) & 1u);
        }
        std::cout << std::endl;
    }
}

#endif
