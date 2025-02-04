#include "Parser.hpp"
#include "Structs.hpp"
#include "SwiftUtil.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/tools.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace
{
    fastgltf::Parser gParser;

    void LoadMaterials(
        Scene& scene,
        const fastgltf::Asset& asset)
    {
        for (const auto& gltfMaterial : asset.materials)
        {
            Material material;
            auto& pbrData = gltfMaterial.pbrData;
            if (pbrData.baseColorTexture)
            {
                auto& imageIndex =
                    asset.textures[pbrData.baseColorTexture.value().textureIndex].imageIndex;
                material.baseTextureIndex = static_cast<int>(imageIndex.value());
            }
            material.baseColorFactor = glm::make_vec4(pbrData.baseColorFactor.data());

            if (pbrData.metallicRoughnessTexture)
            {
                auto& imageIndex =
                    asset.textures[pbrData.metallicRoughnessTexture.value().textureIndex]
                        .imageIndex;
                material.metallicRoughnessTextureIndex = static_cast<int>(imageIndex.value());
            }
            material.metallicFactor = pbrData.metallicFactor;
            material.roughnessFactor = pbrData.roughnessFactor;

            if (gltfMaterial.normalTexture)
            {
                auto& imageIndex =
                    asset.textures[gltfMaterial.normalTexture.value().textureIndex].imageIndex;
                material.normalTextureIndex = static_cast<int>(imageIndex.value());
            }

            if (gltfMaterial.occlusionTexture)
            {
                auto& imageIndex =
                    asset.textures[gltfMaterial.occlusionTexture.value().textureIndex].imageIndex;
                material.occlusionTextureIndex = static_cast<int>(imageIndex.value());
                material.ao = gltfMaterial.occlusionTexture.value().strength;
            }

            material.ior = gltfMaterial.ior;
            material.alphaCutoff = gltfMaterial.alphaCutoff;

            if (gltfMaterial.emissiveTexture)
            {
                auto& imageIndex =
                    asset.textures[gltfMaterial.emissiveTexture.value().textureIndex].imageIndex;
                material.emissiveTextureIndex = static_cast<int>(imageIndex.value());
            }
            material.emissiveFactor = glm::make_vec3(gltfMaterial.emissiveFactor.data());

            scene.materials.emplace_back(material);
        }
    };

    void TraverseNode(
        Scene& scene,
        std::vector<int>& meshes,
        const fastgltf::Node& node,
        const fastgltf::Asset& asset,
        const glm::mat4& parentTransform)
    {
        const auto fastTransform = fastgltf::getTransformMatrix(node);
        const auto currentTransform = parentTransform * glm::make_mat4(fastTransform.data());
        for (const auto& childIndex : node.children)
        {
            auto child = asset.nodes[childIndex];
            TraverseNode(scene, meshes, child, asset, currentTransform);
        }
        if (node.meshIndex)
        {
            const auto& mesh = asset.meshes[node.meshIndex.value()];
            scene.transforms.emplace_back(currentTransform);

            for (const auto& primitive : mesh.primitives)
            {
                const auto oldIndicesSize = static_cast<u32>(scene.indices.size());

                auto& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
                scene.indices.resize(oldIndicesSize + indexAccessor.count);
                switch (indexAccessor.componentType)
                {
                case fastgltf::ComponentType::UnsignedShort:
                {
                    std::vector<u16> shortIndices(indexAccessor.count);
                    fastgltf::copyFromAccessor<u16>(asset, indexAccessor, shortIndices.data());
                    std::ranges::copy(shortIndices, scene.indices.begin() + oldIndicesSize);
                    break;
                }
                case fastgltf::ComponentType::UnsignedInt:
                {
                    fastgltf::copyFromAccessor<u32>(
                        asset,
                        indexAccessor,
                        scene.indices.data() + oldIndicesSize);
                    break;
                }
                default:
                    break;
                }

                const auto oldVerticesSize = static_cast<u32>(scene.vertices.size());
                const auto materialSize = static_cast<u32>(scene.materials.size());
                const auto transformIndex = static_cast<u32>(scene.transforms.size() - 1);
                const auto materialID = primitive.materialIndex
                                            ? static_cast<int>(primitive.materialIndex.value())
                                            : -1;
                scene.meshes.emplace_back(
                    oldVerticesSize,
                    oldIndicesSize,
                    static_cast<u32>(indexAccessor.count),
                    materialSize + materialID,
                    transformIndex);
                meshes.emplace_back(static_cast<int>(scene.meshes.size() - 1));

                assert(primitive.findAttribute("POSITION"));
                auto& positionAccessor =
                    asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];

                std::vector<Vertex> vertices(positionAccessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    asset,
                    positionAccessor,
                    [&](const glm::vec3& position, const size_t index)
                    {
                        vertices[index].position = position;
                    });

                auto positions = vertices | std::views::transform(&Vertex::position);
                auto subrange = std::ranges::to<std::vector>(positions);
                auto boundingSphere = Swift::Visibility::CreateBoundingSphereFromVertices(subrange);
                scene.boundingSpheres.emplace_back(boundingSphere);

                assert(primitive.findAttribute("NORMAL")->accessorIndex);
                auto& normalAccessor =
                    asset.accessors[primitive.findAttribute("NORMAL")->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    asset,
                    normalAccessor,
                    [&](const glm::vec3& normal, const size_t index)
                    {
                        vertices[index].normal = normal;
                    });

                assert(primitive.findAttribute("TEXCOORD_0"));
                auto& uvAccessor =
                    asset.accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    asset,
                    uvAccessor,
                    [&](const glm::vec2& uv, const size_t index)
                    {
                        vertices[index].uvX = uv.x;
                        vertices[index].uvY = uv.y;
                    });
                scene.vertices.insert_range(scene.vertices.end(), vertices);
            }
        }
    }

    std::vector<int> LoadMeshData(
        Scene& scene,
        const fastgltf::Asset& asset)
    {
        std::vector<int> meshes;
        constexpr auto parent = glm::mat4(1.f);
        for (const auto& nodeIndex : asset.scenes[0].nodeIndices)
        {
            const auto node = asset.nodes[nodeIndex];
            TraverseNode(scene, meshes, node, asset, parent);
        }
        return meshes;
    }

    void LoadImageURIs(
        Scene& scene,
        const fastgltf::Asset& asset,
        const std::string_view basePath)
    {
        for (const auto& [data, name] : asset.images)
        {
            assert(std::holds_alternative<fastgltf::sources::URI>(data));
            auto uri = std::get<fastgltf::sources::URI>(data);
            scene.uris.emplace_back(std::string(basePath) + "/" + std::string(uri.uri.string()));
        }
    }
} // namespace

