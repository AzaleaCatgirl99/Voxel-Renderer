#pragma once

#include "App.h"
#include <glm/glm.hpp>

// Camera utility.
class Camera final {
public:
    static void Update();
    static void TickEvents();

    static VXL_INLINE const glm::mat4 GetProj() noexcept {
        return sProjection;
    }

    static VXL_INLINE const glm::mat4 GetView() noexcept {
        return sView;
    }

    static VXL_INLINE const glm::vec3 GetTarget() noexcept {
        return sTarget;
    }

    static VXL_INLINE const glm::vec3 GetPos() noexcept {
        return sPosition;
    }

    static VXL_INLINE float GetFOV() noexcept {
        return sFOV;
    }

    static VXL_INLINE const float GetSpeed() noexcept {
        return sSpeed * App::DeltaTime();
    }

    static VXL_INLINE const float GetSensitivity() noexcept {
        return sSensitivity * App::DeltaTime();
    }
private:
    static glm::mat4 sProjection;
    static glm::mat4 sView;
    static glm::vec3 sPosition;
    static glm::vec3 sTarget;
    static glm::vec3 sUp;
    static bool sMouseGrabbed;
    static glm::vec2 sLastPos;
    static float sYaw;
    static float sPitch;
    static float sSpeed;
    static float sFOV;
    static float sSensitivity;
};
