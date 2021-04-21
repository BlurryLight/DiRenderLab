#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gbuffer_test;
void main()
{
   float color = texture(gbuffer_test,TexCoords).r;
   FragColor = vec4(vec3(1.0 * color),1.0);
}
