#pragma once

#include "util/Logger.h"
#include <cstdint>

class Morton final {
public:
    static void LogMortons();

    static VXL_INLINE uint16_t Encode3DMorton(const uint8_t x, const uint8_t y, const uint8_t z) {
        return sFullMortonEncode3D[(x << 10) | (y << 5) | z];
    }

    static VXL_INLINE uint16_t Decode3DMorton(const uint16_t mortonCode) {
        return sFullMortonDecode3D[mortonCode];
    }

    static void InitializeLookupTables();
    
    static bool Test3DEncodingDecoding();
private:
    static VXL_INLINE uint16_t Encode3DMortonManual(uint8_t x, uint8_t y, uint8_t z) {
        return sMortonEncode3DX[x] | sMortonEncode3DY[y] | sMortonEncode3DZ[z];
    };

    static VXL_INLINE uint16_t Decode3DMorton3LUTs(const uint16_t mortonCode) {
        return sMortonDecode3DHigh[mortonCode >> 10] | sMortonDecode3DMid[(mortonCode >> 5) & 31] | sMortonDecode3DLow[mortonCode & 31];
    }

    static VXL_INLINE uint16_t Decode3DMortonBitManual(const uint16_t mortonCode) {
        // Parallel shift non-overlapping bit subsets into high word, combine, put into low word.
        return
            (((((mortonCode & 0x2854) * 0x820800) & 0x13220000) |
            (((mortonCode & 0x42A1) * 0x208200) & 0x24448000) |
            (((mortonCode & 0x150A) * 0x082080) & 0x08990000)) >> 15);
    };

    static Logger sLogger;

    static const uint16_t sMortonEncode3DX[32];
    static const uint16_t sMortonEncode3DY[32];
    static const uint16_t sMortonEncode3DZ[32];

    static const uint16_t sMortonDecode3DLow[32];
    static const uint16_t sMortonDecode3DMid[32];
    static const uint16_t sMortonDecode3DHigh[32];

    static uint16_t sFullMortonEncode3D[32768];
    static uint16_t sFullMortonDecode3D[32768];
};
