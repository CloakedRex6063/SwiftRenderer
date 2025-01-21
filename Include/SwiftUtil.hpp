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
            float nearPlane,
            float farPlane,
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

    namespace Compression
    {
        enum class Error
        {
            eNone,
            eFileNotFound,
            eCompressionFailed,
            eNotAFolder,
        };

        [[nodiscard]]
        std::expected<
            int,
            Error> CompressFile(const std::filesystem::path& filePath);
        
        [[nodiscard]]
        std::expected<
            std::vector<int>,
            Error>
        BatchCompress(
            const std::filesystem::path& folderPath,
            bool recursive);
    } // namespace Compression

    namespace Performance
    {
        void BeginTimer();
        float EndTimer();
    }

    template<typename T> std::string_view GetErrorMessage(T error)
    {
        if constexpr (std::is_same_v<T, Compression::Error>)
        {
            switch (static_cast<Compression::Error>(error))
            {
            case Compression::Error::eNone:
                return "";
                break;
            case Compression::Error::eFileNotFound:
                return "File not found at given location or corrupt";
                break;
            case Compression::Error::eCompressionFailed:
                return "Compression failed";
                break;
            case Compression::Error::eNotAFolder:
                return "Given destination was not a folder.";
                break;
            }
        }
        return "";
    }
} // namespace Swift::Util
