#include <util/Logger.h>
#include <util/Bitmap.h>
#include <util/Morton.h>
#include <world/chunk/types/EightBitChunk.h>
#include <world/chunk/IChunk.h>
#include <chrono>
#include <random>
#include "world/chunk/ChunkBitmap.h"

static void Test() {
    Logger log = Logger("Test");
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();

    log.Info("Test program running!");


    log.Println("\n-+-+-+-+-+-+-+ Testing morton codes:");

    Morton::InitializeLookupTables();

    if (Morton::Test3DEncodingDecoding()) {
        log.Info("Morton decoding and encoding passed tests!");
    } else {
        log.Info("Morton decoding and encoding failed tests!");
    }

    start = std::chrono::high_resolution_clock::now();
    uint64_t code = 12340;
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            for (uint8_t z = 0; z < 32; z++) {
                code += Morton::Encode3DMorton(x, y, z);

            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("32767 Morton Codes encoded! Time taken: ", end - start);

    if (code == 10) 
        log.Warning("Morton code results circumspect.");

    code = 10;
    start = std::chrono::high_resolution_clock::now();
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            for (uint8_t z = 0; z < 32; z++) {
                code += Morton::Decode3DMorton((x << 10) | (y << 5) | z);
            }
        }
    }    
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("32767 Morton Codes decoded! Time taken: ", end - start);
    
    if (code == 10) 
        log.Warning("Morton code results circumspect.");

    log.Println("\n-+-+-+-+-+-+-+ Testing greedy meshing prep:");

    // Create chunk.
    EightBitChunk chunk = EightBitChunk();

    // Create Chunk data.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 1);

    chunk.SetBlock(1, 0, 30, 0);
    chunk.SetBlock(2, 0, 31, 0);
    chunk.SetBlock(4, 0, 28, 0);

    for (uint8_t x = 3; x < 32; x++) {
        for (uint8_t y = 0; y < 16; y++) {
            for (uint8_t z = 0; z < 32; z++) {
                // if (distrib(gen) == 1) {
                    chunk.SetBlock(3, x, y, z);
                // }
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Chunk data inserted! Time taken: ", end - start, " (Naive insert, disregard)");

    // Create greedy mesh variables.
    ChunkBitmap xyz = chunk.GetBlockBitmap(BlockTypes::Air, true);
    ChunkBitmap yxz = xyz.Copy().OuterTranspose();
    ChunkBitmap xzy = xyz.Copy().InnerTranspose();
    ChunkBitmap yzx = yxz.Copy().InnerTranspose();

    // Axis views of visible faces after culling.
    ChunkBitmap zyxPosCulled = xyz.Copy().CullBackBits().SwapOuterInnerAxes();
    ChunkBitmap yzxPosCulled = xzy.Copy().CullBackBits().SwapOuterInnerAxes();
    ChunkBitmap xzyPosCulled = yzx.Copy().CullBackBits().SwapOuterInnerAxes();

    ChunkBitmap zyxNegCulled = xyz.CullFrontBits().SwapOuterInnerAxes();
    ChunkBitmap yzxNegCulled = xzy.CullFrontBits().SwapOuterInnerAxes();
    ChunkBitmap xzyNegCulled = yzx.CullFrontBits().SwapOuterInnerAxes();

    std::vector<uint32_t> vertices;

    // Hyper Greedy mesh.
    start = std::chrono::high_resolution_clock::now();
    code = 0;
    for (int i = 0; i < 100000; i++)
        zyxNegCulled.GreedyMeshBitmap(vertices);
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Greedy total average over 100k: ", (end - start) / 100000);

    // Meshing pass.
    start = std::chrono::high_resolution_clock::now();
    code = 0;
    for (int i = 0; i < 100000; i++)
        chunk.MeshGreedy();
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Greedy average over 100k: ", (end - start) / 100000);

    log.Println("\n-+-+-+-+-+-+-+ Testing air bit map:");
    // Create air bit map.
    start = std::chrono::high_resolution_clock::now();
    ChunkBitmap xyzTest;

    for (int i = 0; i < 100000; i++) {
        xyzTest = chunk.GetBlockBitmap(BlockTypes::eAir, true);
    }
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Air bit map created! Average time taken: ", (end - start) / 100000);

    // for (int i = 0; i < 32; i++) {
    //     log.Verbose("Layer: ", i);
    //     xyz.LogOuterSlice(i);
    // }

    log.Println("\n-+-+-+-+-+-+-+ Testing transposition:");

    // ChunkBitmap xyz = xyzBitmap;

    xyz.TestOuterTransposes();
    xyz.TestInnerTransposes();

    ChunkBitmap innerTestScalar = xyz.Copy();
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
        xyz.InnerTransposeScalar();
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Scalar inner transpose - Average time taken: ", (end - start) / 100000);

    ChunkBitmap innerTest = xyz.Copy();
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
        innerTest.InnerTranspose();
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Simd inner transpose - Average time taken: ", (end - start) / 100000);

    ChunkBitmap swapOuterScalar = xyz.Copy();
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
        swapOuterScalar.OuterTransposeScalar();
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Scalar outer transpose - Average time taken: ", (end - start) / 100000);

    ChunkBitmap swapOuterSimd = xyz.Copy();
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
        swapOuterSimd.OuterTranspose();
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Simd outer transpose - Average time taken: ", (end - start) / 100000);
}
