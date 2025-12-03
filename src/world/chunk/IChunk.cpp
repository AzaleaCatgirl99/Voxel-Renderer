#include "world/chunk/IChunk.h"

#include <bit>
#include <util/Bitmap.h>
#include <chrono>

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
    sLogger.Verbose("Data allocation completed in ", end - start);

    start = std::chrono::high_resolution_clock::now();
    GetAirBitmap(xyzPos);
    end = std::chrono::high_resolution_clock::now();
    sLogger.Verbose("Air map created in ", end - start);

    start = std::chrono::high_resolution_clock::now();
    yxzPos = xyzPos;
    Bitmap::Outer3DTranspose(yxzPos);
    end = std::chrono::high_resolution_clock::now();
    sLogger.Verbose("Outer transpose 1 done in ", end - start);

    start = std::chrono::high_resolution_clock::now();
    xzyPos = xyzPos;
    Bitmap::Inner3DTranspose(xzyPos);
    end = std::chrono::high_resolution_clock::now();
    sLogger.Verbose("Inner transpose 1 done in ", end - start);

    start = std::chrono::high_resolution_clock::now();
    yzxPos = yxzPos;
    Bitmap::Inner3DTranspose(yzxPos);
    end = std::chrono::high_resolution_clock::now();
    sLogger.Verbose("Inner transpose 2 done in ", end - start);

    ChunkMesh::HyperGreedy mesh;
    mesh.m_mortonBlocks.push_back(xzyPos[0]);

    start = std::chrono::high_resolution_clock::now();
    std::array<uint32_t, 1024> xyzNeg = xyzPos;
    std::array<uint32_t, 1024> xzyNeg = xzyPos;
    std::array<uint32_t, 1024> yzxNeg = yzxPos;
    end = std::chrono::high_resolution_clock::now();
    sLogger.Verbose("Data cloning done in ", end - start);

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

    start = std::chrono::high_resolution_clock::now();
    for (uint16_t i = 0; i < 1024; i++) {
        xyzPos[i] &= ~(xyzPos[i] >> 1);
        xzyPos[i] &= ~(xzyPos[i] >> 1);
        yzxPos[i] &= ~(yzxPos[i] >> 1);

        xyzNeg[i] &= ~(xyzNeg[i] << 1);
        xzyNeg[i] &= ~(xzyNeg[i] << 1);
        yzxNeg[i] &= ~(yzxNeg[i] << 1);
    }
    end = std::chrono::high_resolution_clock::now();
    sLogger.Verbose("Face culling done in ", end - start);

    start = std::chrono::high_resolution_clock::now();
    Bitmap::SwapOuterInnerAxes(xyzPos);
    Bitmap::SwapOuterInnerAxes(xzyPos);
    Bitmap::SwapOuterInnerAxes(yzxPos);

    Bitmap::SwapOuterInnerAxes(xyzNeg);
    Bitmap::SwapOuterInnerAxes(xzyNeg);
    Bitmap::SwapOuterInnerAxes(yzxNeg);
    end = std::chrono::high_resolution_clock::now();
    sLogger.Verbose("Axis alignment done in ", end - start);

    // for (int i = 14; i < 17; i++) {      
    //     sLogger.Verbose("Layer: ", i);
    //     Bitmap::Log3DSlice(yzxPosCulled, i);
    // }
    return mesh;
}
