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
        void UpdateView(glm::mat4 view);
        bool TestVisibility(const BoundingSphere& sphere);
    } // namespace Visibility
} // namespace Swift::Util
