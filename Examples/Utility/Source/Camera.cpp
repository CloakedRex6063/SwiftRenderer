#include "Camera.hpp"

#include "Input.hpp"
#include "Structs.hpp"
#include "glm/gtx/euler_angles.hpp"

namespace
{
    float gYaw;
    float gPitch;
    glm::vec2 gPrevMousePos;
    glm::mat4 gRotationMatrix;
}  // namespace

CameraData Camera::Init(const glm::vec3 pos)
{
    CameraData camera{
            .proj = glm::perspective(glm::radians(60.f), 1280.f / 720.f, 0.01f, 1000.f),
            .pos = pos,
    };
    camera.view = glm::inverse(glm::translate(glm::mat4(1.0f), camera.pos));
    camera.proj[1][1] *= -1;
    return camera;
}

void Camera::HandleKeyboard(CameraData& camera,
                                  const float deltaTime,
                                  const float sensitivity)
{
    const auto& view = camera.view;
    const auto forward = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
    const auto right = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));

    float frontBack = 0;
    float sideways = 0;
    if (Input::GetKeyboardKey(Input::KeyboardKey::W))
    {
        frontBack += deltaTime * sensitivity;
    }
    if (Input::GetKeyboardKey(Input::KeyboardKey::S))
    {
        frontBack -= deltaTime * sensitivity;
    }
    if (Input::GetKeyboardKey(Input::KeyboardKey::A))
    {
        sideways -= deltaTime * sensitivity;
    }
    if (Input::GetKeyboardKey(Input::KeyboardKey::D))
    {
        sideways += deltaTime * sensitivity;
    }
    camera.pos += forward * frontBack + right * sideways;
    
    if (Input::GetKeyboardKey(Input::KeyboardKey::LeftControl))
    {
        camera.pos.y -= deltaTime * sensitivity;
    }
    if (Input::GetKeyboardKey(Input::KeyboardKey::Space))
    {
        camera.pos.y += deltaTime * sensitivity;
    }
}

void Camera::Update(CameraData& camera,
                          const float fov,
                          const glm::vec2 size,
                          const float nearClip,
                          const float farClip)
{
    camera.view = glm::inverse(glm::translate(glm::mat4(1.0f), camera.pos) * gRotationMatrix);
    camera.proj = glm::perspective(glm::radians(fov), size.x / size.y, nearClip, farClip);
    camera.proj[1][1] *= -1;
}

void Camera::HandleMouse(const CameraData& camera,
                               const float deltaTime,
                               const float sensitivity)
{
    if (Input::GetMouseButton(Input::MouseButton::Right))
    {
        const auto mousePosition = Input::GetMousePosition();
        const auto delta = mousePosition - gPrevMousePos;
        gYaw -= delta.x * sensitivity * deltaTime;
        gPitch -= delta.y * sensitivity * deltaTime;
        gPitch = glm::clamp(gPitch, -89.f, 89.f);
    }
    gRotationMatrix = glm::yawPitchRoll(gYaw, gPitch, 0.f);
    gPrevMousePos = Input::GetMousePosition();
}
