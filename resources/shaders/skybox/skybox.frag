#version 450 core
#if defined(GL_ARB_bindless_texture)
#extension GL_ARB_bindless_texture : enable
#endif

out vec4 FragColor;
in vec3 TexCoords;

#if defined(GL_ARB_bindless_texture)
layout(bindless_sampler) uniform samplerCube skybox;
#else
layout(binding=0) uniform samplerCube skybox;
#endif


uniform float u_lod_level= 1.0;
void main()
{
    FragColor = vec4(pow(textureLod(skybox, TexCoords, u_lod_level).xyz, vec3(0.45)), 1.0);
}