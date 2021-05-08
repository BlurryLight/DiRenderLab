#version 450 core

#if defined(GL_ARB_bindless_texture)
#extension GL_ARB_bindless_texture : enable
#endif
#if defined(GL_ARB_bindless_texture)
layout(bindless_sampler) uniform sampler2D  equirectangularMap;
#else
layout(binding = 0 ) uniform sampler2D equirectangularMap;
#endif

out vec4 FragColor;
in vec3 TexCoords;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(TexCoords));
    vec3 color = texture(equirectangularMap, uv).rgb;

    FragColor = vec4(color, 1.0);
}