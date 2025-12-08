#include "util/Bitmap.h"

#include <sstream>
#include <iomanip>
#include <bitset>
#include <chrono>
#include "util/Bitmap.h"

// ========== SIMD ==========

#include <hwy/highway.h>
#include "Bitmap.h"

HWY_BEFORE_NAMESPACE();

namespace HWY_NAMESPACE {
namespace hw = hwy::HWY_NAMESPACE;

const hw::FixedTag<uint8_t, 16> u8Tag;
const hw::FixedTag<uint16_t, 8> u16Tag;
const hw::FixedTag<uint32_t, 4> u32Tag;
const hw::FixedTag<uint64_t, 2> u64Tag;

const hw::Repartition<uint32_t, decltype(u8Tag)> u8Tou32;
const hw::Repartition<uint8_t, decltype(u16Tag)> u16Tou8;
const hw::Repartition<uint16_t, decltype(u32Tag)> u32Tou16;

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

void Inner3DTranspose128Impl(uint32_t* bitmap) {
    uint64_t* bitmap64 = reinterpret_cast<uint64_t*>(bitmap);

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

        // Pipeline Stage 4 for the chunk. SIMD too expensive.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = bitmap64 + layer * 16;
            for (int i = 0; i < 16; i += 2) {
                SwapBits64(slice64[i], slice64[i + 1], 0x3333333333333333ULL, 2); // 00110011
            }
        }

        // Pipeline Stage 5 for the chunk. SIMD too expensive.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint32_t* slice32 = bitmap + layer * 32;
            for (int i = 0; i < 32; i += 2) {
                SwapBits32(slice32[i], slice32[i+1], 0x55555555U, 1); // 0101
            }
        }
    }
}

}

HWY_AFTER_NAMESPACE();

// ========== SIMD Wrappers ==========

#if HWY_ONCE

void Bitmap::Inner3DTranspose(std::array<uint32_t, 1024>& bitmap) {
    HWY_STATIC_DISPATCH(Inner3DTranspose128Impl)(bitmap.data());
}

// ========== Scalar ==========

Logger Bitmap::sLogger = Logger("Bitmap");


void Bitmap::Outer3DTransposeNaive(const std::array<uint32_t, 1024>& sourceMap, std::array<uint32_t, 1024>& destinationMap) {
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            destinationMap[(y << 5) | x] = sourceMap[(x << 5) | y];
        }
    }
}

void Bitmap::Outer3DTranspose(std::array<uint32_t, 1024>& bitmap) {
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = x + 1; y < 32; y++) {
            std::swap(bitmap[(y << 5) | x], bitmap[(x << 5) | y]);
        }
    }
}

void Bitmap::Inner3DTransposeNaive(const std::array<uint32_t, 1024>& sourceMap, std::array<uint32_t, 1024>& destinationMap) {
    destinationMap = {};
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            for (int z = 0; z < 32; z++) {
                uint8_t bit = (sourceMap[x << 5 | y] >> (31 - z)) & 1u;
                destinationMap[x << 5 | z] ^= bit << (31 - y);
            }
        }
    }
}

void Bitmap::Inner3DTransposeScalar(std::array<uint32_t, 1024>& bitmap) {
    uint64_t* data64 = reinterpret_cast<uint64_t*>(bitmap.data());
    uint32_t* data32 = bitmap.data();
    
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
        // Log3DSlice(bitmap);
        // sLogger.Verbose("logged");
        
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
        
        // // Pipeline Stage 4 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = data64 + layer * 16;
            for (int i = 0; i < 8; i++) {
                SwapBits64(slice64[i*2], slice64[i*2+1], 0x3333333333333333ULL, 2); // 00110011
            }
        }

        // Pipeline Stage 5 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint32_t* slice32 = data32 + layer * 32;
            // #pragma unroll(8)  
            for (int i = 0; i < 32; i += 2) {
                SwapBits32(slice32[i], slice32[i+1], 0x55555555U, 1); // 0101
            }
        }
    }
}

void Bitmap::SwapOuterInnerAxes(std::array<uint32_t, 1024>& bitmap) {
    Inner3DTranspose(bitmap);
    Outer3DTranspose(bitmap);
}

bool Bitmap::TestInner3DTransposes(const std::array<uint32_t, 1024>& sourceMap) {
    std::array<uint32_t, 1024> naive;
    alignas(16) std::array<uint32_t, 1024> butterfly = sourceMap;

    Inner3DTransposeNaive(sourceMap, naive);
    Inner3DTranspose(butterfly);

    bool isEqual = true;

    if (!Is3DEqual(naive, butterfly)) {
        sLogger.Warning("Naive and butterfly simd inner transposes do not match!");
        isEqual = false;
    }

    // sLogger.Verbose("Source:");
    // Log3DSlice(sourceMap);

    // sLogger.Verbose("Naive:");
    // Log3DSlice(naive);

    // sLogger.Verbose("Butterfly:");
    // Log3DSlice(butterfly);

    return isEqual;
}

bool Bitmap::TestOuter3DTransposes(const std::array<uint32_t, 1024>& sourceMap) {
    std::array<uint32_t, 1024> naive;
    std::array<uint32_t, 1024> swapper = sourceMap;

    Outer3DTransposeNaive(sourceMap, naive);
    Outer3DTranspose(swapper);

    if (!Is3DEqual(naive, swapper)) {
        sLogger.Warning("Outer transposes do not match!");
        return false;
    }

    return true;
}

bool Bitmap::Is3DEqual(const std::array<uint32_t, 1024>& map1, const std::array<uint32_t, 1024>& map2) {
    for (int i = 0; i < 1024; i++) {
        if (map1[i] != map2[i])
            return false;
    }
    return true;
}

void Bitmap::Log3DSlice(const std::array<uint32_t, 1024>& sourceMap, uint8_t layer) {
    for (int i = 0; i < 32; i++) {
        sLogger.Verbose(std::bitset<32>(sourceMap[i + (32 * layer)]));
    }
}

#endif
