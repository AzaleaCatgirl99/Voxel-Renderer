#include "util/display/vulkan/VkConversions.h"

// Various different functions used to convert one type to a Vulkan type.
namespace VkConversions {

uint32_t GetIndexTypeSize(vk::IndexType type) {
    switch (type) {
    case vk::IndexType::eUint16:
        return sizeof(uint16_t);
    case vk::IndexType::eUint32:
        return sizeof(uint32_t);
    case vk::IndexType::eNoneKHR:
        return 0;
    case vk::IndexType::eUint8KHR:
        return sizeof(uint8_t);
    }
}

vk::PresentModeKHR GetPresentMode(VkStructs::SwapInterval interval) {
    switch (interval) {
    case VkStructs::SwapInterval::eImmediate:
        return vk::PresentModeKHR::eImmediate;
    case VkStructs::SwapInterval::eVSync:
        return vk::PresentModeKHR::eFifo;
    case VkStructs::SwapInterval::eTripleBuffering:
        return vk::PresentModeKHR::eMailbox;
    }
}

vk::Format GetTypeFormat(DataType type) {
    switch (type) {
    case DataType::eVec2:
        return vk::Format::eR32G32Sfloat;
    case DataType::eVec3:
        return vk::Format::eR32G32B32Sfloat;
    case DataType::eVec4:
        return vk::Format::eR32G32B32A32Sfloat;
    case DataType::eMat2:
        return vk::Format::eUndefined; // TODO figure out this one
    case DataType::eMat3:
        return vk::Format::eUndefined; // TODO figure out this one
    case DataType::eMat4:
        return vk::Format::eUndefined; // TODO figure out this one
    case DataType::eFloat:
        return vk::Format::eR32Sfloat;
    case DataType::eDouble:
        return vk::Format::eR64Sfloat;
    case DataType::eInt:
        return vk::Format::eR32Sint;
    case DataType::eUint:
        return vk::Format::eR32Sint;
    case DataType::eInt16:
    case DataType::eUint16:
        return vk::Format::eUndefined;
    }
}

}
