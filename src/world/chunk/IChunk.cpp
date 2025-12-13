#include "world/chunk/IChunk.h"

#include <bit>
#include "world/chunk/ChunkBitmap.h"

Logger IChunk::sLogger = Logger("Chunk");

#define NUM_BLOCK_TYPES 16

void IChunk::Initialize(ChunkPacking packingType) {
    m_packingMode = packingType;
    
    uint32_t maxSize = 1 << static_cast<uint8_t>(packingType); // Identify the number of values that can be represented with the packing type.
    m_blockPalette.Reserve(maxSize);

    m_blockPalette.Insert(BlockTypes::eAir);
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

void IChunk::MeshGreedy(ChunkMesh::Greedy& mesh) {
    // Temporary variables for the three axis views. Do not use after culling.
    ChunkBitmap xyzMask = GetBlockBitmap(BlockTypes::eAir, true);
    ChunkBitmap xzyMask = xyzMask.Copy().InnerTranspose();
    ChunkBitmap yzxMask = xyzMask.Copy().OuterTranspose().InnerTranspose();

    // Axis views of visible faces after culling.
    ChunkBitmap zxyPosCulledMask = xyzMask.Copy().CullMostSigBits().InnerTranspose().OuterTranspose();
    ChunkBitmap yxzPosCulledMask = xzyMask.Copy().CullMostSigBits().InnerTranspose().OuterTranspose();
    ChunkBitmap xyzPosCulledMask = yzxMask.Copy().CullMostSigBits().InnerTranspose().OuterTranspose();

    ChunkBitmap zxyNegCulledMask = xyzMask.CullLeastSigBits().InnerTranspose().OuterTranspose();
    ChunkBitmap yxzNegCulledMask = xzyMask.CullLeastSigBits().InnerTranspose().OuterTranspose();
    ChunkBitmap xyzNegCulledMask = yzxMask.CullLeastSigBits().InnerTranspose().OuterTranspose();

    for (uint32_t block = 1; block < m_blockPaletteCounts.size(); block++) {

        if (m_blockPaletteCounts[block] == 0)
            continue;

        // Temporary variables for the three axis views. Do not use after masking.
        ChunkBitmap xyz = GetBlockBitmap(static_cast<BlockTypes>(block));
        ChunkBitmap yxz = xyz.Copy().OuterTranspose();
        ChunkBitmap zxy = xyz.Copy().InnerTranspose().OuterTranspose();

        // Axis views of specific blocks. Use precomputed faces to speed up data prep.
        ChunkBitmap xyzPosCulled = xyz.Copy().And(xyzPosCulledMask);
        ChunkBitmap yxzPosCulled = yxz.Copy().And(yxzPosCulledMask);
        ChunkBitmap zxyPosCulled = zxy.Copy().And(zxyPosCulledMask);

        ChunkBitmap xyzNegCulled = xyz.And(xyzNegCulledMask);
        ChunkBitmap yxzNegCulled = yxz.And(yxzNegCulledMask);
        ChunkBitmap zxyNegCulled = zxy.And(zxyNegCulledMask);

        xyzPosCulled.GreedyMeshBitmap(mesh.m_vertices);
        yxzPosCulled.GreedyMeshBitmap(mesh.m_vertices);
        zxyPosCulled.GreedyMeshBitmap(mesh.m_vertices);

        xyzNegCulled.GreedyMeshBitmap(mesh.m_vertices);
        yxzNegCulled.GreedyMeshBitmap(mesh.m_vertices);
        zxyNegCulled.GreedyMeshBitmap(mesh.m_vertices);
    }
}
