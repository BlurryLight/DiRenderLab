#version 450 core

layout (binding = 0) uniform sampler2D screen;

in vec2 TexCoords;

// shader outputs
layout (location = 0) out vec4 frag;

void main()
{
    frag = vec4(texture(screen, TexCoords).rgb, 1.0f);
}