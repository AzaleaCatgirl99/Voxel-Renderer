#include "util/display/pipeline/Type.h"
#include <cstdint>

const size_t GetDataTypeCount(DataType type) {
    switch (type) {
    case DataType::eVec2:
        return 2;
    case DataType::eVec3:
        return 3;
    case DataType::eVec4:
        return 4;
    case DataType::eMat2:
        return 2 * 2;
    case DataType::eMat3:
        return 3 * 3;
    case DataType::eMat4:
        return 4 * 4;
    default:
        return 1;
    }
}

const size_t GetDataTypeSize(DataType type) {
    switch (type) {
    case DataType::eVec2:
    case DataType::eVec3:
    case DataType::eVec4:
    case DataType::eMat2:
    case DataType::eMat3:
    case DataType::eMat4:
        return sizeof(float) * GetDataTypeCount(type);
    case DataType::eFloat:
        return sizeof(float);
    case DataType::eDouble:
        return sizeof(double);
    case DataType::eInt:
        return sizeof(int);
    case DataType::eUint:
        return sizeof(unsigned int);
    case DataType::eInt16:
        return sizeof(int16_t);
    case DataType::eUint16:
        return sizeof(uint16_t);
    }
}
