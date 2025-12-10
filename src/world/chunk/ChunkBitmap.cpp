#include "world/chunk/ChunkBitmap.h"

#include <bitset>

// ========== SIMD ==========

#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();

namespace HWY_NAMESPACE {
namespace hw = hwy::HWY_NAMESPACE;

void CullFrontBitsImpl(uint32_t* bitmap) {
    const hw::ScalableTag<uint32_t> u32Tag;
    const size_t numLanes = hw::Lanes(u32Tag);

    for (int i = 0; i < 1024; i += numLanes) {
        auto dataVec = hw::Load(u32Tag, bitmap + i);
        auto culledVec = hw::And(dataVec, hw::Not(hw::ShiftRight<1>(dataVec)));
        hw::Store(culledVec, u32Tag, bitmap + i);
    }
}

void CullBackBitsImpl(uint32_t* bitmap) {
    const hw::ScalableTag<uint32_t> u32Tag;
    const size_t numLanes = hw::Lanes(u32Tag);

    for (int i = 0; i < 1024; i += numLanes) {
        auto dataVec = hw::Load(u32Tag, bitmap + i);
        auto culledVec = hw::And(dataVec, hw::Not(hw::ShiftLeft<1>(dataVec)));
        hw::Store(culledVec, u32Tag, bitmap + i);
    }
}

void constexpr SwapBits32(uint32_t& a, uint32_t& b, uint32_t mask, uint32_t shift) {
    uint32_t t = ((b >> shift) ^ a) & mask;
    a ^= t;
    b ^= (t << shift);
}

void constexpr SwapBits64(uint64_t& a, uint64_t& b, uint64_t mask, uint64_t shift) {
    uint64_t t = ((b >> shift) ^ a) & mask;
    a ^= t;
    b ^= (t << shift);
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

    auto bitMask1 = hw::Set(u8Tag, 0x0F);
    auto bitMask2 = hw::Set(u64Tag, 0x0000'0000'3333'3333ULL);
    auto bitMask3 = hw::Set(u64Tag, 0x0000'0000'5555'5555ULL);

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
            auto secondVec1 = hw::BitCast(u16Tou8, hw::InterleaveOdd(u16Tag, firstVec5, firstVec1));
            auto secondVec2 = hw::BitCast(u16Tou8, hw::InterleaveOdd(u16Tag, firstVec6, firstVec2));
            auto secondVec3 = hw::BitCast(u16Tou8, hw::InterleaveOdd(u16Tag, firstVec7, firstVec3));
            auto secondVec4 = hw::BitCast(u16Tou8, hw::InterleaveOdd(u16Tag, firstVec8, firstVec4));
            auto secondVec5 = hw::BitCast(u16Tou8, hw::InterleaveEven(u16Tag, firstVec5, firstVec1));
            auto secondVec6 = hw::BitCast(u16Tou8, hw::InterleaveEven(u16Tag, firstVec6, firstVec2));
            auto secondVec7 = hw::BitCast(u16Tou8, hw::InterleaveEven(u16Tag, firstVec7, firstVec3));
            auto secondVec8 = hw::BitCast(u16Tou8, hw::InterleaveEven(u16Tag, firstVec8, firstVec4));

            // Stage 2.
            auto thirdVec1 = hw::InterleaveOdd(u8Tag, secondVec3, secondVec1);
            auto thirdVec2 = hw::InterleaveOdd(u8Tag, secondVec4, secondVec2);
            auto thirdVec3 = hw::InterleaveEven(u8Tag, secondVec3, secondVec1); 
            auto thirdVec4 = hw::InterleaveEven(u8Tag, secondVec4, secondVec2);
            auto thirdVec5 = hw::InterleaveOdd(u8Tag, secondVec7, secondVec5);
            auto thirdVec6 = hw::InterleaveOdd(u8Tag, secondVec8, secondVec6);
            auto thirdVec7 = hw::InterleaveEven(u8Tag, secondVec7, secondVec5);
            auto thirdVec8 = hw::InterleaveEven(u8Tag, secondVec8, secondVec6); 

            // Stage 3.
            auto firstSwap1 = hw::And(hw::Xor(thirdVec1, hw::ShiftRight<4>(thirdVec2)), bitMask1);
            auto fourthVec1 = hw::Xor(thirdVec1, firstSwap1);
            auto fourthVec2 = hw::Xor(thirdVec2, hw::ShiftLeft<4>(firstSwap1));
            auto firstSwap2 = hw::And(hw::Xor(thirdVec3, hw::ShiftRight<4>(thirdVec4)), bitMask1);
            auto fourthVec3 = hw::Xor(thirdVec3, firstSwap2);
            auto fourthVec4 = hw::Xor(thirdVec4, hw::ShiftLeft<4>(firstSwap2));
            auto firstSwap3 = hw::And(hw::Xor(thirdVec5, hw::ShiftRight<4>(thirdVec6)), bitMask1);
            auto fourthVec5 = hw::Xor(thirdVec5, firstSwap3);
            auto fourthVec6 = hw::Xor(thirdVec6, hw::ShiftLeft<4>(firstSwap3));
            auto firstSwap4 = hw::And(hw::Xor(thirdVec7, hw::ShiftRight<4>(thirdVec8)), bitMask1);
            auto fourthVec7 = hw::Xor(thirdVec7, firstSwap4);
            auto fourthVec8 = hw::Xor(thirdVec8, hw::ShiftLeft<4>(firstSwap4));

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
            for (int i = 0; i < 16; i += 2) {
                SwapBits64(slice64[i], slice64[i + 1], 0x3333333333333333ULL, 2); // 00110011
            }
        }

        // Pipeline Stage 5 for the chunk. SIMD more expensive.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint32_t* slice32 = bitmap + layer * 32;
            for (int i = 0; i < 32; i += 2) {
                SwapBits32(slice32[i], slice32[i+1], 0x55555555U, 1); // 0101
            }
        }
    }
}

