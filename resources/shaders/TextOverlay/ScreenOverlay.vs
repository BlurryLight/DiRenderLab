#version 450 core
void main(void)
{
    // Id = 0,  -1, -1
    // Id = 1,  -1, 1,0
    // Id = 2,  1, -1,0
    // Id = 3,  1, 1,0
    float x = float(gl_VertexID / 2) * 2.0 - 1.0;
    float y = float(gl_VertexID & 1) * 2.0 - 1.0;
    gl_Position = vec4(x,y,0.0,1.0);
}