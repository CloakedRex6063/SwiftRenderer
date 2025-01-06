#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require

#define SamplerBinding = 0;
#define UniformBinding = 1;
#define StorageBinding = 2;
#define ImageBinding = 3;

struct Material
{
    int baseColorTexture;
    vec3 baseColorFactor;

    int metallicRoughnessFactor;
    float metallicFactor;
    float roughnessFactor;
    int normalTexture;

    int occlusionTexture;
    float ao;
    float ior;
    float alphacutoff;

    int emissiveTexture;
    vec3 emissiveFactor;
};

struct Vertex
{
    vec3 position;
    float uvX;
    vec3 normal;
    float uvY;
    vec3 tangent;
    float padding;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer { Vertex vertices[]; };
layout(buffer_reference, std430) readonly buffer CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec4 pos;
};
layout(buffer_reference, std430) readonly buffer TransformBuffer
{
    mat4 transforms[];
};

layout(buffer_reference) readonly buffer MaterialBuffer
{
    Material materials[];
};