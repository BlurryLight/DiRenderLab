// This shader is for rapid prototyping
#version 450 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aTexCoords;
layout (location=3) in vec3 aTNormal;
layout (location=4) in vec3 aBNormal;

out VS_OUT
{
//output : view space Fragpos and normal
    vec3 vFragPos;
    vec3 vNormal;
//world space Fragpos and normal
    vec3 wFragPos;
    vec3 wNormal;
//    vec3 wTNormal;
//    vec3 wBNormal;
    vec2 TexCoords;
    mat3 TBN;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vs_out.TexCoords = aTexCoords;
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.wFragPos = worldPos.xyz;
    vec4 viewPos = view * worldPos;
    vs_out.vFragPos = viewPos.xyz;

    mat3 inverse_transpose = transpose(inverse(mat3(view *model)));
    vs_out.vNormal = normalize(inverse_transpose * aNormal);

    inverse_transpose = transpose(inverse(mat3(model)));
    vs_out.wNormal = normalize(inverse_transpose * aNormal);
    vec3 wTNormal = normalize(inverse_transpose * aTNormal);
    vec3 wBNormal = normalize(inverse_transpose * aBNormal);
    vs_out.TBN = mat3(wTNormal, wBNormal, vs_out.wNormal);

    gl_Position = projection * viewPos;
}
