#pragma once

#include <array>
#include <cstdint>
#include <bit>
#include "util/Logger.h"

class Bitmap final  {
public:
    static void Outer3DTranspose(std::array<uint32_t, 1024>& bitmap);

    static void Outer3DTransposeNaive(const std::array<uint32_t, 1024>& sourceMap, std::array<uint32_t, 1024>& newMap);

    static void Inner3DTranspose(std::array<uint32_t, 1024>& bitmap);

    static void Inner3DTransposeNaive(const std::array<uint32_t, 1024>& sourceMap, std::array<uint32_t, 1024>& newMap);

    static void Inner3DTransposeScalar(std::array<uint32_t, 1024>& bitmap);

    static bool Is3DEqual(const std::array<uint32_t, 1024>& map1, const std::array<uint32_t, 1024>& map2);

    static bool TestInner3DTransposes(const std::array<uint32_t, 1024>& sourceMap);

    static bool TestOuter3DTransposes(const std::array<uint32_t, 1024>& sourceMap);

    static void Log3DSlice(const std::array<uint32_t, 1024>& sourceMap, uint8_t layer = 0);

    static void constexpr SwapBits32(uint32_t& a, uint32_t& b, uint32_t mask, uint32_t shift) {
        uint32_t t = (std::rotr(a, shift) ^ b) & mask;
        b ^= t;
        a ^= std::rotl(t, shift);
    }

    static void constexpr SwapBits64(uint64_t& a, uint64_t& b, uint64_t mask, uint64_t shift) {
        uint64_t t = ((a >> shift) ^ b) & mask;
        b ^= t;
        a ^= (t << shift);
    }
    
    // Shorthand for an inner followed by outer transposition.
    static void SwapOuterInnerAxes(std::array<uint32_t, 1024>& bitmap);
private:
    static Logger sLogger;
};
