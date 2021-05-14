#version 450 core

#if defined(GL_ARB_bindless_texture)
#extension GL_ARB_bindless_texture : enable
#endif
#if defined(GL_ARB_bindless_texture)
layout(bindless_sampler) uniform sampler2D texture0;
#else
layout(binding = 0) uniform sampler2D texture0;
#endif

out vec4 FragColor;
in VS_OUT
{
    vec3 vFragPos;
    vec3 vNormal;
    vec3 wFragPos;
    vec3 wNormal;
    vec2 TexCoords;
} fs_in;
void main()
{
    FragColor = texture(texture0, fs_in.TexCoords);
}