// 0011 OBA
// 0011 OBA
// 1100 EAB
// 1100 EAB

// 0101 OAB
// 1010 EAB
// 0101
// 1010

constexpr void TransposeFour128(uint32_t* bitmap) {
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

constexpr void SwapBlock128(uint32_t* source, uint32_t* destination) { 
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

ChunkBitmap& ChunkBitmap::CullFrontBits() {
    HWY_STATIC_DISPATCH(CullFrontBitsImpl)(m_bitmap.data());
    return *this;
}

ChunkBitmap& ChunkBitmap::CullBackBits() {
    HWY_STATIC_DISPATCH(CullBackBitsImpl)(m_bitmap.data());
    return *this;
}

ChunkBitmap& ChunkBitmap::InnerTranspose() {
    HWY_STATIC_DISPATCH(InnerTranspose128Impl)(m_bitmap.data());
    return *this;
}

ChunkBitmap& ChunkBitmap::OuterTranspose() {
    HWY_STATIC_DISPATCH(OuterTranspose128Impl)(m_bitmap.data());
    return *this;
}

// ========== Scalar ==========

Logger ChunkBitmap::sLogger = Logger("ChunkBitmap");

void ChunkBitmap::OuterTransposeNaive(ChunkBitmap& destinationMap) const {
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            destinationMap[(y << 5) | x] = m_bitmap[(x << 5) | y];
        }
    }
}

void ChunkBitmap::OuterTransposeScalar() {
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = x + 1; y < 32; y++) {
            std::swap(m_bitmap[(y << 5) | x], m_bitmap[(x << 5) | y]);
        }
    }
}

void ChunkBitmap::InnerTransposeNaive(ChunkBitmap& destinationMap) const {
    destinationMap = {};
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            for (int z = 0; z < 32; z++) {
                uint8_t bit = (m_bitmap[x << 5 | y] >> (31 - z)) & 1u;
                destinationMap[x << 5 | z] ^= bit << (31 - y);
            }
        }
    }
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
}

ChunkBitmap& ChunkBitmap::SwapOuterInnerAxes() {
    InnerTranspose();
    OuterTranspose();
    return *this;
}

bool ChunkBitmap::TestInnerTransposes() const {
    ChunkBitmap naive;
    ChunkBitmap butterfly = Copy();

    InnerTransposeNaive(naive);
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

    InnerTransposeNaive(naive);
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