namespace Parser
{
    void Parser::Init()
    {
        constexpr auto extensions = fastgltf::Extensions::KHR_materials_transmission |
                                    fastgltf::Extensions::KHR_materials_volume |
                                    fastgltf::Extensions::KHR_materials_specular |
                                    fastgltf::Extensions::KHR_materials_emissive_strength |
                                    fastgltf::Extensions::KHR_materials_ior |
                                    fastgltf::Extensions::KHR_texture_transform |
                                    fastgltf::Extensions::KHR_materials_unlit;
        gParser = fastgltf::Parser(extensions);
    }

    std::expected<
        std::vector<int>,
        ParserError>
    Parser::LoadMeshes(
        Scene& scene,
        std::filesystem::path filePath)
    {
        auto expectedMappedFile = fastgltf::MappedGltfFile::FromPath(filePath);
        if (!expectedMappedFile)
        {
            std::cerr << "Failed to load meshes from file " << filePath << std::endl;
            return std::unexpected(ParserError::eFileNotFound);
        }

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
                                     fastgltf::Options::LoadExternalBuffers |
                                     fastgltf::Options::GenerateMeshIndices;

        const auto directory = std::filesystem::path(filePath).parent_path();
        auto expectedAsset = gParser.loadGltf(expectedMappedFile.get(), directory, gltfOptions);
        if (!expectedAsset)
        {
            std::cerr << "Failed to load meshes from file " << filePath << std::endl;
            std::cerr << fastgltf::getErrorMessage(expectedAsset.error()) << std::endl;
            return std::unexpected(ParserError::eInvalidFile);
        }
        const auto asset = std::move(expectedAsset.get());
        auto meshes = LoadMeshData(scene, asset);
        LoadMaterials(scene, asset);
        LoadImageURIs(scene, asset, directory.string());
        return meshes;
    }
} // namespace Parser
