#version 330 core
out vec4 FragColor;

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


void main()
{
    FragColor = vec4(1.0,1.0,0.0,1.0);
}