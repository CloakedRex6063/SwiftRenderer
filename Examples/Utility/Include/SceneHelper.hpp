#pragma once
#include "Structs.hpp"
#include "SwiftStructs.hpp"

struct Scene;

enum class SceneError
{
    eNoError,
    eFileNotFound,
    eInvalidFile,
    eNoData,
};

namespace SceneHelper
{
    [[nodiscard]]
    std::expected<
        void,
        SceneError>
    AddToScene(
        Scene& scene,
        std::string_view filePath);
    [[nodiscard]]
    std::expected<
        void,
        SceneError>
    CreateSceneBuffers(
        Scene& scene,
        Swift::BufferHandle cameraBuffer,
        Swift::BufferHandle lightBuffer,
        Swift::ImageHandle irradiance,
        Swift::ImageHandle specular,
        Swift::ImageHandle lut);
    void DestroyScene(Scene& scene);

    [[nodiscard]]
    std::string_view GetErrorMessage(SceneError error);
} // namespace SceneHelper