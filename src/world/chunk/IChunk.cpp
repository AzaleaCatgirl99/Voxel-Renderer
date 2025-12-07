#include "world/chunk/IChunk.h"

#include <bit>
#include <util/Bitmap.h>
#include <chrono>
#include "IChunk.h"

Logger IChunk::sLogger = Logger("Chunk");

#define NUM_BLOCK_TYPES 16

void IChunk::Initialize(ChunkPacking packingType) {
    m_packingMode = packingType;
    
    uint32_t maxSize = 1 << static_cast<uint8_t>(packingType); // Identify the number of values that can be represented with the packing type.
    m_blockPalette.Reserve(maxSize);

    m_blockPalette.Insert(BlockTypes::Air);
    m_blockPaletteIndices[0] = 0;
    m_blockPaletteCounts[0] = 32768;
}

uint16_t IChunk::GetBlock(const uint8_t x, const uint8_t y, const uint8_t z) const {
    const uint16_t index = (x << 10) | (y << 5) | z;
    return RawGetBlock(index);
}

uint16_t IChunk::SetBlock(const uint16_t newBlock, const uint8_t x, const uint8_t y, const uint8_t z) {
    const uint16_t index = (x << 10) | (y << 5) | z;
    uint16_t oldBlock = RawGetBlock(index);
    RawSetBlock(index, newBlock);

    if (--m_blockPaletteCounts[oldBlock] == 0) {
        const uint8_t index = m_blockPaletteIndices[oldBlock];
        m_blockPalette.Delete(index);
    }

    if (++m_blockPaletteCounts[newBlock] == 1) {
        if (m_blockPalette.IsFull()) 
            throw sLogger.RuntimeError("Chunk palette is full! Dynamic chunk reformatting not yet implemented.");

        const uint8_t index = m_blockPalette.Insert(newBlock);
        m_blockPaletteIndices[newBlock] = index;
    }

    return oldBlock;
}

ChunkMesh::Naive IChunk::MeshNaive() {
    ChunkMesh::Naive mesh;

    uint16_t index;
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            for (uint8_t z = 0; z < 32; z++) {
                const uint8_t paletteIndex = RawGetBlock(index);
                const uint16_t blockType = m_blockPalette[paletteIndex];

                uint32_t packedBits = (blockType << 15) | (x << 10) | (y << 5) | z;
                mesh.m_vertices.push_back(packedBits);

                index++;
            }
        }
    }

    return mesh;
}

ChunkMesh::HyperGreedy IChunk::MeshHyperGreedy() {
    auto start = std::chrono::high_resolution_clock::now();
    std::array<uint32_t, 1024> xyzPos;
    std::array<uint32_t, 1024> yxzPos;
    std::array<uint32_t, 1024> xzyPos;
    std::array<uint32_t, 1024> yzxPos;

    auto end = std::chrono::high_resolution_clock::now();
    // sLogger.Verbose("Data allocation completed in ", end - start);

    // start = std::chrono::high_resolution_clock::now();
    GetSolidBitmap(xyzPos);
    // end = std::chrono::high_resolution_clock::now();
    // sLogger.Verbose("Air map created in ", end - start);

    // start = std::chrono::high_resolution_clock::now();
    yxzPos = xyzPos;
    Bitmap::Outer3DTranspose(yxzPos);
    // end = std::chrono::high_resolution_clock::now();
    // sLogger.Verbose("Outer transpose 1 done in ", end - start);

    // start = std::chrono::high_resolution_clock::now();
    xzyPos = xyzPos;
    Bitmap::Inner3DTranspose(xzyPos);
    // end = std::chrono::high_resolution_clock::now();
    // sLogger.Verbose("Inner transpose 1 done in ", end - start);

    // start = std::chrono::high_resolution_clock::now();
    yzxPos = yxzPos;
    Bitmap::Inner3DTranspose(yzxPos);
    // end = std::chrono::high_resolution_clock::now();
    // sLogger.Verbose("Inner transpose 2 done in ", end - start);

    ChunkMesh::HyperGreedy mesh;

    // start = std::chrono::high_resolution_clock::now();
    std::array<uint32_t, 1024> xyzNeg = xyzPos;
    std::array<uint32_t, 1024> xzyNeg = xzyPos;
    std::array<uint32_t, 1024> yzxNeg = yzxPos;
    // end = std::chrono::high_resolution_clock::now();
    // sLogger.Verbose("Data cloning done in ", end - start);

    // for (int i = 0; i < 32; i++) {
    //     sLogger.Verbose("Logging slice ", i);
    //     Bitmap::Log3DSlice(xyzPos, i);
    // }
    // sLogger.Verbose("Logging slice of xyz:");
    // Bitmap::Log3DSlice(xyzPos);
    // sLogger.Verbose("Logging slice of yxz:");
    // Bitmap::Log3DSlice(yxzPos);
    // sLogger.Verbose("Logging slice of xzy:");
    // Bitmap::Log3DSlice(xzyPos);
    // sLogger.Verbose("Logging slice of yzx:");
    // Bitmap::Log3DSlice(yzxPos);

    // Bitmap::Log3DSlice(xzyPos, 1);

    // for (int i = 0; i < 16; i++) {      
    //     sLogger.Verbose("Layer: ", i);
    //     Bitmap::Log3DSlice(xzyPos, i);
    // }

    // start = std::chrono::high_resolution_clock::now();
    for (uint16_t i = 0; i < 1024; i++) {
        xyzPos[i] &= ~(xyzPos[i] >> 1);
        xzyPos[i] &= ~(xzyPos[i] >> 1);
        yzxPos[i] &= ~(yzxPos[i] >> 1);

        xyzNeg[i] &= ~(xyzNeg[i] << 1);
        xzyNeg[i] &= ~(xzyNeg[i] << 1);
        yzxNeg[i] &= ~(yzxNeg[i] << 1);
    }
    // end = std::chrono::high_resolution_clock::now();
    // sLogger.Verbose("Face culling done in ", end - start);

    // start = std::chrono::high_resolution_clock::now();

    // xyz .
    // xzy .
    // zxy
    // zyx .

    Bitmap::SwapOuterInnerAxes(xyzPos); // zyx
    Bitmap::SwapOuterInnerAxes(xzyPos); // yxz : xyz -> yxz
    Bitmap::SwapOuterInnerAxes(yzxPos); // xzy : 

    Bitmap::SwapOuterInnerAxes(xyzNeg);
    Bitmap::SwapOuterInnerAxes(xzyNeg);
    Bitmap::SwapOuterInnerAxes(yzxNeg);
    // end = std::chrono::high_resolution_clock::now();
    // sLogger.Verbose("Axis alignment done in ", end - start);

    // for (int i = 14; i < 17; i++) {      
    //     sLogger.Verbose("Layer: ", i);
    //     Bitmap::Log3DSlice(yzxPosCulled, i);
    // }

    std::vector<uint32_t> faces;
    faces.reserve(100);

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
        GreedyMeshBitmap(faces, xyzPos, 0);
    end = std::chrono::high_resolution_clock::now();
    sLogger.Verbose(" --- Greedy meshing done on average in ", (end - start) / 100000);

    return mesh;
}

