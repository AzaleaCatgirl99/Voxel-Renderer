#include "util/Morton.h"
#include <iomanip>

Logger Morton::sLogger = Logger("Morton");

const uint16_t Morton::sMortonEncode3DX[32] = {
    0x0000, 0x0004, 0x0020, 0x0024, 0x0100, 0x0104, 0x0120, 0x0124, 0x0800, 0x0804, 0x0820,
    0x0824, 0x0900, 0x0904, 0x0920, 0x0924, 0x4000, 0x4004, 0x4020, 0x4024, 0x4100, 0x4104,
    0x4120, 0x4124, 0x4800, 0x4804, 0x4820, 0x4824, 0x4900, 0x4904, 0x4920, 0x4924
};

const uint16_t Morton::sMortonEncode3DY[32] = {
    0x0000, 0x0002, 0x0010, 0x0012, 0x0080, 0x0082, 0x0090, 0x0092, 0x0400, 0x0402, 0x0410,
    0x0412, 0x0480, 0x0482, 0x0490, 0x0492, 0x2000, 0x2002, 0x2010, 0x2012, 0x2080, 0x2082,
    0x2090, 0x2092, 0x2400, 0x2402, 0x2410, 0x2412, 0x2480, 0x2482, 0x2490, 0x2492
};

const uint16_t Morton::sMortonEncode3DZ[32] = {
    0x0000, 0x0001, 0x0008, 0x0009, 0x0040, 0x0041, 0x0048, 0x0049, 0x0200, 0x0201, 0x0208,
    0x0209, 0x0240, 0x0241, 0x0248, 0x0249, 0x1000, 0x1001, 0x1008, 0x1009, 0x1040, 0x1041,
    0x1048, 0x1049, 0x1200, 0x1201, 0x1208, 0x1209, 0x1240, 0x1241, 0x1248, 0x1249
};

const uint16_t Morton::sMortonDecode3DLow[32] = {
    0x0000, 0x0001, 0x0020, 0x0021, 0x0400, 0x0401, 0x0420, 0x0421, 0x0002, 0x0003, 0x0022,
    0x0023, 0x0402, 0x0403, 0x0422, 0x0423, 0x0040, 0x0041, 0x0060, 0x0061, 0x0440, 0x0441,
    0x0460, 0x0461, 0x0042, 0x0043, 0x0062, 0x0063, 0x0442, 0x0443, 0x0462, 0x0463
};

const uint16_t Morton::sMortonDecode3DMid[32] = {
    0x0000, 0x0800, 0x0004, 0x0804, 0x0080, 0x0880, 0x0084, 0x0884, 0x1000, 0x1800, 0x1004,
    0x1804, 0x1080, 0x1880, 0x1084, 0x1884, 0x0008, 0x0808, 0x000c, 0x080c, 0x0088, 0x0888,
    0x008c, 0x088c, 0x1008, 0x1808, 0x100c, 0x180c, 0x1088, 0x1888, 0x108c, 0x188c
};

const uint16_t Morton::sMortonDecode3DHigh[32] = {
    0x0000, 0x0100, 0x2000, 0x2100, 0x0010, 0x0110, 0x2010, 0x2110, 0x0200, 0x0300, 0x2200,
    0x2300, 0x0210, 0x0310, 0x2210, 0x2310, 0x4000, 0x4100, 0x6000, 0x6100, 0x4010, 0x4110,
    0x6010, 0x6110, 0x4200, 0x4300, 0x6200, 0x6300, 0x4210, 0x4310, 0x6210, 0x6310
};

uint16_t Morton::sFullMortonDecode3D[32768] = {};
uint16_t Morton::sFullMortonEncode3D[32768] = {};

