#version 460
#extension GL_GOOGLE_include_directive : require 
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require
#include "bindings.glsl"

layout (location = 0) in vec3 WorldPos;
layout(location = 0) out vec4 FragColor;

layout(binding = SamplerBinding) uniform sampler2D samplers[];

layout(buffer_reference, std430) readonly buffer ViewBuffer
{
    mat4 views[];
};

layout(push_constant) uniform Constant
{
    mat4 proj;
    ViewBuffer viewBuffer;
    int cubemapIndex;
} pushConstant;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(samplers[pushConstant.cubemapIndex], uv).rgb;

    FragColor = vec4(color, 1.0);
}
