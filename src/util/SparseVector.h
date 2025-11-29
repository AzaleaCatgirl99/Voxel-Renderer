#pragma once

#include <vector>
#include <queue>
#include <cstdint>

// Class for handling sparse vectors.
template<typename DataType, typename IndexType>
class SparseVector {
public:
    // Default constructor.
    SparseVector() = default;

    // Returns the value at the specified index.
    inline DataType& operator [](IndexType pIndex) {
        return m_data[pIndex];
    }

    // Ensure elements inserted are deleted later with Delete() before they are destroyed.
    inline IndexType Insert(DataType pValue) {
        // Double size if needed.
        IndexType newIndex;
        if (!m_unused.empty()) {
            newIndex = m_unused.front();
            m_data[newIndex] = pValue;
            m_unused.pop();
        } else {
            newIndex = m_data.size();
            m_data.push_back(pValue);
        }
        return newIndex;
    }

    // Deletes an element given its index.
    inline void Delete(IndexType pIndex) {
        // m_data[pIndex] = nullptr;
        m_unused.push(pIndex);
    }

    // Gets the number of active elements in the vector.
    inline IndexType Size() {
        return m_data.size() - m_unused.size();
    }

    // Reserves the underlying vector.
    inline void Reserve(const IndexType size) {
        m_data.reserve(size);
    }

    // Finds if the sparse vector is full.
    inline bool IsFull() const {
        return m_unused.empty() && m_data.capacity() == m_data.size();
    }

    // Returns a pointer to the start of the internal vector.
    inline DataType* Data() const {
        return m_data.data();
    }
private:
    std::vector<DataType> m_data;
    std::queue<IndexType> m_unused;
};
