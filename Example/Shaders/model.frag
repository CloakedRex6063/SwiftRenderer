#version 460
#include "common.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform sampler2D samplers[];

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
    outFragColor = texture(samplers[0], inUV);
}