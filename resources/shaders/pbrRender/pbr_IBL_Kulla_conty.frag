#version 450 core

#if defined(GL_ARB_bindless_texture)
#extension GL_ARB_bindless_texture : enable
#endif

out vec4 FragColor;
in VS_OUT
{
    vec3 vFragPos;
    vec3 vNormal;
    vec3 wFragPos;
    vec3 wNormal;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

// material parameters
#if defined(GL_ARB_bindless_texture)
layout(bindless_sampler) uniform sampler2D  u_albedoMap;
layout(bindless_sampler) uniform sampler2D  u_metallicMap;
layout(bindless_sampler) uniform sampler2D  u_roughnessMap;
layout(bindless_sampler) uniform sampler2D  u_normalMap;
layout(bindless_sampler) uniform samplerCube prefilterMap;
layout(bindless_sampler) uniform sampler2D brdfMap;
layout(bindless_sampler) uniform sampler2D brdfAvgMap;
#else
layout(binding=0) uniform sampler2D  u_albedoMap;
layout(binding=1) uniform sampler2D  u_metallicMap;
layout(binding=2) uniform sampler2D  u_roughnessMap;
layout(binding=3) uniform sampler2D  u_normalMap;
layout(binding=5) uniform samplerCube prefilterMap;
layout(binding=6) uniform sampler2D brdfMap;
#endif
// lights
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 camPos;
uniform float u_metallic_index;
uniform float u_roughness_index;
const float PI = 3.14159265359;
#define EPS 1e-6

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;// / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, EPS);// prevent divide by zero for roughness=0.0 and NdotH=1.0
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 AverageFresnel(vec3 r, vec3 g)
{
    return vec3(0.087237) + 0.0230685*g - 0.0864902*g*g + 0.0774594*g*g*g
    + 0.782654*r - 0.136432*r*r + 0.278708*r*r*r
    + 0.19744*g*r + 0.0360605*g*g*r - 0.2586*g*r*r;
}

vec3 MultiScatterBRDF(float NdotL, float NdotV,vec3 F0,vec3 albedo,float roughness)
{

    vec2 Eo = texture2D(brdfMap, vec2(NdotL, roughness)).xy;
    vec3 E_o = vec3(Eo.x) + vec3(Eo.y);
    vec2 Ei = texture2D(brdfMap, vec2(NdotV, roughness)).xy;
    vec3 E_i = vec3(Ei.x) + (Ei.y);

    vec3 E_avg = texture2D(brdfAvgMap, vec2(0, roughness)).xyz;
    vec3 F_avg = AverageFresnel(albedo, F0);

    vec3 fms = (( 1.0- E_o) * ( 1.0 - E_i)) / (PI * (1.0  - E_avg));
    vec3 fadd = F_avg * E_avg / ( 1.0 - F_avg*( 1.0 - E_avg));
    return fadd * fms;
}
void main()
{
    vec3 tex_normal = texture(u_normalMap, fs_in.TexCoords).rgb * 2.0 - 1.0;
    vec3 N = normalize(fs_in.TBN * tex_normal);//translate texnormal to world-space
    vec3 V = normalize(camPos - fs_in.wFragPos);
    vec3 R = reflect(-V, N);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    vec3 albedo  = pow(texture(u_albedoMap, fs_in.TexCoords).rgb, vec3(2.2));
    float metallic = texture(u_metallicMap, fs_in.TexCoords).r * u_metallic_index;
    float roughness = clamp(texture(u_roughnessMap, fs_in.TexCoords).r * u_roughness_index,0.0,1.0);
//    float roughness = u_roughness_index;
//     float ao = texture(u_aoMap, fs_in.TexCoords).r;
    float ao = 1.0;
    F0 = mix(F0, albedo, metallic);
    float NdotV = max(dot(N,V),0.0);

    vec3 Lo = vec3(0.0);
    //Direct Lighting
    {
        // reflectance equation
        vec3 L = normalize(lightPosition - fs_in.wFragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPosition - fs_in.wFragPos);
        //linear attenuation will be more realistic but less physically correct
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColor * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        vec3 nominator    = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 brdf= nominator / max(denominator, EPS);// prevent divide by zero for NdotV=0.0 or NdotL=0.0

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);
        vec3 Fms = MultiScatterBRDF(NdotL, NdotV,F0,albedo,roughness);

        // add to outgoing radiance Lo
        Lo +=  clamp((brdf + Fms),0.0,1.0)* radiance * NdotL;
    }

    //IBL
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfMap, vec2(NdotV, roughness)).rg;
    vec3 Fms = MultiScatterBRDF(1.0, NdotV,F0,albedo,roughness);
    vec3 specular = prefilteredColor * clamp((F * brdf.x + brdf.y + Fms),0.0,1.0);//schilick 被拆开的结果

    vec3 ambient   =  specular * ao;
    vec3 color = ambient + Lo;
//    color -= (0.1000001 * Lo * 10.0);
    color = clamp(color,0.0,1.0);
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}