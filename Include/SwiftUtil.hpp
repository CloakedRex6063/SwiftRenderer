#pragma once
#include "SwiftStructs.hpp"

namespace Swift
{
    struct BoundingSphere;
}

namespace Swift
{
    namespace Visibility
    {
        BoundingSphere CreateBoundingSphereFromVertices(std::span<glm::vec3> positions);
        BoundingAABB CreateBoundingAABBFromVertices(std::span<glm::vec3> positions);
        void UpdateFrustum(
            Frustum& frustum,
            glm::mat4& viewMatrix,
            glm::vec3 cameraPos,
            float nearClip,
            float farClip,
            float fov,
            float aspect);
        bool IsInFrustum(
            const Frustum& frustum,
            const BoundingSphere& sphere,
            const glm::mat4& worldTransform);

        inline glm::vec3 GetScaleFromMatrix(const glm::mat4& matrix)
        {
            return {
                glm::length(glm::vec3(matrix[0])),
                glm::length(glm::vec3(matrix[1])),
                glm::length(glm::vec3(matrix[2])),
            };
        }
    } // namespace Visibility
    
    namespace Performance
    {
        void BeginTimer();
        float EndTimer();
    }
} // namespace Swift
