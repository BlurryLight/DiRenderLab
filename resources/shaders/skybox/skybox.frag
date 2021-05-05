#version 450 core
#extension GL_ARB_bindless_texture : enable
out vec4 FragColor;

in vec3 TexCoords;

layout(bindless_sampler) uniform samplerCube skybox;

void main()
{
    FragColor = vec4(pow(texture(skybox, TexCoords).xyz, vec3(0.45)), 1.0);
}