void Morton::LogMortons() {
    std::stringstream xstream;
    sLogger.Verbose("Printing Morton Codes for X:");
    for (uint16_t x = 0; x < 32; x++) {
        uint16_t code = 0;
        code |= (((x >> 0) & 1u) << 2);
        code |= (((x >> 1) & 1u) << 5);
        code |= (((x >> 2) & 1u) << 8);
        code |= (((x >> 3) & 1u) << 11);
        code |= (((x >> 4) & 1u) << 14);
        xstream << "0x" << std::hex << std::setfill('0') << std::setw(4) << code << ", ";

        // sLogger.Verbose(x, ": ", std::bitset<16>(code));
    }
    sLogger.Verbose(xstream.str());

    std::stringstream ystream;
    sLogger.Verbose("Printing Morton Codes for Y:");
    for (uint16_t y = 0; y < 32; y++) {
        uint16_t code = 0;
        code |= (((y >> 0) & 1u) << 1);
        code |= (((y >> 1) & 1u) << 4);
        code |= (((y >> 2) & 1u) << 7);
        code |= (((y >> 3) & 1u) << 10);
        code |= (((y >> 4) & 1u) << 13);
        ystream << "0x" << std::hex << std::setfill('0') << std::setw(4) << code << ", ";

        // sLogger.Verbose(y, ": ", std::bitset<16>(code));
    }
    sLogger.Verbose(ystream.str());

    std::stringstream zstream;
    sLogger.Verbose("Printing Morton Codes for Z:");
    for (uint16_t z = 0; z < 32; z++) {
        uint16_t code = 0;
        code |= (((z >> 0) & 1u) << 0);
        code |= (((z >> 1) & 1u) << 3);
        code |= (((z >> 2) & 1u) << 6);
        code |= (((z >> 3) & 1u) << 9);
        code |= (((z >> 4) & 1u) << 12);
        zstream << "0x" << std::hex << std::setfill('0') << std::setw(4) << code << ", ";

        // sLogger.Verbose(z, ": ", std::bitset<16>(code));
    }
    sLogger.Verbose(zstream.str());

    std::stringstream decodeLowStream;
    std::stringstream decodeMidStream;
    std::stringstream decodeHighStream;
    for (uint16_t n = 0; n < 32; n++) {
        uint16_t low = Decode3DMorton(n);
        uint16_t mid = Decode3DMorton(n << 5);
        uint16_t high = Decode3DMorton(n << 10);
        decodeLowStream << "0x" << std::hex << std::setfill('0') << std::setw(4) << low << ", ";
        decodeMidStream << "0x" << std::hex << std::setfill('0') << std::setw(4) << mid << ", ";
        decodeHighStream << "0x" << std::hex << std::setfill('0') << std::setw(4) << high << ", ";
    }
    sLogger.Verbose("Low 5 bit decode values: ", decodeLowStream.str());
    sLogger.Verbose("Mid 5 bit decode values: ", decodeMidStream.str());
    sLogger.Verbose("High 5 bit decode values: ", decodeHighStream.str());
}

void Morton::InitializeLookupTables() {
    for (uint8_t x = 0; x < 32; x++) {
        for (uint8_t y = 0; y < 32; y++) {
            for (uint8_t z = 0; z < 32; z++) {
                uint16_t packed = (x << 10) | (y << 5) | z;
                uint16_t code = Encode3DMortonManual(x, y, z);

                sFullMortonDecode3D[code] = packed;
                sFullMortonEncode3D[packed] = code;
            }
        }
    }
}

bool Morton::Test3DEncodingDecoding() {
    for (int x = 0; x < 32; x++) {
        for (int y = 0; y < 32; y++) {
            for (int z = 0; z < 32; z++) {
                uint16_t actual = (x << 10) | (y << 5) | z;
                uint16_t encoded = Encode3DMorton(x, y, z);
                uint16_t decoded = Decode3DMorton(encoded);

                if (decoded != actual) {
                    sLogger.Verbose("Actual: ", std::bitset<16>(actual), " (x: ", x, ", y: ", y, ", z: ", z, ")");
                    sLogger.Verbose("Encoded: ", std::bitset<16>(encoded));
                    sLogger.Verbose("Decoded: ", std::bitset<16>(decoded));
                    return false;
                }
            }
        }
    }

    return true;
}

// void Morton::SearchMagic() {
//     for (uint16_t n = 0; n < 512; n++) {
//         uint16_t m = (~n & 0b111111111);

//         if (((n & 0b110000000) == 0b110000000) && ((n & 0b000101000) != 0)) continue;
//         if (((m & 0b110000000) == 0b110000000) && ((m & 0b000101000) != 0)) continue;

//         if (((n & 0b101000000) == 0b101000000) && ((n & 0b000100100) != 0)) continue;
//         if (((m & 0b101000000) == 0b101000000) && ((m & 0b000100100) != 0)) continue;

