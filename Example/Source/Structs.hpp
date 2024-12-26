#pragma once

struct Vertex
{
    glm::vec3 position{};
    float uvX{};
    glm::vec3 normal{};
    float uvY{};
    glm::vec3 tangent{};
    float padding{};
};

struct Mesh
{
    u32 vertexOffset{};
    u32 firstIndex{};
    u32 indexCount{};
    int materialIndex{};
    u32 transformIndex{};
};

struct Material
{
    int baseColorTexture = -1;
    glm::vec3 baseColorFactor{};

    int metallicRoughnessTexture = -1;
    float metallicFactor{};
    float roughnessFactor{};
    int normalTexture = -1;
    
    int occlusionTexture = -1;
    float ao;
    float ior;
    float alphaCutoff;
    
    int emissiveTexture{};
    glm::vec3 emissiveFactor{};
};

struct Model
{
    std::vector<Mesh> meshes{};
    std::vector<Material> materials{};
    std::vector<std::string> uris{};
    std::vector<Vertex> vertices{};
    std::vector<u32> indices{};
    std::vector<glm::mat4> transforms{};
};