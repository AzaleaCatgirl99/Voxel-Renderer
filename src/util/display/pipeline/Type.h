#pragma once

#include <cstddef>

// Various different types.
enum class DataType {
    eVec2,
    eVec3,
    eVec4,
    eMat2,
    eMat3,
    eMat4,
    eFloat,
    eDouble,
    eInt,
    eUint,
    eInt16,
    eUint16
};

const size_t GetDataTypeCount(DataType type);

const size_t GetDataTypeSize(DataType type);
