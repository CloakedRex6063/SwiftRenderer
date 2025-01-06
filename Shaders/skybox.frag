#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(binding = 4) uniform samplerCube cubemaps[];

layout(push_constant) uniform Constant
{
    int cubemapIndex;
    mat4 viewProj;
};

void main()
{
    FragColor = texture(cubemaps[cubemapIndex], TexCoords);
}