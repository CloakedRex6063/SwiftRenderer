#version 460
#include "bindings.glsl"
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 WorldPos;
layout(location = 0) out vec4 FragColor;

layout(binding = SamplerBinding) uniform sampler2D samplers[];

layout(push_constant) uniform Constant
{
    int cubemapIndex;
    mat4 viewProj;
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
