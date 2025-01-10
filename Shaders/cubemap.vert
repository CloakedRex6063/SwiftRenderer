#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_multiview : require 

vec3 positions[] = {
// Back face
vec3(-1.0f, -1.0f, -1.0f),
vec3( 1.0f,  1.0f, -1.0f),
vec3( 1.0f, -1.0f, -1.0f),
vec3( 1.0f,  1.0f, -1.0f),
vec3(-1.0f, -1.0f, -1.0f),
vec3(-1.0f,  1.0f, -1.0f),

// Front face
vec3(-1.0f, -1.0f,  1.0f),
vec3( 1.0f, -1.0f,  1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3(-1.0f,  1.0f,  1.0f),
vec3(-1.0f, -1.0f,  1.0f),

// Left face
vec3(-1.0f,  1.0f,  1.0f),
vec3(-1.0f,  1.0f, -1.0f),
vec3(-1.0f, -1.0f, -1.0f),
vec3(-1.0f, -1.0f, -1.0f),
vec3(-1.0f, -1.0f,  1.0f),
vec3(-1.0f,  1.0f,  1.0f),

// Right face
vec3( 1.0f,  1.0f,  1.0f),
vec3( 1.0f, -1.0f, -1.0f),
vec3( 1.0f,  1.0f, -1.0f),
vec3( 1.0f, -1.0f, -1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3( 1.0f, -1.0f,  1.0f),

// Bottom face
vec3(-1.0f, -1.0f, -1.0f),
vec3( 1.0f, -1.0f, -1.0f),
vec3( 1.0f, -1.0f,  1.0f),
vec3( 1.0f, -1.0f,  1.0f),
vec3(-1.0f, -1.0f,  1.0f),
vec3(-1.0f, -1.0f, -1.0f),

// Top face
vec3(-1.0f,  1.0f, -1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3( 1.0f,  1.0f, -1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3(-1.0f,  1.0f, -1.0f),
vec3(-1.0f,  1.0f,  1.0f)
};

layout (location = 0) out vec3 WorldPos;

layout(buffer_reference, std430) readonly buffer ViewBuffer
{
    mat4 views[];
};

layout(push_constant) uniform Constant
{
    mat4 proj;
    ViewBuffer viewBuffer;
};

void main()
{
    WorldPos = positions[gl_VertexIndex];
    WorldPos.y = -WorldPos.y;
    gl_Position =  proj * viewBuffer.views[gl_ViewIndex] * vec4(positions[gl_VertexIndex], 1.0);
}