#version 450 core

#if defined(GL_ARB_bindless_texture)
#extension GL_ARB_bindless_texture : enable
#endif
#if defined(GL_ARB_bindless_texture)
layout(bindless_sampler) uniform sampler2D screen;
#else
// screen image
layout (binding = 0) uniform sampler2D screen;
#endif

in vec2 TexCoords;

// shader outputs
layout (location = 0) out vec4 frag;


void main()
{
    frag = vec4(texture(screen, TexCoords).rgb, 1.0f);
}