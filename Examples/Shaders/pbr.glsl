// From https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/1.2.lighting_textured/1.2.pbr.fs
#include "common.glsl"

const int PointLight = 0;
const int DirectionalLight = 1;

vec3 GetNormalFromMap(sampler2D normalMap, vec3 normal, vec2 uv, vec3 worldPos)
{
    vec3 tangentNormal = texture(normalMap, uv).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(worldPos);
    vec3 Q2  = dFdy(worldPos);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);

    vec3 N   = normalize(normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NoH = max(dot(N, H), 0.0);
    float NoH2 = NoH * NoH;

    float nom = a2;
    float denom = (NoH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NoV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float nom = NoV;
    float denom = NoV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NoV, roughness);
    float ggx2 = GeometrySchlickGGX(NoL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


vec3 CalculateDiffuseIBL(vec3 normal, vec3 albedo, samplerCube irradianceCube) {
    vec3 irradiance = texture(irradianceCube, -normal).rgb;
    return irradiance * albedo;
}

vec3 CalculateSpecularIBL(vec3 normal, vec3 viewDir, float roughness, vec3 F, samplerCube prefilter, sampler2D brdfLUT) {
    const float MAX_REFLECTION_LOD = 5.0;
    vec3 R = reflect(viewDir, normal);
    vec3 prefilteredColor = textureLod(prefilter, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(brdfLUT, vec2(clamp(dot(normal, viewDir), 0.0, 0.99), roughness)).rg;
    return prefilteredColor * (F * envBRDF.x + envBRDF.y);
}

vec3 CalculateBRDF(vec3 normal, vec3 view, vec3 lightDir, vec3 albedo, vec3 F0, float metallic, float roughness, vec3 lightColor)
{
    vec3 H = normalize(view + lightDir);
    vec3 F = FresnelSchlick(max(dot(H, view), 0.0), F0);
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, view, lightDir, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, view), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NoL = max(dot(normal, lightDir), 0.0);

    return (kD * albedo / PI + specular) * lightColor * NoL;
}

float CalculateAttenuation(vec3 lightPos, vec3 position, float range) {
    float distance = length(lightPos - position);
    return clamp(1.0 - (distance / range), 0.0, 1.0);
}