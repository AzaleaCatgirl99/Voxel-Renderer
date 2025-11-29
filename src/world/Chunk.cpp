#include "world/Chunk.h"

Logger Chunk::sLogger = Logger("Chunk");

#define NUM_BLOCK_TYPES 16

Chunk::Chunk(ChunkPacking packingType) {
    m_packingMode = packingType;
    
    m_blockData = ChunkData(packingType);

    uint32_t maxSize = 1 << static_cast<uint8_t>(packingType); // Identify the number of values that can be represented with the packing type.
    m_blockPalette.Reserve(maxSize);
    m_blockPaletteIndices.reserve(maxSize);
    m_blockPaletteCounts = std::vector<uint16_t>(NUM_BLOCK_TYPES, 0);
}

uint16_t Chunk::GetBlock(const uint8_t x, const uint8_t y, const uint8_t z) const {
    const uint16_t index = (x << 10) | (y << 5) | z;
    return m_blockData.Get(index);
}

uint16_t Chunk::SetBlock(const uint16_t newBlock, const uint8_t x, const uint8_t y, const uint8_t z) {
    const uint16_t index = (x << 10) | (y << 5) | z;
    uint16_t oldBlock = m_blockData.GetSet(index, newBlock);

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

ChunkMesh::Naive Chunk::MeshNaive() {
    ChunkMesh::Naive mesh;

    uint16_t index;
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            for (uint8_t z = 0; z < 32; z++) {
                const uint8_t paletteIndex = m_blockData.Get(index);
                const uint16_t blockType = m_blockPalette[paletteIndex];

                uint32_t packedBits = (blockType << 15) | (x << 10) | (y << 5) | z;
                mesh.m_vertices.push_back(packedBits);

                index++;
            }
        }
    }

    return mesh;
}
