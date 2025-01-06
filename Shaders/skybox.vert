#version 460

vec3 skyboxPositions[] = {
vec3(-1.0f,  1.0f, -1.0f),
vec3(-1.0f, -1.0f, -1.0f),
vec3( 1.0f, -1.0f, -1.0f),
vec3( 1.0f, -1.0f, -1.0f),
vec3( 1.0f,  1.0f, -1.0f),
vec3(-1.0f,  1.0f, -1.0f),

vec3(-1.0f, -1.0f,  1.0f),
vec3(-1.0f, -1.0f, -1.0f),
vec3(-1.0f,  1.0f, -1.0f),
vec3(-1.0f,  1.0f, -1.0f),
vec3(-1.0f,  1.0f,  1.0f),
vec3(-1.0f, -1.0f,  1.0f),

vec3( 1.0f, -1.0f, -1.0f),
vec3( 1.0f, -1.0f,  1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3( 1.0f,  1.0f, -1.0f),
vec3( 1.0f, -1.0f, -1.0f),

vec3(-1.0f, -1.0f,  1.0f),
vec3(-1.0f,  1.0f,  1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3( 1.0f, -1.0f,  1.0f),
vec3(-1.0f, -1.0f,  1.0f),

vec3(-1.0f,  1.0f, -1.0f),
vec3( 1.0f,  1.0f, -1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3( 1.0f,  1.0f,  1.0f),
vec3(-1.0f,  1.0f,  1.0f),
vec3(-1.0f,  1.0f, -1.0f),

vec3(-1.0f, -1.0f, -1.0f),
vec3(-1.0f, -1.0f,  1.0f),
vec3( 1.0f, -1.0f, -1.0f),
vec3( 1.0f, -1.0f, -1.0f),
vec3(-1.0f, -1.0f,  1.0f),
vec3( 1.0f, -1.0f,  1.0f)
};

layout(location = 0) out vec3 TexCoords;

layout(push_constant) uniform Constant
{
    int cubemapIndex;
    mat4 viewProj;
};

void main()
{
    TexCoords = skyboxPositions[gl_VertexIndex];
    TexCoords.xy *= -1.0;
    vec4 pos = viewProj * vec4(TexCoords, 1.0);
    gl_Position = pos.xyww;

}  