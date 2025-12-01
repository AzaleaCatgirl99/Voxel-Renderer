#pragma once

#include "App.h"
#include <glm/glm.hpp>

// Camera utility.

class Camera final {
public:
    static void Update();
    static void TickEvents();

    static constexpr const glm::mat4 GetProj() noexcept {
        return sProjection;
    }

    static constexpr const glm::mat4 GetView() noexcept {
        return sView;
    }

    static constexpr const glm::vec3 GetTarget() noexcept {
        return sTarget;
    }

    static constexpr const glm::vec3 GetPos() noexcept {
        return sPosition;
    }

    static constexpr float GetFOV() noexcept {
        return sFOV;
    }

    static constexpr const float GetSpeed() noexcept {
        return sSpeed * App::DeltaTime();
    }

    static constexpr const float GetSensitivity() noexcept {
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
