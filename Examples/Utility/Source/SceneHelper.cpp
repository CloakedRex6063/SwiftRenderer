#include "SceneHelper.hpp"
#include "Parser.hpp"
#include "Swift.hpp"

namespace
{
    void LoadTextures(Scene& scene)
    {
        Swift::BeginTransfer();

        std::unordered_map<u32, int> textureIndices;
        for (const auto& [index, uri] : std::views::enumerate(scene.uris))
        {
            const auto image = Swift::LoadImageFromFileQueued(uri, 0, true, uri);
            textureIndices[index] = static_cast<int>(Swift::GetImageArrayIndex(image));
        }

        // Update the material texture indices so that we can index into the texture in the shader
        for (auto& material : scene.materials)
        {
            if (material.baseTextureIndex != -1)
            {
                material.baseTextureIndex = textureIndices.at(material.baseTextureIndex);
            }
            if (material.metallicRoughnessTextureIndex != -1)
            {
                material.metallicRoughnessTextureIndex =
                    textureIndices.at(material.metallicRoughnessTextureIndex);
            }
            if (material.emissiveTextureIndex != -1)
            {
                material.emissiveTextureIndex = textureIndices.at(material.emissiveTextureIndex);
            }
            if (material.normalTextureIndex != -1)
            {
                material.normalTextureIndex = textureIndices.at(material.normalTextureIndex);
            }
            if (material.occlusionTextureIndex != -1)
            {
                material.occlusionTextureIndex = textureIndices.at(material.occlusionTextureIndex);
            }
        }

        Swift::EndTransfer();
    }
} // namespace

std::expected<
    void,
    SceneError>
SceneHelper::AddToScene(
    Scene& scene,
    const std::string_view filePath)
{
    const auto meshExpected = Parser::LoadMeshes(scene, filePath);
    if (!meshExpected.has_value())
    {
        switch (meshExpected.error())
        {
        case ParserError::eNoError:
            break;
        case ParserError::eFileNotFound:
            return std::unexpected(SceneError::eFileNotFound);
        case ParserError::eInvalidFile:
            return std::unexpected(SceneError::eInvalidFile);
        }
    }
    return {};
}
std::expected<
    void,
    SceneError>
SceneHelper::CreateSceneBuffers(
    Scene& scene,
    const Swift::BufferHandle cameraBuffer,
    const Swift::BufferHandle lightBuffer,
    const Swift::ImageHandle irradiance,
    const Swift::ImageHandle specular,
    const Swift::ImageHandle lut)
{
    SceneBuffers sceneBuffers;

    if (scene.vertices.empty() || scene.indices.empty() || scene.transforms.empty() ||
        scene.boundingSpheres.empty())
    {
        return std::unexpected(SceneError::eNoData);
    }

    if (Swift::IsValid(sceneBuffers.vertexBuffer))
    {
        Swift::DestroyBuffer(scene.sceneBuffers.vertexBuffer);
        Swift::DestroyBuffer(scene.sceneBuffers.indexBuffer);
        Swift::DestroyBuffer(scene.sceneBuffers.transformBuffer);
        Swift::DestroyBuffer(scene.sceneBuffers.boundingBuffer);
        Swift::DestroyBuffer(scene.sceneBuffers.materialBuffer);
    }

    const auto vertexSize = scene.vertices.size() * sizeof(Vertex);
    const auto vertexBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, vertexSize, "Vertex Buffer");
    Swift::UploadToBuffer(vertexBuffer, scene.vertices.data(), 0, vertexSize);
    sceneBuffers.vertexBuffer = vertexBuffer;

    const auto indexSize = scene.indices.size() * sizeof(u32);
    const auto indexBuffer =
        Swift::CreateBuffer(Swift::BufferType::eIndex, indexSize, "Index Buffer");
    Swift::UploadToBuffer(indexBuffer, scene.indices.data(), 0, indexSize);
    sceneBuffers.indexBuffer = indexBuffer;

    const auto transformSize = scene.transforms.size() * sizeof(glm::mat4);
    const auto transformBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, transformSize, "Transform Buffer");
    Swift::UploadToBuffer(transformBuffer, scene.transforms.data(), 0, transformSize);
    sceneBuffers.transformBuffer = transformBuffer;

    const auto boundingSize = scene.boundingSpheres.size() * sizeof(Swift::BoundingSphere);
    const auto boundingBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, boundingSize, "Bounding Sphere Buffer");
    Swift::UploadToBuffer(boundingBuffer, scene.boundingSpheres.data(), 0, boundingSize);
    sceneBuffers.boundingBuffer = boundingBuffer;

    LoadTextures(scene);

    const auto materialSize = scene.materials.size() * sizeof(Material);
    const auto materialBuffer =
        Swift::CreateBuffer(Swift::BufferType::eStorage, materialSize, "Material Buffer");
    Swift::UploadToBuffer(materialBuffer, scene.materials.data(), 0, materialSize);
    sceneBuffers.materialBuffer = materialBuffer;

    scene.sceneBuffers = sceneBuffers;

    scene.pushConstant = ModelPushConstant{
        .cameraBufferAddress = Swift::GetBufferAddress(cameraBuffer),
        .vertexBufferAddress = Swift::GetBufferAddress(vertexBuffer),
        .transformBufferAddress = Swift::GetBufferAddress(transformBuffer),
        .materialBufferAddress = Swift::GetBufferAddress(materialBuffer),
        .lightBufferAddress = Swift::GetBufferAddress(lightBuffer),
        .irradianceIndex = int(Swift::GetImageArrayIndex(irradiance)),
        .specularIndex = int(Swift::GetImageArrayIndex(specular)),
        .lutIndex = int(Swift::GetImageArrayIndex(lut)),
    };

    return {};
}

void SceneHelper::DestroyScene(Scene& scene)
{
    scene.vertices.clear();
    scene.indices.clear();
    scene.transforms.clear();
    scene.boundingSpheres.clear();
    scene.materials.clear();

    Swift::DestroyBuffer(scene.sceneBuffers.vertexBuffer);
    Swift::DestroyBuffer(scene.sceneBuffers.indexBuffer);
    Swift::DestroyBuffer(scene.sceneBuffers.transformBuffer);
    Swift::DestroyBuffer(scene.sceneBuffers.boundingBuffer);
    Swift::DestroyBuffer(scene.sceneBuffers.materialBuffer);
}

std::string_view SceneHelper::GetErrorMessage(const SceneError error)
{
    switch (error)
    {
    case SceneError::eNoError:
        return "No error";
    case SceneError::eFileNotFound:
        return "File not found. Possibly wrong location";
    case SceneError::eInvalidFile:
        return "Invalid File contents, file may be corrupt";
    case SceneError::eNoData:
        return "No data found!, add atleast one valid model before creating scene buffers";
    }
    return "";
}