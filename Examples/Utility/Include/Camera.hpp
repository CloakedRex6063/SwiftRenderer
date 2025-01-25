#pragma once

struct CameraData;
namespace Camera
{
    CameraData Init(glm::vec3 pos);
    void HandleKeyboard(CameraData& camera,
                        float deltaTime,
                        float sensitivity);
    void Update(CameraData& camera,
                float fov,
                glm::vec2 size,
                float nearClip,
                float farClip);
    void HandleMouse(const CameraData& camera,
                    float deltaTime,
                    float sensitivity);
}  // namespace Util::Camera
