#include <util/Logger.h>
#include <util/Bitmap.h>
#include <util/Morton.h>
#include <world/chunk/types/EightBitChunk.h>
#include <world/chunk/IChunk.h>
#include <chrono>
#include <random>

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

    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            for (uint8_t z = 0; z < (32 - y); z++) {
                // if (distrib(gen) == 1) {
                    chunk.SetBlock(3, x, y, z);
                // }
            }
        }
    }
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Chunk data inserted! Time taken: ", end - start, " (Naive insert, disregard)");

    // Hyper Greedy mesh.
    start = std::chrono::high_resolution_clock::now();
    code = 0;
    for (int i = 0; i < 1; i++)
        chunk.MeshHyperGreedy();
    end = std::chrono::high_resolution_clock::now();
    // log.Verbose("Hyper greedy data prep average over 100k: ", (end - start) / 1);

    log.Println("\n-+-+-+-+-+-+-+ Testing air bit map:");
    // Create air bit map.
    start = std::chrono::high_resolution_clock::now();
    alignas(16) std::array<uint32_t, 1024> xyzBitmap{};
    for (int i = 0; i < 100000; i++)
        chunk.GetSolidBitmap(xyzBitmap);
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Air bit map created! Average time taken: ", (end - start) / 100000);

    // for (int i = 0; i < 32; i++) {
    //     log.Verbose("Layer: ", i);
    //     Bitmap::Log3DSlice(xyzBitmap, i);
    // }

    log.Println("\n-+-+-+-+-+-+-+ Testing transposition:");

    if (Bitmap::TestInner3DTransposes(xyzBitmap) && Bitmap::TestOuter3DTransposes(xyzBitmap))
        log.Verbose("Accuracy of transposition algorithms verified!");

    alignas(16) std::array<uint32_t, 1024> innerTestScalar = xyzBitmap;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
        Bitmap::Inner3DTransposeScalar(innerTestScalar);
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Scalar inner transpose - Average time taken: ", (end - start) / 100000);

    alignas(16) std::array<uint32_t, 1024> innerTest = xyzBitmap;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
        Bitmap::Inner3DTranspose(innerTest);
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Simd inner transpose - Average time taken: ", (end - start) / 100000);

    std::array<uint32_t, 1024> swapOuter = xyzBitmap;
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++)
        Bitmap::Outer3DTranspose(swapOuter);
    end = std::chrono::high_resolution_clock::now();
    log.Verbose("Outer transpose - Average time taken: ", (end - start) / 100000);
}
