#include "Parser.hpp"
#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/tools.hpp"
#include "glm/gtc/type_ptr.hpp"

void Parser::Init()
{
    constexpr auto extensions = fastgltf::Extensions::KHR_materials_transmission |
                                fastgltf::Extensions::KHR_materials_volume;
    mParser = fastgltf::Parser(extensions);
}

std::optional<Model> Parser::LoadModelFromFile(const std::string_view filename)
{
    auto expectedMappedFile = fastgltf::MappedGltfFile::FromPath(filename);
    if (!expectedMappedFile)
    {
        std::cerr << "Failed to load model from file " << filename << std::endl;
        return std::nullopt;
    }

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
                                 fastgltf::Options::LoadExternalBuffers |
                                 fastgltf::Options::GenerateMeshIndices;

    const auto directory = std::filesystem::path(filename).parent_path();
    auto expectedAsset = mParser.loadGltf(expectedMappedFile.get(), directory, gltfOptions);
    if (!expectedAsset)
    {
        std::cerr << "Failed to load model from file " << filename << std::endl;
        std::cerr << fastgltf::getErrorMessage(expectedAsset.error()) << std::endl;
        return std::nullopt;
    }
    const auto asset = std::move(expectedAsset.get());

    Model model;
    LoadMeshes(asset, model);
    LoadMaterials(asset, model);

    return model;
}

void Parser::LoadMaterials(
    const fastgltf::Asset& asset,
    Model& model)
{
    for (const auto& gltfMaterial : asset.materials)
    {
        Material material;
        auto& pbrData = gltfMaterial.pbrData;
        if (pbrData.baseColorTexture)
        {
            material.baseColorTexture = pbrData.baseColorTexture.value().textureIndex;
        }
        material.baseColorFactor = glm::make_vec4(pbrData.baseColorFactor.data());

        if (pbrData.metallicRoughnessTexture)
        {
            material.metallicRoughnessTexture =
                pbrData.metallicRoughnessTexture.value().textureIndex;
        }
        material.metallicFactor = pbrData.metallicFactor;
        material.roughnessFactor = pbrData.roughnessFactor;

        if (gltfMaterial.normalTexture)
        {
            material.normalTexture = gltfMaterial.normalTexture.value().textureIndex;
        }

        if (gltfMaterial.occlusionTexture)
        {
            material.occlusionTexture = gltfMaterial.occlusionTexture.value().textureIndex;
            material.ao = gltfMaterial.occlusionTexture.value().strength;
        }
        
        material.ior = gltfMaterial.ior;
        material.alphaCutoff = gltfMaterial.alphaCutoff;

        if (gltfMaterial.emissiveTexture)
        {
            material.emissiveTexture = gltfMaterial.emissiveTexture.value().textureIndex;
        }
        material.emissiveFactor = glm::make_vec3(gltfMaterial.emissiveFactor.data());

        model.materials.emplace_back(material);
    }
}

void Parser::LoadMeshes(
    const fastgltf::Asset& asset,
    Model& model)
{
    constexpr auto parent = glm::mat4(1.f);
    for (const auto& nodeIndex : asset.scenes[0].nodeIndices)
    {
        const auto node = asset.nodes[nodeIndex];
        TraverseNode(node, asset, parent, model);
    }
}

void Parser::LoadImageURIs(
    const fastgltf::Asset& asset,
    Model& model)
{
    for (const auto& [data, name] : asset.images)
    {
        assert(std::holds_alternative<fastgltf::sources::URI>(data));
        auto uri = std::get<fastgltf::sources::URI>(data);
        model.uris.emplace_back(uri.uri.string());
    }
}

void Parser::TraverseNode(
    const fastgltf::Node& node,
    const fastgltf::Asset& asset,
    const glm::mat4& parentTransform,
    Model& model)
{
    const auto fastTransform = fastgltf::getTransformMatrix(node);
    const auto currentTransform = parentTransform * glm::make_mat4(fastTransform.data());
    if (node.meshIndex)
    {
        const auto& mesh = asset.meshes[node.meshIndex.value()];
        model.transforms.emplace_back(currentTransform);

        for (const auto& primitive : mesh.primitives)
        {
            const auto oldIndicesSize = static_cast<u32>(model.indices.size());

            auto& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
            model.indices.resize(oldIndicesSize + indexAccessor.count);
            switch (indexAccessor.componentType)
            {
            case fastgltf::ComponentType::UnsignedShort:
            {
                std::vector<u16> shortIndices(indexAccessor.count);
                fastgltf::copyFromAccessor<u16>(asset, indexAccessor, shortIndices.data());
                std::ranges::copy(shortIndices, model.indices.begin() + oldIndicesSize);
                break;
            }
            case fastgltf::ComponentType::UnsignedInt:
            {
                fastgltf::copyFromAccessor<u32>(
                    asset,
                    indexAccessor,
                    model.indices.data() + oldIndicesSize);
                break;
            }
            default:
                break;
            }

            const auto oldVerticesSize = static_cast<u32>(model.vertices.size());
            const auto materialID =
                primitive.materialIndex ? static_cast<int>(primitive.materialIndex.value()) : -1;
            model.meshes.emplace_back(
                oldVerticesSize,
                oldIndicesSize,
                static_cast<u32>(model.indices.size() - oldIndicesSize),
                materialID,
                static_cast<u32>(model.transforms.size() - 1));

            assert(primitive.findAttribute("POSITION"));
            auto& positionAccessor =
                asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];

            model.vertices.resize(oldVerticesSize + positionAccessor.count);
            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset,
                positionAccessor,
                [&](const glm::vec3& position, const size_t index)
                {
                    model.vertices[oldVerticesSize + index].position = position;
                });

            assert(primitive.findAttribute("NORMAL")->accessorIndex);
            auto& normalAccessor =
                asset.accessors[primitive.findAttribute("NORMAL")->accessorIndex];
            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                asset,
                normalAccessor,
                [&](const glm::vec3& normal, const size_t index)
                {
                    model.vertices[oldVerticesSize + index].normal = normal;
                });

            assert(primitive.findAttribute("TEXCOORD_0"));
            auto& uvAccessor =
                asset.accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];
            fastgltf::iterateAccessorWithIndex<glm::vec2>(
                asset,
                uvAccessor,
                [&](const glm::vec2& uv, const size_t index)
                {
                    model.vertices[oldVerticesSize + index].uvX = uv.x;
                    model.vertices[oldVerticesSize + index].uvY = uv.y;
                });
        }
    }
    for (const auto& childIndex : node.children)
    {
        auto child = asset.nodes[childIndex];
        TraverseNode(child, asset, currentTransform, model);
    }
}