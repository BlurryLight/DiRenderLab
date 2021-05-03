#version 450 core
#extension GL_ARB_bindless_texture : enable
out vec4 FragColor;
in VS_OUT
{
    vec3 vFragPos;
    vec3 vNormal;
    vec3 wFragPos;
    vec3 wNormal;
    vec2 TexCoords;
} fs_in;

layout(bindless_sampler) uniform samplerCube skybox;
//layout(location = 0) uniform vec3 ucameraPos;
void main()
{
    vec3 I = normalize(fs_in.vFragPos);
    vec3 R = reflect(I, fs_in.vNormal);
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}
