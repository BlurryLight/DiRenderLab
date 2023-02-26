#version 450 core

layout (binding = 0) uniform sampler2DMS screen;

// in vec2 TexCoords;

// shader outputs
layout (location = 0) out vec4 frag;

void main()
{
    frag = vec4(texelFetch(screen, ivec2(gl_FragCoord.xy),0).rgb, 1.0f);
}