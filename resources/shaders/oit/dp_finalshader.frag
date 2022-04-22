#version 330 core

layout(location = 0) out vec4 vFragColor;//fragment shader output

//uniforms
uniform sampler2D colorTexture;//colour texture from previous pass
//uniform vec4 vBackgroundColor;//background colour

in vec2 TexCoords;

void main()
{
    //get the colour from the colour buffer
    vec4 color = texture(colorTexture, TexCoords);
    vFragColor = color;
    //combine the colour read from the colour texture with the background colour
    //by multiplying the colour alpha with the background colour and adding the
    //product to the given colour uniform
    //        vFragColor = color + vBackgroundColor*color.a;
    //    vFragColor = color;
}