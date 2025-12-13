#include <chrono>
#include <random>
#include "util/Logger.h"
#include "util/Morton.h"
#include "world/chunk/IChunk.h"
#include "world/chunk/types/EightBitChunk.h"
#include "world/chunk/ChunkBitmap.h"

static void Test() {
    Logger log = Logger("Test");
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();

    log.Info("Test program running!");


    // log.Println("\n-+-+-+-+-+-+-+ Testing morton codes:");

    // Morton::InitializeLookupTables();

    // if (Morton::Test3DEncodingDecoding()) {
    //     log.Info("Morton decoding and encoding passed tests!");
    // } else {
    //     log.Info("Morton decoding and encoding failed tests!");
    // }

    // start = std::chrono::high_resolution_clock::now();
    // uint64_t code = 12340;
    // for (uint8_t x = 0; x < 32; x++) {
    //     for (uint8_t y = 0; y < 32; y++) {
    //         for (uint8_t z = 0; z < 32; z++) {
    //             code += Morton::Encode3DMorton(x, y, z);

    //         }
    //     }
    // }
    // end = std::chrono::high_resolution_clock::now();
    // log.Verbose("32767 Morton Codes encoded! Time taken: ", end - start);

    // if (code == 10) 
    //     log.Warning("Morton code results circumspect.");

    // code = 10;
    // start = std::chrono::high_resolution_clock::now();
    // for (uint8_t x = 0; x < 32; x++) {
    //     for (uint8_t y = 0; y < 32; y++) {
    //         for (uint8_t z = 0; z < 32; z++) {
    //             code += Morton::Decode3DMorton((x << 10) | (y << 5) | z);
    //         }
    //     }
    // }    
    // end = std::chrono::high_resolution_clock::now();
    // log.Verbose("32767 Morton Codes decoded! Time taken: ", end - start);
    
    // if (code == 10) 
    //     log.Warning("Morton code results circumspect.");

    // log.Println("\n-+-+-+-+-+-+-+ Testing greedy meshing prep:");

    // Create chunk.
    EightBitChunk chunk = EightBitChunk();

    // Create Chunk data.
    std::random_device rd;
    std::mt19937 gen(100);
    std::uniform_int_distribution<> distrib(0, 100);

    // chunk.SetBlock(1, 15, 30, 5);
    // chunk.SetBlock(2, 0, 31, 0);
    // chunk.SetBlock(4, 0, 28, 0);

    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            for (uint8_t z = 16; z < 32; z++) {
                // if (distrib(gen) < 50)
                    chunk.SetBlock(1, x, y, z);
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Chunk data inserted! Time taken: ", end - start, " (Naive insert, disregard)");


    std::vector<uint32_t> testVec;
    testVec.reserve(50000);
    start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < 50000; i++)
        testVec.push_back(i);
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("50k vector insert: ", end - start);


    // Create greedy mesh variables.
    ChunkBitmap xyz = chunk.GetBlockBitmap(BlockTypes::eAir, true);

    // std::vector<uint32_t> vertices;

    // // for (int i = 0; i < 16; i++) {
    // //     log.Verbose("Layer: ", i);
    // //     zyxNegCulled.LogInnerSlice(i);
    // // }
    // // return;

    // vertices.reserve(6000000);
    // // Greedy mesh.
    // start = std::chrono::high_resolution_clock::now();
    // code = 0;
    // for (int i = 0; i < 1000; i++)
    //     zyxNegCulled.Copy().GreedyMeshBitmap(vertices, 10, 5, 0);

    // end = std::chrono::high_resolution_clock::now();
    // log.Verbose("Greedy bitmap single pass average over 1k: ", (end - start) / 1000);

    // log.Verbose("Num of vertices per pass: ", vertices.size() / 1000);

    // Meshing pass.
    uint32_t count = 0;
    ChunkMesh::Greedy mesh;
    mesh.m_vertices.reserve(50000);
    start = std::chrono::high_resolution_clock::now();
    // code = 0;
    for (int i = 0; i < 1000; i++) {
        chunk.MeshGreedy(mesh);
        count = mesh.m_vertices.size();
        mesh.m_vertices.clear();
    }
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Chunk with ", count, " faces done on average in: ", (end - start) / 1000);

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

    // for (int i = 0; i < 32; i++) {
    //     log.Verbose("Layer: ", i);
    //     xyzTest.LogInnerSlice(i);
    // }

    // log.Println("\n-+-+-+-+-+-+-+ Testing culling:");
    // log.Verbose("Source: ");
    // xyzTest.LogInnerSlice();
    // log.Verbose("After culling:");
    // xyzTest.CullLeastSigBits();
    // xyzTest.LogInnerSlice();

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
