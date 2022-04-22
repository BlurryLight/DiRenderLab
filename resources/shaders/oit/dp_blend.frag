#version 450 core

layout(binding = 0)uniform sampler2D tempTexture;//intermediate blending result

layout(location = 0) out vec4 vFragColor;//fragment shader output

in vec2 TexCoords;
void main()
{
    //return the intermediate blending result
    vFragColor = texture(tempTexture, TexCoords);
}