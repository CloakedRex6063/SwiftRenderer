#version 460
#extension GL_GOOGLE_include_directive : require 
#include "pbr.glsl"

#ifdef INDIRECT
layout(location = 0) in flat int inDrawID;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inWorldPos;
layout(location = 3) in vec3 inNormal;
#else
layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;
#endif

layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform sampler2D samplers[];
layout(binding = 4) uniform samplerCube cubemaps[];

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
} pushConstant;
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
} pushConstant;
#endif

void main()
{
#ifdef INDIRECT
    PerDrawData drawData = pushConstant.perDrawBuffer.perDrawData[inDrawID];
    if(drawData.materialIndex == -1) discard;
    Material material = pushConstant.materialBuffer.materials[drawData.materialIndex];
#else
    if(pushConstant.materialIndex == -1) discard;
    Material material = pushConstant.materialBuffer.materials[pushConstant.materialIndex];
#endif
    bool useBaseTexture = material.baseTextureIndex != -1;
    bool useEmissiveTexture = material.emissiveTextureIndex != -1;
    bool useOcclusionTexture = material.occlusionTextureIndex != -1;
    bool useRoughnessTexture = material.metallicRoughnessTextureIndex != -1;
    bool useNormalTexture = material.normalTextureIndex != -1;
    

    vec4 baseColor = material.baseColorFactor;
    if(useBaseTexture)
    {
        baseColor = pow(texture(samplers[material.baseTextureIndex], inUV), vec4(2.2));
    }

    vec4 emissive = vec4(0);
    if(useEmissiveTexture)
    {
        emissive = pow(texture(samplers[material.emissiveTextureIndex], inUV), vec4(2.2));
    }

    float occlusion = 1.f;
    float metal = material.metallicFactor;
    float rough = material.roughnessFactor;
    if(useRoughnessTexture)
    {
        vec4 orm = pow(texture(samplers[material.metallicRoughnessTextureIndex], inUV), vec4(2.2));
        occlusion = orm.r;
        rough = orm.g * material.roughnessFactor;
        metal = orm.b * material.metallicFactor;
    }

    if(useOcclusionTexture)
    {
        occlusion = pow(texture(samplers[material.occlusionTextureIndex], inUV), vec4(2.2)).r;
    }
    occlusion *= material.ao;

    vec3 N = inNormal;
    if(useNormalTexture)
    {
        N = GetNormalFromMap(samplers[material.normalTextureIndex], inNormal, inUV, inWorldPos);
    }

    vec3 camPos = pushConstant.cameraBuffer.position;

    vec3 Lo = vec3(0.0);
    vec3 V = normalize(camPos - inWorldPos);
    vec3 R = reflect(V, N);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor.rgb, metal);
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, rough);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metal;

    LightBuffer lightBuffer = pushConstant.lightBuffer;
    for (int i = 0; i < pushConstant.lightCount; i++) {
        Light light = lightBuffer.lights[i];
        vec3 L = normalize(light.position - inWorldPos);
        float attenuation = CalculateAttenuation(light.position, inWorldPos, 1.0);
        vec3 lightColor = light.color * attenuation;

        Lo += CalculateBRDF(N, V, L, baseColor.rgb, F0, metal, rough, lightColor);
    }
    
    vec3 diffuseIBL = CalculateDiffuseIBL(N, baseColor.rgb, cubemaps[pushConstant.irradianceIndex]);
    vec3 specularIBL = CalculateSpecularIBL(N, V, rough, F, cubemaps[pushConstant.specularIndex], samplers[pushConstant.lutIndex]);
    vec3 ambient = (kD * diffuseIBL + specularIBL * 0.5) * occlusion;
    
    vec3 color = ambient + Lo + emissive.rgb;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    if(baseColor.a < 0.1) discard;
    outFragColor = vec4(color, 1.0);
}