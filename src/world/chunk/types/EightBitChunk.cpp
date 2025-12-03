#include <world/chunk/types/EightBitChunk.h>

Logger EightBitChunk::sLogger = Logger("EightBitChunk");

EightBitChunk::EightBitChunk() {
    Initialize(ChunkPacking::Eight);
}

EightBitChunk::EightBitChunk(std::array<uint8_t, 32768>& blockData) {
    std::memcpy(m_blockData.data(), blockData.data(), sizeof(m_blockData));
}

uint16_t EightBitChunk::RawGetBlock(const uint16_t index) const {
    return m_blockData[index];
}

void EightBitChunk::RawSetBlock(const uint16_t index, const uint16_t newBlock) {
    m_blockData[index] = newBlock;
    // sLogger.Verbose("Setting ", index, " to ", newBlock);
}

// void EightBitChunk::GetAirBitmap(std::array<uint32_t, 1024>& bitmap) const {
//     uint8_t airIndex = static_cast<uint8_t>(m_blockPaletteIndices[BlockTypes::Air]);

//     if (m_blockPaletteCounts[BlockTypes::Air] >= 16384) {
//         bitmap.fill(0u);
//         for (size_t i = 0; i < 32768; i++) {
//             if (m_blockData[i] != airIndex) {
//                 bitmap[i >> 5] |= (1u << (i & 31));
//             }
//         }
//     } else {
//         bitmap.fill(~0u);
//         for (size_t i = 0; i < 32768; i++) {
//             if (m_blockData[i] == airIndex) {
//                 bitmap[i >> 5] ^= (1u << (i & 31));
//             }
//         }
//     }
// }

void EightBitChunk::GetAirBitmap(std::array<uint32_t, 1024>& bitmap) const {
    uint64_t airMask = m_blockPaletteIndices[BlockTypes::Air];

    // Expand to all slots.
    airMask |= airMask << 8;
    airMask |= airMask << 16;
    airMask |= airMask << 32;

    const uint64_t* data = reinterpret_cast<const uint64_t*>(m_blockData.data());
    uint64_t* bitmap64 = reinterpret_cast<uint64_t*>(bitmap.data());

    for (int i = 0; i < 512; i++) {
        uint64_t trueBits = 0u;
        for (int n = 0; n < 8; n++) {
            // Find bits that differ from the air mask.
            uint64_t bits = data[(i << 3) + n] ^ airMask;
            
            // Ensure least significant bit represents truthiness of individual bytes.
            bits |= bits >> 1;
            bits |= bits >> 2;
            bits |= bits >> 4;

            // Mask out all but LSBs.
            bits &= 0x0101010101010101ULL;

            // Isolate LSB bits into least signficiant byte.
            bits = (bits * 0x8040201008040201ULL) >> 56;

            // Shift byte to correct position in trueBits.
            bits <<= ((7-n) * 8);

            // Move byte into trueBits.
            trueBits |= bits; // Shift into correct byte location.
        }

        // Set the binary truth values into the bitmap.
        bitmap64[i] = trueBits;
    }
}
