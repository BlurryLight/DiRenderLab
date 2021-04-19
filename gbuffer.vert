#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform bool invertNormal=false;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // 在gbuffer里面保存摄像机空间内的坐标、法线和深度，这样可以用texture来查询坐标、法线和纹理
    vec4 viewPos = view * model * vec4(aPos,1.0);
    FragPos = viewPos.xyz;
    TexCoords = aTexCoords;
    mat3 inverse_transpose = transpose(inverse(mat3(view *model)));
    Normal = inverse_transpose * (invertNormal ? -aNormal : aNormal);
    gl_Position = projection * viewPos;
}
