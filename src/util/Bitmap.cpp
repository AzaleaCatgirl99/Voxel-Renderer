#include "util/Bitmap.h"

#include <sstream>
#include <iomanip>
#include "Bitmap.h"

Logger Bitmap::sLogger = Logger("Bitmap");

// void Bitmap::Outer3DTransposeInPlace(std::array<uint32_t, 1024>& bitmap) {
//     std::array<uint32_t, 1024> newMap;
//     Outer3DTranspose(bitmap, newMap);
//     bitmap = newMap;
// }

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
            uint32_t swap = bitmap[(x << 5) | y] ^ bitmap[(y << 5) | x];
            bitmap[(x << 5) | y] ^= swap;
            bitmap[(y << 5) | x] ^= swap;
        }
    }
}

void Bitmap::Inner3DTransposeSampleMasker(const std::array<uint32_t, 1024>& sourceMap, std::array<uint32_t, 1024>& destinationMap) {
    uint16_t sample;
    for (int i = 0; i < 1024; i += 48)
        sample += std::popcount(sourceMap[i]); // 21  iterations.
    uint32_t mask;

    if (sample <= 336) { // 336 is half of the number of bits sampled.
        destinationMap.fill(0u);

        for (uint8_t x = 0; x < 32; x++) {
            mask = 0;

            for (uint8_t y = 0; y < 32; y++) {
                uint32_t word = sourceMap[x << 5 | y];
                sample = std::popcount(word);

                // Skip zeroes.
                if (sample == 0)
                    continue;
                
                // Skip all 1's case.
                if (sample == 32) {
                    mask |= (1u << y); // Flip the mask bit so we change it to 1 for all.
                    continue;
                }

                // If more ones than zeroes, flip the mask bit and invert the word.
                if (sample > 16) {
                    mask |= (1u << y);
                    word = ~word;
                }

                while (word != 0) {
                    uint8_t z = std::countr_zero(word);
                    word ^= 1u << z; // flip the z bit.

                    destinationMap[x << 5 | z] ^= 1u << y; // flip the relevant bit.
                }
            }

            // skip the empty mask case.
            if (mask == 0)
                continue;

            // Apply mask to all new words.
            for (uint8_t z = 0; z < 32; z++) {
                destinationMap[x << 5 | z] ^= mask;
            }
        }
    } else {
        destinationMap.fill(~0u); // 1's are more common.

        for (uint8_t x = 0; x < 32; x++) {
            mask = 0;

            for (uint8_t y = 0; y < 32; y++) {
                uint32_t word = sourceMap[x << 5 | y];
                sample = std::popcount(word);

                // Skip the all 1's case.
                if (sample == 32)
                    continue;
                
                // Skip all 0's case.
                if (sample == 0) {
                    mask |= (1u << y); // Flip the mask bit so we change it to 0 for all.
                    continue;
                }

                // If more zeroes than ones, flip the mask bit and invert the word.
                if (sample < 16) {
                    mask |= (1u << y);
                    word = ~word;
                }

                while (word != ~0u) {
                    uint8_t z = std::countr_one(word);
                    word ^= 1u << z; // flip the index bit.

                    destinationMap[x << 5 | z] ^= 1u << y; // flip the relevant bit.
                }
            }

            // skip the empty mask case.
            if (mask == 0)
                continue;

            // Apply mask to all new words.
            for (uint8_t z = 0; z < 32; z++) {
                destinationMap[x << 5 | z] ^= mask;
            }
        }
    }
}

void Bitmap::Inner3DTransposeNaive(const std::array<uint32_t, 1024>& sourceMap, std::array<uint32_t, 1024>& destinationMap) {
    destinationMap = {};
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            for (int z = 0; z < 32; z++) {
                uint8_t bit = (sourceMap[x << 5 | y] >> z) & 1u;
                destinationMap[x << 5 | z] ^= bit << y;
            }
        }
    }
}

void Bitmap::Inner3DTransposeSampleSkipper(const std::array<uint32_t, 1024>& sourceMap, std::array<uint32_t, 1024>& destinationMap) {
    uint16_t sample;
    for (int i = 0; i < 1024; i += 48)
        sample += std::popcount(sourceMap[i]);
    
    if (sample <= 336) {
        destinationMap.fill(0u);
        for (int x = 0; x < 32; x++) {
            for (int y = 0; y < 32; y++) {
                uint32_t word =  sourceMap[x << 5u | y];

                while (word != 0) {
                    uint8_t z = std::countr_zero(word);
                    word ^= 1 << z;
                    destinationMap[x << 5 | z] ^= 1u << y;
                }
            }
        }
    } else {
        destinationMap.fill(~0u);
        for (int x = 0; x < 32; x++) {
            for (int y = 0; y < 32; y++) {
                uint32_t word =  sourceMap[x << 5 | y];

                while (word != ~0u) {
                    uint8_t z = std::countr_one(word);
                    word ^= 1 << z;
                    destinationMap[x << 5 | z] ^= 1u << y;
                }
            }
        }
    }
}

