#version 460
#extension GL_GOOGLE_include_directive : require 
#include "common.glsl"

layout(location = 0) out vec3 TexCoords;

layout(push_constant) uniform SkyboxConstant
{
    VertexBuffer vertexBuffer;
    CameraBuffer cameraBuffer;
    int cubemapIndex;
    mat4 viewProj;
};

void main()
{
    TexCoords = vertexBuffer.vertices[gl_VertexIndex].position;
    vec4 pos = cameraBuffer.projection * mat4(mat3(cameraBuffer.view)) * vec4(TexCoords, 1.0);
    gl_Position = pos.xyww;
}  