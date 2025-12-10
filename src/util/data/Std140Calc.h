#pragma once

#include <cstdint>

// Used for calculating the size of an Std140 buffer.
class Std140Calc final {
public:
    VXL_INLINE Std140Calc() = default;

    VXL_INLINE Std140Calc& PutFloat() noexcept {
        m_size += sizeof(float);

        return *this;
    }

    VXL_INLINE Std140Calc& PutInt() noexcept {
        m_size += sizeof(int);

        return *this;
    }

    VXL_INLINE Std140Calc& PutUInt() noexcept {
        m_size += sizeof(unsigned int);

        return *this;
    }

    VXL_INLINE Std140Calc& PutVec2() noexcept {
        m_size += sizeof(float) * 2;

        return *this;
    }

    VXL_INLINE Std140Calc& PutVec3() noexcept {
        m_size += sizeof(float) * 3;

        return *this;
    }

    VXL_INLINE Std140Calc& PutVec4() noexcept {
        m_size += sizeof(float) * 4;

        return *this;
    }

    VXL_INLINE Std140Calc& PutMat2() noexcept {
        m_size += sizeof(float) * (2 * 2);

        return *this;
    }

    VXL_INLINE Std140Calc& PutMat3() noexcept {
        m_size += sizeof(float) * (3 * 3);

        return *this;
    }

    VXL_INLINE Std140Calc& PutMat4() noexcept {
        m_size += sizeof(float) * (4 * 4);

        return *this;
    }

    VXL_INLINE const uint32_t Get() const noexcept {
        return m_size;
    }
private:
    uint32_t m_size = 0;
};