// void Bitmap::Inner3DTranspose(const std::array<uint32_t, 1024>& sourceMap, std::array<uint32_t, 1024>& destinationMap) {
//     destinationMap = sourceMap;
//     Inner3DTransposeInPlace(destinationMap);
// }

void Bitmap::Inner3DTranspose(std::array<uint32_t, 1024>& bitmap) {
    uint64_t* data64 = reinterpret_cast<uint64_t*>(bitmap.data());
    
    // Process in specific chunk sizes to optimize locality.
    constexpr int chunkSize = 4; // 4 is best for me, very slightly better than not chunking.
    
    for (int chunkStart = 0; chunkStart < 32; chunkStart += chunkSize) {
        int chunkEnd = std::min(chunkStart + chunkSize, 32);
        
        // Pipeline Stage 1 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = data64 + layer * 16;
            #pragma unroll(8)
            for (int i = 0; i < 8; i++) {
                SwapBits64(slice64[i], slice64[i+8], 0x0000FFFF0000FFFF, 16);
            }
        }
        
        // Pipeline Stage 2 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = data64 + layer * 16;
            #pragma unroll(4)
            for (int i = 0; i < 4; i++) {
                SwapBits64(slice64[i], slice64[i+4], 0x00FF00FF00FF00FF, 8);
                SwapBits64(slice64[i+8], slice64[i+12], 0x00FF00FF00FF00FF, 8);
            }
        }
        
        // Pipeline Stage 3 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = data64 + layer * 16;
            #pragma unroll(2)
            for (int i = 0; i < 2; i++) {
                SwapBits64(slice64[i], slice64[i+2], 0x0F0F0F0F0F0F0F0F, 4);
                SwapBits64(slice64[i+4], slice64[i+6], 0x0F0F0F0F0F0F0F0F, 4);
                SwapBits64(slice64[i+8], slice64[i+10], 0x0F0F0F0F0F0F0F0F, 4);
                SwapBits64(slice64[i+12], slice64[i+14], 0x0F0F0F0F0F0F0F0F, 4);
            }
        }
        
        // Pipeline Stage 4+5 for the chunk.
        for (int layer = chunkStart; layer < chunkEnd; layer++) {
            uint64_t* slice64 = data64 + layer * 16;
            #pragma unroll(4)  
            for (int i = 0; i < 16; i += 4) {
                SwapBits64(slice64[i], slice64[i+1], 0x3333333333333333ULL, 2);
                SwapBits64(slice64[i], slice64[i+1], 0x5555555555555555ULL, 1);
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
    std::array<uint32_t, 1024> sampleSkipper;
    std::array<uint32_t, 1024> sampleMasker;
    std::array<uint32_t, 1024> fast = sourceMap;

    Inner3DTransposeNaive(sourceMap, naive);
    Inner3DTransposeSampleSkipper(sourceMap, sampleSkipper);
    Inner3DTransposeSampleMasker(sourceMap, sampleMasker);
    Inner3DTranspose(fast);

    bool isEqual = true;

    if (!Is3DEqual(naive, sampleSkipper)) {
        sLogger.Warning("Naive and sample skipper do not match!");
        isEqual = false;
    }

    if (!Is3DEqual(naive, sampleMasker)) {
        sLogger.Warning("Naive and sample masker do not match!");
        isEqual = false;
    }

    if (!Is3DEqual(sampleSkipper, sampleMasker)) {
        sLogger.Warning("sample skipper and sample masker do not match!");
        isEqual = false;
    }

    if (!Is3DEqual(naive, fast)) {
        sLogger.Warning("naive and fast do not match!");
        isEqual = false;
    }

    // sLogger.Verbose("Source:");
    // Log3DSlice(sourceMap);

    // sLogger.Verbose("Naive:");
    // Log3DSlice(naive);

    // sLogger.Verbose("Fast:");
    // Log3DSlice(fast);

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
    for (int i = 31; i >= 0; i--) {
        sLogger.Verbose(std::bitset<32>(sourceMap[i + (32 * layer)]));
    }
}
