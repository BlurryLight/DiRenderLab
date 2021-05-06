#version 450 core
#extension GL_ARB_bindless_texture : enable
out vec4 FragColor;

in vec3 TexCoords;

layout(bindless_sampler) uniform samplerCube skybox;
uniform float u_lod_level= 1.0;
void main()
{
    FragColor = vec4(pow(textureLod(skybox, TexCoords, u_lod_level).xyz, vec3(0.45)), 1.0);
}