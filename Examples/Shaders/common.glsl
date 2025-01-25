#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require

const float PI = 3.14159265359;

struct Light
{
    vec3 position;
    float intensity;
    vec3 color;
    int type;
    vec3 direction;
    int padding;
};

struct Material
{
    vec4 baseColorFactor;

    int baseTextureIndex;
    int metallicRoughnessTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;

    vec3 emissiveFactor;
    float metallicFactor;

    float roughnessFactor;
    float ao;
    float alphaCutoff;
    float ior;

    int normalTextureIndex;
    int padding;
    int padding2;
    int padding3;
};

struct Vertex
{
    vec3 position;
    float uvX;
    vec3 normal;
    float uvY;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer { Vertex vertices[]; };
layout(buffer_reference, std430) readonly buffer CameraBuffer
{
    mat4 view;
    mat4 projection;
    vec3 position;
    float padding;
};

layout(buffer_reference, std430) readonly buffer TransformBuffer
{
    mat4 transforms[];
};

layout(buffer_reference, std430) readonly buffer MaterialBuffer
{
    Material materials[];
};

layout(buffer_reference, std430) readonly buffer LightBuffer
{
    Light lights[];
};

struct PerDrawData
{
    int materialIndex;
    int transformIndex;
    vec2 padding;
};

layout(buffer_reference, std430, buffer_reference_align = 8) readonly buffer PerDrawBuffer
{
    PerDrawData perDrawData[];
};
