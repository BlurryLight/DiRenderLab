#version 450 core
#extension GL_ARB_bindless_texture : enable

layout(bindless_sampler) uniform sampler2D  diffuseMaps[128];

out vec4 FragColor;
in VS_OUT
{
    vec3 vFragPos;
    vec3 vNormal;
    vec3 wFragPos;
    vec3 wNormal;
    vec2 TexCoords;
    flat uint InstanceID;
} fs_in;

struct DirLight
{
    vec4 lightDirection; // towards object
    vec4 lightAttenuation;
};
layout(binding = 0,std140) uniform LightBlock
{
    DirLight dirLights[3];
};
void main()
{
    vec3 diffuse = texture(diffuseMaps[fs_in.InstanceID % 128],fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.wNormal);
    vec3 color = vec3(0);
    for(int i = 0; i < 3;i++)
    {
        color += max(dot(-dirLights[i].lightDirection.xyz,normal),0.0) *  dirLights[i].lightAttenuation.xyz * diffuse;
    }
    FragColor = vec4(color, 1.0);
}