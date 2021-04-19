#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gbuffer_test;
void main()
{
   vec3 color = texture(gbuffer_test,TexCoords).rgb;
//   color = normalize(color);
   FragColor = vec4(color * 0.5 + 0.5,1.0);
}
