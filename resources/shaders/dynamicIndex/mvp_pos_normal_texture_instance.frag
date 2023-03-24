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
void main()
{
    vec3 color = texture(diffuseMaps[fs_in.InstanceID % 128],fs_in.TexCoords).rgb;
    FragColor = vec4(color, 1.0);
}