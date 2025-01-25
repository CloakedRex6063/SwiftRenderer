#pragma once
#include "SwiftStructs.hpp"

struct ModelPushConstant
{
    u64 cameraBufferAddress{};
    u64 vertexBufferAddress{};
    u64 transformBufferAddress{};
    u64 materialBufferAddress{};
    u64 lightBufferAddress{};
    int transformOffset{};
    int transformIndex{};
    int materialIndex{};
    int lightCount{};
    int irradianceIndex{};
    int specularIndex{};
    int lutIndex{};
};

struct IndirectDrawPushConstant
{
    u64 perDrawBufferAddress{};
    u64 cameraBufferAddress{};
    u64 lightBufferAddress{};
    u64 vertexBufferAddress{};
    u64 transformBufferAddress{};
    u64 materialBufferAddress{};
    int lightCount{};
    int irradianceIndex{};
    int specularIndex{};
    int lutIndex{};
};

struct PerDrawData
{
    int materialIndex{};
    int transformIndex{};
    glm::vec2 padding{};
};

struct IndirectFillPushConstant
{
    u64 indirectBuffer = 0;
    u64 meshBuffer = 0;
    u32 meshCount = 0;
};

struct IndirectFillCullPushConstant
{
    u64 indirectBuffer = 0;
    u64 meshBuffer = 0;
    u64 frustumBuffer = 0;
    u64 boundingBuffer = 0;
    u64 transformBuffer = 0;
    u64 visBuffer = 0;
    u32 meshCount = 0;
};

struct StreamPushConstant
{
    u64 transformBuffer{};
    u64 meshBuffer{};
    u64 lodBuffer{};
    u64 visibilityBuffer{};
    glm::vec3 cameraPos{};
    u32 meshCount{};
    float minDistance{};
    float maxDistance{};
};

struct SkyboxPushConstant
{
    u64 vertexBuffer = 0;
    u64 cameraBuffer = 0;
    u64 cubemapIndex = 0;
};

struct Vertex
{
    glm::vec3 position{};
    float uvX{};
    glm::vec3 normal{};
    float uvY{};
};

struct Mesh
{
    int vertexOffset{};
    u32 firstIndex{};
    u32 indexCount{};
    int materialIndex{};
    int transformIndex{};
    int padding{};
};

struct Material
{
    glm::vec4 baseColorFactor{};
    int baseTextureIndex = -1;
    int metallicRoughnessTextureIndex = -1;
    int occlusionTextureIndex = -1;
    int emissiveTextureIndex = -1;
    
    glm::vec3 emissiveFactor = {};
    float metallicFactor = 0.f;
    
    float roughnessFactor = 0.f;
    float ao = 1.0f;
    float alphaCutoff = 0.5f;
    float ior = 1.5f;
    
    int normalTextureIndex = -1;
    int padding{};
    int padding2{};
    int padding3{};
};

struct Scene
{
    std::vector<Mesh> meshes{};
    std::vector<Swift::BoundingSphere> boundingSpheres{};
    std::vector<Material> materials{};
    std::vector<std::string> uris{};
    std::vector<Vertex> vertices{};
    std::vector<u32> indices{};
    std::vector<glm::mat4> transforms{};
    ModelPushConstant pushConstant{};
};

struct CameraData
{
    glm::mat4 view{};
    glm::mat4 proj{};
    glm::vec3 pos{};
    float padding{};
};

struct Light
{
    glm::vec3 position{};
    float intensity = 1000.0f;
    glm::vec3 color = glm::vec3(1, 1, 1);
    int type = 0;
    glm::vec3 direction = glm::vec3(0, 0, 1);
    int padding = 0;
};

struct Transform
{
    glm::vec3 position{};
    glm::vec3 rotation{};
    glm::vec3 scale{};
    glm::mat4 worldTransform{};
};