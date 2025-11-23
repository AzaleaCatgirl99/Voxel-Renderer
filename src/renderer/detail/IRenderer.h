#pragma once

namespace detail {

// Internal base virtual renderer.
class IRenderer {
public:
    virtual void Initialize() = 0;

    virtual void Destroy() = 0;
};

}
