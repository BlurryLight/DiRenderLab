#version 450 core

layout (binding = 0) uniform sampler2DMS gAlbedo;
layout (binding = 1) uniform sampler2DMS gNormal;
layout (binding = 2) uniform sampler2DMS gPosition;

// in vec2 TexCoords;

// shader outputs
layout (location = 0) out vec4 frag;

uniform vec3 lightDir;

struct PointLight {    
    vec4 position;
    // padding
    vec4 color;
    // padding
    float constant;
    float linear;
    float quadratic;  
    float padding;
    // padding
};  
#define NR_POINT_LIGHTS 50 
layout (std140) uniform PointLightBlock
{
    PointLight pointLights[NR_POINT_LIGHTS];
};

vec3 CalcPointLight(PointLight light, vec3 albedo,vec3 normal, vec3 fragPos)
{
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    float distance    = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
    vec3 diffuse  = light.color.xyz * diff * albedo * attenuation;
    return clamp(diffuse,0.0,1.0);
} 

void main()
{
    int SampleNum = textureSamples(gAlbedo);
    vec3 res = vec3(0);
    for(int i =0 ; i < SampleNum;i++)
    {
        vec3 texColor = texelFetch(gAlbedo, ivec2(gl_FragCoord.xy),i).rgb;
        vec3 wNormal = texelFetch(gNormal, ivec2(gl_FragCoord.xy),i).rgb * 2.0 - 1.0;
        vec3 wFragPos = texelFetch(gPosition, ivec2(gl_FragCoord.xy),i).rgb;
        vec3 lightColor = vec3(0.5);
        vec3 diffuse = max(dot(wNormal,-lightDir),0.0) * lightColor * texColor;
        res += diffuse;
        for(int j = 0; j < NR_POINT_LIGHTS;j++)
        {
            res += CalcPointLight(pointLights[j],texColor,wNormal,wFragPos);
        }
    }
    res /= SampleNum;
    frag = vec4(res,1.0);
}