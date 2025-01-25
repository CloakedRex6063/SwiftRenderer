#version 460
#extension GL_GOOGLE_include_directive : require 
#include "common.glsl"

#ifdef INDIRECT
layout(location = 0) out int outDrawID;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outWorldPos;
layout(location = 3) out vec3 outNormal;
#else
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNormal;
#endif

#ifdef INDIRECT
layout(push_constant) uniform Constant
{
    PerDrawBuffer perDrawBuffer;
    CameraBuffer cameraBuffer;
    LightBuffer lightBuffer;
    VertexBuffer vertexBuffer;
    TransformBuffer transformBuffer;
    MaterialBuffer materialBuffer;
    int lightCount;
    int irradianceIndex;
    int specularIndex;
    int lutIndex;
};
#else
layout(push_constant) uniform Constant
{
    CameraBuffer cameraBuffer;
    VertexBuffer vertexBuffer;
    TransformBuffer transformBuffer;
    MaterialBuffer materialBuffer;
    LightBuffer lightBuffer;
    int transformOffset;
    int transformIndex;
    int materialIndex;
    int lightCount;
    int irradianceIndex;
    int specularIndex;
    int lutIndex;
};
#endif

void main() 
{
#ifdef INDIRECT
    outDrawID = gl_DrawID;
    PerDrawData drawData = perDrawBuffer.perDrawData[gl_DrawID];
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    mat4 model = transformBuffer.transforms[drawData.transformIndex];
#else
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    mat4 model = transformBuffer.transforms[transformIndex];
#endif
    
    gl_Position = cameraBuffer.projection * cameraBuffer.view * model * vec4(vertex.position, 1.0);
    outUV = vec2(vertex.uvX, vertex.uvY);
    outWorldPos = vec3(model * vec4(vertex.position, 1.0));
    outNormal = normalize(vec3(model * vec4(vertex.normal, 0.0)));
}