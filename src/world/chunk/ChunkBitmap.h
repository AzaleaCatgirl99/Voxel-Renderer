#pragma once

#include <array>
#include <cstdint>
#include <bit>
#include "util/Logger.h"

class ChunkBitmap final {
public:
    ChunkBitmap() = default;

    ChunkBitmap(const ChunkBitmap& otherBitmap) : m_bitmap(otherBitmap.m_bitmap) {};

    ChunkBitmap(const std::array<uint32_t, 1024>& otherBitmap) : m_bitmap(otherBitmap) {};

    ChunkBitmap& CullBackBits();

    ChunkBitmap& CullFrontBits();

    ChunkBitmap& OuterTranspose();

    void OuterTransposeNaive(ChunkBitmap& newMap) const;

    void OuterTransposeScalar();

    ChunkBitmap& InnerTranspose();

    void InnerTransposeNaive(ChunkBitmap& newMap) const;

    void InnerTransposeScalar();

    bool TestOuterTransposes() const;

    bool TestInnerTransposes() const;

    void LogInnerSlice(uint8_t layer = 0) const;

    void LogOuterSlice(uint8_t layer = 0) const;
    
    // Shorthand for an inner followed by outer transposition.
    ChunkBitmap& SwapOuterInnerAxes();

    constexpr uint32_t* Data() {
        return m_bitmap.data();
    }

    constexpr ChunkBitmap Copy() const {
        return ChunkBitmap(*this);
    }

    constexpr auto& operator[](size_t index) {
        return m_bitmap[index];
    }

    constexpr auto& operator[](size_t index) const {
        return m_bitmap[index];
    }

    bool operator==(const ChunkBitmap& otherBitmap) const;
private:
    static Logger sLogger;

    static void constexpr SwapBits32(uint32_t& a, uint32_t& b, uint32_t mask, uint32_t shift) {
        uint32_t t = ((b >> shift) ^ a) & mask;
        a ^= t;
        b ^= (t << shift);
    }

    static void constexpr SwapBits64(uint64_t& a, uint64_t& b, uint64_t mask, uint64_t shift) {
        uint64_t t = ((b >> shift) ^ a) & mask;
        a ^= t;
        b ^= (t << shift);
    }

    alignas(16) std::array<uint32_t, 1024> m_bitmap;
};
