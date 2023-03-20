#version 450 core
out vec4 FragColor;
in VS_OUT
{
    vec3 vFragPos;
    vec3 vNormal;
    vec3 wFragPos;
    vec3 wNormal;
    vec3 Color;
} fs_in;
void main()
{
    FragColor = vec4(fs_in.Color, 1.0);
    // FragColor = vec4(0.5 * fs_in.wNormal + 0.5, 1.0);
}