#pragma once

#include "util/display/pipeline/Type.h"
#include <array>
#include <cstddef>

// Simple class for handling different vertex formats.
class VertexFormat final {
public:
    struct ElementData {
        DataType type;
        size_t size = 0;
    };

    VXL_INLINE VertexFormat() = default;

    // Adds an element.
    VXL_INLINE VertexFormat& Element(DataType type) {
        if (m_elementsSize < 10) {
            m_elements[m_elementsSize] = {type, GetDataTypeCount(type)};
            m_elementsSize++;
            m_stride += GetDataTypeSize(type);
        }

        return *this;
    }

    VXL_INLINE const size_t GetStride() const noexcept {
        return m_stride;
    }

    VXL_INLINE const size_t GetElementsSize() const noexcept {
        return m_elementsSize;
    }

    VXL_INLINE const std::array<ElementData, 10> GetElements() const noexcept {
        return m_elements;
    }
private:
    std::array<ElementData, 10> m_elements;
    size_t m_elementsSize = 0;
    size_t m_stride = 0;
};
