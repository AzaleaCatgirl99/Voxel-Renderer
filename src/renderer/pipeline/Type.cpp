#include "renderer/pipeline/Type.h"

const size_t GetRenderTypeCount(eRenderType type) {
    switch (type) {
    case RENDER_TYPE_VEC2:
        return 2;
    case RENDER_TYPE_VEC3:
        return 3;
    case RENDER_TYPE_VEC4:
        return 4;
    case RENDER_TYPE_MAT2:
        return 2 * 2;
    case RENDER_TYPE_MAT3:
        return 3 * 3;
    case RENDER_TYPE_MAT4:
        return 4 * 4;
    default:
        return 1;
    }
}

const size_t GetRenderTypeSize(eRenderType type) {
    switch (type) {
    case RENDER_TYPE_VEC2:
    case RENDER_TYPE_VEC3:
    case RENDER_TYPE_VEC4:
    case RENDER_TYPE_MAT2:
    case RENDER_TYPE_MAT3:
    case RENDER_TYPE_MAT4:
        return sizeof(float) * GetRenderTypeCount(type);
    case RENDER_TYPE_FLOAT:
        return sizeof(float);
    case RENDER_TYPE_DOUBLE:
        return sizeof(double);
    case RENDER_TYPE_INT:
        return sizeof(int);
    case RENDER_TYPE_UINT:
        return sizeof(unsigned int);
    }
}
