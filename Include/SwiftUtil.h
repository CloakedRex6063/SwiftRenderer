#pragma once

namespace Swift
{
    struct BoundingSphere;
}

namespace Swift::Util
{
    namespace Visibility
    {
        std::optional<BoundingSphere>
        CreateBoundingSphereFromVertices(std::span<glm::vec3> positions);
        void UpdateFrustum(
            glm::mat4& viewMatrix,
            glm::vec3 cameraPos,
            float nearPlane,
            float farPlane,
            float fov,
            float aspect);
        bool IsInFrustum(
            const BoundingSphere& sphere,
            const glm::mat4& worldTransform);
    } // namespace Visibility
} // namespace Swift::Util
