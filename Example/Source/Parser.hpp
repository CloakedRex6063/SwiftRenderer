#pragma once
#include "Structs.hpp"
#include "fastgltf/core.hpp"

class Parser
{
public:
    void Init();
    std::optional<Model> LoadModelFromFile(std::string_view filename);
    void Shutdown();

    fastgltf::Parser mParser;

private:
    static void LoadMaterials(
        const fastgltf::Asset& asset,
        Model& model);
    static void LoadMeshes(
        const fastgltf::Asset& asset,
        Model& model);
    static void TraverseNode(
        const fastgltf::Node& node,
        const fastgltf::Asset& asset,
        const glm::mat4& parentTransform,
        Model& model);
};
