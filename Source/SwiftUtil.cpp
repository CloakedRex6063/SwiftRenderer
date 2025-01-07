#include "SwiftUtil.h"
#include "SwiftStructs.hpp"
#include "glm/gtx/norm.hpp"

namespace
{
    std::array<glm::vec4, 6> gPlanes;
}

namespace Swift::Util
{
    namespace Visibility
    {
        std::optional<BoundingSphere>
        CreateBoundingSphereFromVertices(const std::span<glm::vec3> positions)
        {
            if (positions.empty())
            {
                return std::nullopt;
            }

            BoundingSphere sphere;
            sphere.center = {0, 0, 0};

            for (const auto& pt : positions)
            {
                sphere.center += pt;
            }
            sphere.center /= static_cast<float>(positions.size());

            float maxSquaredDistance = 0.0f;
            for (const auto& pt : positions)
            {
                maxSquaredDistance =
                    std::max(maxSquaredDistance, glm::distance2(pt, sphere.center));
            }

            sphere.radius = std::sqrt(maxSquaredDistance);

            return sphere;
        }
        void UpdateView(glm::mat4 view)
        {
            for (auto i = 0; i < 3; ++i)
            {
                for (size_t j = 0; j < 2; ++j)
                {
                    const float sign = j ? 1.f : -1.f;
                    for (auto k = 0; k < 4; ++k)
                    {
                        gPlanes[2 * i + j][k] = view[k][3] + sign * view[k][i];
                    }
                }
            }

            // normalize plane; see Appendix A.2
            for (auto&& plane : gPlanes)
            {
                plane /= glm::length(glm::vec3(plane));
            }
        }

        bool TestVisibility(const BoundingSphere& sphere)
        {
            std::array V{0, 1, 4, 5};
            return std::ranges::all_of(
                V,
                [&](const size_t i)
                {
                    const auto& plane = gPlanes[i];
                    return glm::dot(sphere.center, glm::vec3(plane)) + plane.w + sphere.radius >= 0;
                });
        }
    } // namespace Visibility
} // namespace Swift::Util