//         if (((n & 0b100100000) == 0b100100000) && ((n & 0b011000000) != 0)) continue;
//         if (((m & 0b100100000) == 0b100100000) && ((m & 0b011000000) != 0)) continue;

//         if (((n & 0b100010000) == 0b100010000) && ((n & 0b000001100) != 0)) continue;
//         if (((m & 0b100010000) == 0b100010000) && ((m & 0b000001100) != 0)) continue;

//         if (((n & 0b100001000) == 0b100001000) && ((n & 0b000010001) != 0)) continue;
//         if (((m & 0b100001000) == 0b100001000) && ((m & 0b000010001) != 0)) continue;

//         if (((n & 0b100000100) == 0b100000100) && ((n & 0b001010000) != 0)) continue;
//         if (((m & 0b100000100) == 0b100000100) && ((m & 0b001010000) != 0)) continue;

//         if (((n & 0b011000000) == 0b011000000) && ((n & 0b000010100) != 0)) continue;
//         if (((m & 0b011000000) == 0b011000000) && ((m & 0b000010100) != 0)) continue;

//         if (((n & 0b010100000) == 0b010100000) && ((n & 0b000010010) != 0)) continue;
//         if (((m & 0b010100000) == 0b010100000) && ((m & 0b000010010) != 0)) continue;

//         if (((n & 0b010010000) == 0b010010000) && ((n & 0b001100000) != 0)) continue;
//         if (((m & 0b010010000) == 0b010010000) && ((m & 0b001100000) != 0)) continue;

//         if (((n & 0b010001000) == 0b010001000) && ((n & 0b100000100) != 0)) continue;
//         if (((m & 0b010001000) == 0b010001000) && ((m & 0b100000100) != 0)) continue;

//         if (((n & 0b010000100) == 0b010000100) && ((n & 0b000001000) != 0)) continue;
//         if (((m & 0b010000100) == 0b010000100) && ((m & 0b000001000) != 0)) continue;

//         if (((n & 0b001100000) == 0b001100000) && ((n & 0b100001010) != 0)) continue;
//         if (((m & 0b001100000) == 0b001100000) && ((m & 0b100001010) != 0)) continue;

//         if (((n & 0b001010000) == 0b001010000) && ((n & 0b010001001) != 0)) continue;
//         if (((m & 0b001010000) == 0b001010000) && ((m & 0b010001001) != 0)) continue;

//         if (((n & 0b001001000) == 0b001001000) && ((n & 0b000110000) != 0)) continue;
//         if (((m & 0b001001000) == 0b001001000) && ((m & 0b000110000) != 0)) continue;

//         if (((n & 0b001000100) == 0b001000100) && ((n & 0b010000010) != 0)) continue;
//         if (((m & 0b001000100) == 0b001000100) && ((m & 0b010000010) != 0)) continue;

//         if (((n & 0b001000010) == 0b001000010) && ((n & 0b000100000) != 0)) continue;
//         if (((m & 0b001000010) == 0b001000010) && ((m & 0b000100000) != 0)) continue;

//         if (((n & 0b001000001) == 0b001000001) && ((n & 0b000010100) != 0)) continue;
//         if (((m & 0b001000001) == 0b001000001) && ((m & 0b000010100) != 0)) continue;

//         if (((n & 0b000110000) == 0b000110000) && ((n & 0b010000101) != 0)) continue;
//         if (((m & 0b000110000) == 0b000110000) && ((m & 0b010000101) != 0)) continue;

//         if (((n & 0b000101000) == 0b000101000) && ((n & 0b001000100) != 0)) continue;
//         if (((m & 0b000101000) == 0b000101000) && ((m & 0b001000100) != 0)) continue;

//         if (((n & 0b000100100) == 0b000100100) && ((n & 0b000011000) != 0)) continue;
//         if (((m & 0b000100100) == 0b000100100) && ((m & 0b000011000) != 0)) continue;

//         if (((n & 0b000100010) == 0b000100010) && ((n & 0b001000001) != 0)) continue;
//         if (((m & 0b000100010) == 0b000100010) && ((m & 0b001000001) != 0)) continue;

