#pragma once

#include <cstddef>

// Various different types.
enum eRenderType {
    RENDER_TYPE_VEC2,
    RENDER_TYPE_VEC3,
    RENDER_TYPE_VEC4,
    RENDER_TYPE_MAT2,
    RENDER_TYPE_MAT3,
    RENDER_TYPE_MAT4,
    RENDER_TYPE_FLOAT,
    RENDER_TYPE_DOUBLE,
    RENDER_TYPE_INT,
    RENDER_TYPE_UINT
};

const size_t GetRenderTypeCount(eRenderType type);

const size_t GetRenderTypeSize(eRenderType type);
