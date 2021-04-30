#version 450 core
out vec3 TexCoords;
uniform mat4 view;
void main()
{
    vec3[4] vertices = vec3[4](vec3(-1.0, -1.0, 1.0), //left-bottom
    vec3(1.0, -1.0, 1.0), //right-bottom
    vec3(-1.0, 1.0, 1.0), //left-top
    vec3(1.0, 1.0, 1.0)//right-top
    );
    TexCoords = mat3(view) * vertices[gl_VertexID];
    gl_Position = vec4(vertices[gl_VertexID], 1.0);
}