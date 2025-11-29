#pragma once

#include "renderer/pipeline/Type.h"
#include <array>
#include <cstddef>

namespace detail {
class VkRenderer;
}

// Simple class for handling different vertex formats.
class VertexFormat final {
public:
    consteval VertexFormat() = default;

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
    friend detail::VkRenderer;

    struct ElementData {
        eRenderType m_type;
        size_t m_size = 0;
    };

    std::array<ElementData, 10> m_elements;
    size_t m_elementsSize = 0;
    size_t m_stride = 0;
};
