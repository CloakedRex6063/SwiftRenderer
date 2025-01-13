#pragma once
#include "SwiftStructs.hpp"

namespace Swift
{
    struct BoundingSphere;
}

namespace Swift::Util
{
    namespace Visibility
    {
        BoundingSphere CreateBoundingSphereFromVertices(std::span<glm::vec3> positions);
        BoundingAABB CreateBoundingAABBFromVertices(std::span<glm::vec3> positions);
        void UpdateFrustum(
            Frustum& frustum,
            glm::mat4& viewMatrix,
            glm::vec3 cameraPos,
            float nearPlane,
            float farPlane,
            float fov,
            float aspect);
        bool IsInFrustum(
            const Frustum& frustum,
            const BoundingSphere& sphere,
            const glm::mat4& worldTransform);
    } // namespace Visibility
} // namespace Swift::Util
