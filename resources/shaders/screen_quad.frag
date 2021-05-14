#version 450 core

#extension GL_ARB_bindless_texture : enable
in vec2 TexCoords;

// shader outputs
layout (location = 0) out vec4 frag;

// screen image
layout (bindless_sampler) uniform sampler2D screen;

void main()
{
    frag = vec4(texture(screen, TexCoords).rgb, 1.0f);
}