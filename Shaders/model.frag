#version 460

#include "common.glsl"

layout (location = 0) out vec4 outFragColor;
layout(location = 0) in mat4 transform;

layout(push_constant) uniform Constant
{
    VertexBuffer vertexBuffer;
    CameraBuffer cameraBuffer;
    TransformBuffer transformBuffer;
    MaterialBuffer materialBuffer;
    int transformIndex;
    int materialIndex;
} pushConstant;


void main()
{
    Material material = pushConstant.materialBuffer.materials[pushConstant.materialIndex];
    outFragColor = vec4(material.baseColorFactor, 1.0);
}