void IChunk::GreedyMeshBitmap(std::vector<uint32_t>& vertices, std::array<uint32_t, 1024>& bitmap, int normal) const {
    int n = 0;
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {

            if (bitmap[(x << 5) | y] == 0)
                continue;

            uint16_t index = (x << 5) | y;
            uint32_t bits = bitmap[index];

            if (bits == 0)
                continue;
            
            uint8_t z = std::countl_zero(bits);
            uint8_t height = std::countl_one(bits << z);
            uint8_t width;

            uint32_t mask = bits & (~0 << (32 - z + height));
            bitmap[index] ^= mask;

            for (width = 1; width < (32 - x); width++) {
                if ((bitmap[index + width] & mask) != mask)
                    break;

                bitmap[index + width] ^= mask;
            }

            vertices.push_back((height << 20) | (width << 15) | (x << 10) | (y << 5) | z);
        }
    }
}

// void IChunk::GreedyMeshBitmap(std::vector<uint32_t>& vertices, std::array<uint32_t, 1024>& bitmap, int normal) const {
//     int n = 0;

//     // auto start = std::chrono::high_resolution_clock::now();
//     // Preprocessing: compute a 32-bit column summary
//     uint32_t columnBitmap[32] = {}; // Each bit represents if there exists a 1 in that row (y)
//     for (int x = 0; x < 32; x++) {
//         __m256i acc = _mm256_setzero_si256();
//         for (int y = 0; y < 32; y += 8) {
//             // Load 8 consecutive 32-bit words in the column
//             uint32_t temp[8];
//             for (int k = 0; k < 8; k++)
//                 temp[k] = bitmap[(x << 5) | (y + k)];
//             __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(temp));
//             acc = _mm256_or_si256(acc, vec);
//         }

//         // Horizontal OR to produce a 32-bit column mask
//         alignas(32) uint32_t tmp[8];
//         _mm256_store_si256(reinterpret_cast<__m256i*>(tmp), acc);
//         uint32_t colMask = tmp[0] | tmp[1] | tmp[2] | tmp[3] | tmp[4] | tmp[5] | tmp[6] | tmp[7];
//         columnBitmap[x] = colMask;
//     }

//     // auto end = std::chrono::high_resolution_clock::now();
//     // sLogger.Verbose(" --- Greedy meshing preprocessing done in ", (end - start) / 1);

//     // start = std::chrono::high_resolution_clock::now();

//     // Main greedy mesh loop
//     for (int x = 0; x < 32; x++) {
//         uint32_t colMask = columnBitmap[x];
//         if (colMask == 0)
//             continue; // skip entire column if nothing set

//         int y = 0;
//         while (y < 32) {
//             // skip zeros in this column using ctz
//             uint32_t maskShifted = colMask >> y;
//             if (maskShifted == 0)
//                 break; // no more ones
//             int skip = std::countr_zero(maskShifted);
//             y += skip;

//             uint16_t index = (x << 5) | y;
//             uint32_t bits = bitmap[index];

//             uint8_t z = std::countl_zero(bits);
//             uint8_t height = std::countl_one(bits << z);
//             uint8_t width;

//             uint32_t mask = bits & (~0u << (32 - z + height));
//             bitmap[index] ^= mask;

//             for (width = 1; width < (32 - x); width++) {
//                 if ((bitmap[index + width] & mask) != mask)
//                     break;
//                 bitmap[index + width] ^= mask;
//             }

//             vertices.push_back((height << 20) | (width << 15) | (x << 10) | (y << 5) | z);

//             y += height; // skip past this vertical run
//         }
//     }

//     // end = std::chrono::high_resolution_clock::now();
//     // sLogger.Verbose(" --- Greedy main loop done in ", (end - start) / 1);
// }
