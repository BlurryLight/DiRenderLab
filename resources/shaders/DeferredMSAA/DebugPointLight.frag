#version 330 core
layout (location = 0 ) out vec3 gNormal;
layout (location = 1) out vec3 gPosition;
layout (location = 2) out vec4 FragColor;

in VS_OUT
{
//output : view space Fragpos and normal
    vec3 vFragPos;
    vec3 vNormal;
//world space Fragpos and normal
    vec3 wFragPos;
    vec3 wNormal;
    vec2 TexCoords;
} fs_in;


uniform vec3 lightColor;
void main()
{
    gPosition = fs_in.wFragPos;
    gNormal = 0.5 * normalize(fs_in.wNormal) + 0.5;
    FragColor = vec4(lightColor,1.0);
}