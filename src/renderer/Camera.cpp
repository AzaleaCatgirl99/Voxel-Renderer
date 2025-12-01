#include "renderer/Camera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include "util/display/Window.h"

glm::mat4 Camera::sProjection;
glm::mat4 Camera::sView;
glm::vec3 Camera::sPosition = {0.0f, 0.0f, 0.0f};
glm::vec3 Camera::sTarget = {0.0f, 0.0f, 0.0f};
glm::vec3 Camera::sUp = {0.0f, 1.0f, 0.0f};
bool Camera::sMouseGrabbed = true;
glm::vec2 Camera::sLastPos;
float Camera::sYaw = 0.0f;
float Camera::sPitch = 0.0f;
float Camera::sSpeed = 0.03f;
float Camera::sFOV = 60.0f;
float Camera::sSensitivity = 0.01f;

void Camera::Update() {
    Window::SetMouseGrabbed(sMouseGrabbed);

    if (sSpeed < 0.0f)
        sSpeed = 0.0f;

    if (sSensitivity < 0.0f)
        sSensitivity = 0.0f;

    if (sFOV < 30.0f)
        sFOV = 30.0f;

    if (sFOV > 120.0f)
        sFOV = 120.0f;

    sProjection = glm::perspective(glm::radians(sFOV), Window::ApsectRatio(), 0.1f, 10000.0f);
    sProjection[1][1] *= -1;

    if (sMouseGrabbed) {
        sLastPos = Window::MouseRelativePos();
        glm::vec2 mousePos = Window::MouseRelativePos();

        float x = sLastPos.x - mousePos.x;
        float y = mousePos.y - sLastPos.y;

        x *= GetSensitivity();
        y *= GetSensitivity();

        sYaw += x;
        sPitch += y;

        if (sPitch > 89.0f)
            sPitch = 89.0f;

        if (sPitch < -89.0f)
            sPitch = -89.0f;

        Window::SetMousePosition(Window::Width() / 2.0f, Window::Height() / 2.0f);

        glm::vec3 direction;
        direction.x = std::cos(glm::radians(sYaw)) * std::cos(glm::radians(sPitch));
        direction.y = std::sin(glm::radians(sPitch));
        direction.z = std::sin(glm::radians(sYaw)) * std::cos(glm::radians(sPitch));
        sTarget = glm::normalize(direction);

        const bool* keyStates = SDL_GetKeyboardState(nullptr);

        if (keyStates[SDL_SCANCODE_W]) {
            sPosition.x += std::cos(glm::radians(sYaw)) * GetSpeed();
            sPosition.z += std::sin(glm::radians(sYaw)) * GetSpeed();
        }

        if (keyStates[SDL_SCANCODE_S]) {
            sPosition.x -= std::cos(glm::radians(sYaw)) * GetSpeed();
            sPosition.z -= std::sin(glm::radians(sYaw)) * GetSpeed();
        }

        if (keyStates[SDL_SCANCODE_A]) {
            sPosition.x -= std::cos(glm::radians(sYaw + 90)) * GetSpeed();
            sPosition.z -= std::sin(glm::radians(sYaw + 90)) * GetSpeed();
        }

        if (keyStates[SDL_SCANCODE_D]) {
            sPosition.x += std::cos(glm::radians(sYaw + 90)) * GetSpeed();
            sPosition.z += std::sin(glm::radians(sYaw + 90)) * GetSpeed();
        }

        if (keyStates[SDL_SCANCODE_SPACE])
            sPosition.y += GetSpeed();

        if (keyStates[SDL_SCANCODE_LSHIFT])
            sPosition.y -= GetSpeed();

        sView = glm::lookAt(sPosition, sPosition + sTarget, sUp);
        // sView[1][1] *= -1;
    }
}

void Camera::TickEvents() {
    if (Window::GetEvent()->type == SDL_EVENT_KEY_DOWN) {
        if (Window::GetEvent()->key.key == SDLK_ESCAPE)
            sMouseGrabbed = !sMouseGrabbed;

        if (Window::GetEvent()->key.key == SDLK_RIGHT)
            sSpeed += 0.01f;

        if (Window::GetEvent()->key.key == SDLK_LEFT)
            sSpeed -= 0.01f;

        if (Window::GetEvent()->key.key == SDLK_UP)
            sFOV += 5.0f;

        if (Window::GetEvent()->key.key == SDLK_DOWN)
            sFOV -= 5.0f;

        if (Window::GetEvent()->key.key == SDLK_PERIOD)
            sSensitivity += 0.01f;

        if (Window::GetEvent()->key.key == SDLK_COMMA)
            sSensitivity -= 0.01f;
    }
}
