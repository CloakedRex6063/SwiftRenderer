#version 460

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

layout(push_constant) uniform Constant
{
    int cubemapIndex;
    mat4 viewProj;
} pushConstant;

void main()
{
    WorldPos = positions[gl_VertexIndex];
    gl_Position =  pushConstant.viewProj * vec4(positions[gl_VertexIndex], 1.0);
}