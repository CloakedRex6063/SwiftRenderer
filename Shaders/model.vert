#version 460

#include "common.glsl"

layout(push_constant) uniform Constant
{
    VertexBuffer vertexBuffer;
    CameraBuffer cameraBuffer;
    TransformBuffer transformBuffer;
    MaterialBuffer materialBuffer;
    int transformIndex;
} pushConstant;

layout(location = 0) out mat4 outTransform;

void main() 
{
    Vertex vertex = pushConstant.vertexBuffer.vertices[gl_VertexIndex];
    mat4 transform = pushConstant.transformBuffer.transforms[pushConstant.transformIndex];
    CameraBuffer cameraBuffer = pushConstant.cameraBuffer;
    gl_Position = cameraBuffer.proj * cameraBuffer.view * transform * vec4(vertex.position, 1);
    outTransform = transform;
}