//         if (((n & 0b000100001) == 0b000100001) && ((n & 0b100010000) != 0)) continue;
//         if (((m & 0b000100001) == 0b000100001) && ((m & 0b100010000) != 0)) continue;

//         if (((n & 0b000011000) == 0b000011000) && ((n & 0b101000010) != 0)) continue;
//         if (((m & 0b000011000) == 0b000011000) && ((m & 0b101000010) != 0)) continue;

//         if (((n & 0b000010100) == 0b000010100) && ((n & 0b100100010) != 0)) continue;
//         if (((m & 0b000010100) == 0b000010100) && ((m & 0b100100010) != 0)) continue;

//         if (((n & 0b000010010) == 0b000010010) && ((n & 0b000001100) != 0)) continue;
//         if (((m & 0b000010010) == 0b000010010) && ((m & 0b000001100) != 0)) continue;

//         if (((n & 0b000010001) == 0b000010001) && ((n & 0b001100000) != 0)) continue;
//         if (((m & 0b000010001) == 0b000010001) && ((m & 0b001100000) != 0)) continue;

//         if (((n & 0b000001100) == 0b000001100) && ((n & 0b010100001) != 0)) continue;
//         if (((m & 0b000001100) == 0b000001100) && ((m & 0b010100001) != 0)) continue;

//         if (((n & 0b000001010) == 0b000001010) && ((n & 0b010010000) != 0)) continue;
//         if (((m & 0b000001010) == 0b000001010) && ((m & 0b010010000) != 0)) continue;

//         if (((n & 0b000001001) == 0b000001001) && ((n & 0b000000110) != 0)) continue;
//         if (((m & 0b000001001) == 0b000001001) && ((m & 0b000000110) != 0)) continue;

//         if (((n & 0b000000110) == 0b000000110) && ((n & 0b001010000) != 0)) continue;
//         if (((m & 0b000000110) == 0b000000110) && ((m & 0b001010000) != 0)) continue;

//         if (((n & 0b000000101) == 0b000000101) && ((n & 0b001001000) != 0)) continue;
//         if (((m & 0b000000101) == 0b000000101) && ((m & 0b001001000) != 0)) continue;

//         if (((n & 0b000000011) == 0b000000011) && ((n & 0b000101000) != 0)) continue;
//         if (((m & 0b000000011) == 0b000000011) && ((m & 0b000101000) != 0)) continue;

//         sLogger.Info("Found a candidate!! -> ", std::bitset<9>(n));
//     }
//     sLogger.Info("Searched all possibilities.");
// }

// uint16_t Morton::Encode3DMorton(uint8_t x, uint8_t y, uint8_t z) {
//     return xMortonEncode3D[x] | yMortonEncode3D[y] | zMortonEncode3D[z];
// }

// uint16_t Morton::Decode3DMorton(uint16_t mortonCode) {
//     // 0100100100100100 -> 
//     // 0111110000000000
//     // const uint16_t x = 
//     return
//         ((mortonCode << 8) & 0b0'00001'0000000000u) |
//         ((mortonCode << 6) & 0b0'00010'0000000000u) |
//         ((mortonCode << 4) & 0b0'00100'0000000000u) |
//         ((mortonCode << 2) & 0b0'01000'0000000000u) |
//         ((mortonCode << 0) & 0b0'10000'0000000000u) |

//     // 0010010010010010 -> 
//     // 0000001111100000
//     // const uint16_t y = 
//         ((mortonCode << 4) & 0b000000'00001'00000u) |
//         ((mortonCode << 2) & 0b000000'00010'00000u) |
//         ((mortonCode << 0) & 0b000000'00100'00000u) |
//         ((mortonCode >> 2) & 0b000000'01000'00000u) |
//         ((mortonCode >> 4) & 0b000000'10000'00000u) |

//     // 0001001001001001 -> 
//     // 0000000000011111
//     // const uint16_t z = 
//         ((mortonCode << 0) & 0b00000000000'00001u) |
//         ((mortonCode >> 2) & 0b00000000000'00010u) |
//         ((mortonCode >> 4) & 0b00000000000'00100u) |
//         ((mortonCode >> 6) & 0b00000000000'01000u) |
//         ((mortonCode >> 8) & 0b00000000000'10000u);

//     // return x | y | z;
// }
