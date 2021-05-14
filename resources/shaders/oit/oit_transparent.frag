#version 450 core

#if defined(GL_ARB_bindless_texture)
#extension GL_ARB_bindless_texture : enable
#endif
#if defined(GL_ARB_bindless_texture)
layout(bindless_sampler) uniform sampler2D texture0;
#else
layout(binding = 0) uniform sampler2D texture0;
#endif
// shader outputs
layout (location = 0) out vec4 accum;
layout (location = 1) out float reveal;

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
    // weight function
    vec4 color = texture(texture0, fs_in.TexCoords);
    float weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);

    // store pixel color accumulation
    accum = vec4(color.rgb * color.a, color.a) * weight;

    // store pixel revealage threshold
    reveal = color.a;
}