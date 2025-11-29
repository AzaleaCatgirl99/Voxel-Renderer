#pragma once

#include "util/display/pipeline/Type.h"
#include <array>
#include <cstddef>

// Simple class for handling different vertex formats.
class VertexFormat final {
public:
    constexpr VertexFormat() = default;

    // Adds an element.
    constexpr VertexFormat& Element(eRenderType type) {
        if (m_elementsSize < 10) {
            m_elements[m_elementsSize] = {type, GetRenderTypeCount(type)};
            m_elementsSize++;
            m_stride += GetRenderTypeSize(type);
        }

        return *this;
    }
private:
    friend class GraphicsPipeline;
    friend class VertexBuffer;

    struct ElementData {
        eRenderType m_type;
        size_t m_size = 0;
    };

    std::array<ElementData, 10> m_elements;
    size_t m_elementsSize = 0;
    size_t m_stride = 0;
};
