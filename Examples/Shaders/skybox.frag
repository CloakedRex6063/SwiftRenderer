#version 460
#extension GL_GOOGLE_include_directive : require 
#include "common.glsl"

layout(location = 0) in vec3 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(binding = 4) uniform samplerCube cubemaps[];

layout(push_constant) uniform SkyboxConstant
{
    VertexBuffer vertexBuffer;
    CameraBuffer cameraBuffer;
    int cubemapIndex;
    mat4 viewProj;
};

void main()
{
    vec3 envColor = texture(cubemaps[cubemapIndex], TexCoords).rgb;
    FragColor = vec4(envColor, 1.0);
}