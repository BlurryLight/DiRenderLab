#version 450 core

layout(location = 0) out vec4 vFragColor;//fragment shader output

//uniforms
layout(binding = 0) uniform sampler2D texture0;
layout(binding = 1) uniform sampler2D  depthTexture;//depth texture
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
    //read the depth value from the depth texture
    float frontDepth = texture(depthTexture, fs_in.TexCoords).r;

    //compare the current fragment depth with the depth in the depth texture
    //if it is less, discard the current fragment
    if (gl_FragCoord.z <= (frontDepth + 0.001))
    discard;

    //otherwise set the given color uniform as the final output
    vFragColor = texture(texture0, fs_in.TexCoords);
}
