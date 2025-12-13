#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include "util/Logger.h"

enum AxisOrder : uint8_t {
    eXYZ = 0,
    eXZY = 1,
    eYXZ = 2,
    eYZX = 3,
    eZXY = 4,
    eZYX = 5
};

class ChunkBitmap final {
public:
    ChunkBitmap() = default;

    ChunkBitmap(const ChunkBitmap& otherBitmap) : m_bitmap(otherBitmap.m_bitmap), m_axisOrder(otherBitmap.m_axisOrder) {};

    ChunkBitmap(const std::array<uint32_t, 1024>& otherBitmap) : m_bitmap(otherBitmap) {};

    void GreedyMeshBitmap(std::vector<uint32_t>& vertices);
    
    ChunkBitmap& CullMostSigBits();

    ChunkBitmap& CullLeastSigBits();

    ChunkBitmap& OuterTranspose();

    void OuterTransposeNaive(ChunkBitmap& newMap);

    void OuterTransposeScalar();

    ChunkBitmap& InnerTranspose();

    void InnerTransposeNaive(ChunkBitmap& newMap);

    void InnerTransposeScalar();

    bool TestOuterTransposes() const;

    bool TestInnerTransposes() const;

    void LogInnerSlice(uint8_t layer = 0) const;

    void LogOuterSlice(uint8_t layer = 0) const;

    ChunkBitmap& And(const ChunkBitmap& otherMap);
    
    VXL_INLINE uint32_t* Data() noexcept {
        return m_bitmap.data();
    }

    VXL_INLINE ChunkBitmap Copy() const {
        return ChunkBitmap(*this);
    }

    VXL_INLINE auto& operator[](size_t index) {
        return m_bitmap[index];
    }

    VXL_INLINE auto& operator[](size_t index) const {
        return m_bitmap[index];
    }

    bool operator==(const ChunkBitmap& otherBitmap) const;
private:
    static Logger sLogger;

    static std::array<AxisOrder, 6> sAxisOrderAfterOuter;
    static std::array<AxisOrder, 6> sAxisOrderAfterInner;

    VXL_INLINE void UpdateAxisAfterOuter() {
        m_axisOrder = sAxisOrderAfterOuter[m_axisOrder];
    }

    VXL_INLINE void UpdateAxisAfterInner() {
        m_axisOrder = sAxisOrderAfterInner[m_axisOrder];
    }

    static void VXL_INLINE SwapBits32(uint32_t& a, uint32_t& b, uint32_t mask, uint32_t shift) {
        uint32_t t = ((b >> shift) ^ a) & mask;
        a ^= t;
        b ^= (t << shift);
    }

    static void VXL_INLINE SwapBits64(uint64_t& a, uint64_t& b, uint64_t mask, uint64_t shift) {
        uint64_t t = ((b >> shift) ^ a) & mask;
        a ^= t;
        b ^= (t << shift);
    }

    alignas(16) std::array<uint32_t, 1024> m_bitmap;

    AxisOrder m_axisOrder;
};
