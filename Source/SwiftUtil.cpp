#include "SwiftUtil.h"
#include "SwiftStructs.hpp"
#include "glm/gtx/norm.hpp"

#include <SwiftPCH.hpp>

namespace
{
    struct Plane
    {
        glm::vec3 normal;
        float distance;
        float GetSignedDistanceToPlane(glm::vec3 vec) const;
    };
    float Plane::GetSignedDistanceToPlane(glm::vec3 vec) const
    {
        return glm::dot(normal, vec) - distance;
    }

    struct Frustum
    {
        Plane topFace;
        Plane bottomFace;

        Plane leftFace;
        Plane rightFace;

        Plane nearFace;
        Plane farFace;
    } gFrustum;

    Plane CreatePlane(
        const glm::vec3& position,
        const glm::vec3& normal)
    {
        return {
            .normal = glm::normalize(normal),
            .distance = glm::dot(glm::normalize(normal), position),
        };
    };

    glm::vec3 GetScaleFromMatrix(const glm::mat4& matrix)
    {
        return {
            glm::length(glm::vec3(matrix[0])),
            glm::length(glm::vec3(matrix[1])),
            glm::length(glm::vec3(matrix[2])),
        };
    }

    bool IsInsidePlane(
        const Plane& plane,
        const Swift::BoundingSphere& sphere)
    {
        return glm::dot(plane.normal, sphere.center) - plane.distance > -sphere.radius;
    }
} // namespace

namespace Swift::Util
{
        std::optional<BoundingSphere>
        Visibility::CreateBoundingSphereFromVertices(const std::span<glm::vec3> positions)
        {
            auto minAABB = glm::vec3(std::numeric_limits<float>::max());
            auto maxAABB = glm::vec3(std::numeric_limits<float>::min());
            for (auto&& position : positions)
            {
                minAABB.x = std::min(minAABB.x, position.x);
                minAABB.y = std::min(minAABB.y, position.y);
                minAABB.z = std::min(minAABB.z, position.z);

                maxAABB.x = std::max(maxAABB.x, position.x);
                maxAABB.y = std::max(maxAABB.y, position.y);
                maxAABB.z = std::max(maxAABB.z, position.z);
            }

            return BoundingSphere((maxAABB + minAABB) * 0.5f, glm::length(minAABB - maxAABB));
        }

        void Visibility::UpdateFrustum(
            glm::mat4& viewMatrix,
            const glm::vec3 cameraPos,
            const float nearPlane,
            const float farPlane,
            const float fov,
            const float aspect)
        {
            const auto forward =
                glm::normalize(-glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]));
            const auto right =
                glm::normalize(glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]));
            const auto up =
                glm::normalize(glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]));
            const float halfVSide = farPlane * tanf(fov * .5f);
            const float halfHSide = halfVSide * aspect;
            const glm::vec3 frontMultFar = farPlane * forward;

            gFrustum.nearFace = CreatePlane(cameraPos + nearPlane * forward, forward);
            gFrustum.farFace = CreatePlane(cameraPos + frontMultFar, -forward);
            gFrustum.rightFace =
                CreatePlane(cameraPos, glm::cross(frontMultFar - right * halfHSide, up));
            gFrustum.leftFace =
                CreatePlane(cameraPos, glm::cross(up, frontMultFar + right * halfHSide));
            gFrustum.topFace =
                CreatePlane(cameraPos, glm::cross(right, frontMultFar - up * halfVSide));
            gFrustum.bottomFace =
                CreatePlane(cameraPos, glm::cross(frontMultFar + up * halfVSide, right));
        }

        bool Visibility::IsInFrustum(
            const BoundingSphere& sphere,
            const glm::mat4& worldTransform)
        {
            const auto globalScale = GetScaleFromMatrix(worldTransform);
            const auto globalCenter = worldTransform * glm::vec4(sphere.center, 1.0f);
            const float maxScale = std::max(std::max(globalScale.x, globalScale.y), globalScale.z);

            const BoundingSphere boundingSphere {globalCenter, sphere.radius * maxScale * 0.5f};

            return IsInsidePlane(gFrustum.leftFace, boundingSphere) &&
                   IsInsidePlane(gFrustum.rightFace, boundingSphere) &&
                   IsInsidePlane(gFrustum.topFace, boundingSphere) &&
                   IsInsidePlane(gFrustum.bottomFace, boundingSphere) &&
                   IsInsidePlane(gFrustum.nearFace, boundingSphere) &&
                   IsInsidePlane(gFrustum.farFace, boundingSphere);
        }
} // namespace Swift::Util