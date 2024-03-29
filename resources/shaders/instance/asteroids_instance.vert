#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

layout (std430, binding = 0) readonly buffer InstanceData
{
    mat4 mats[];
} input_data;


out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform float time_f;

void main()
{
    TexCoords = aTexCoords;
    float sin_f = sin(time_f);
    float cos_f = cos(time_f);
    mat4 model_rotate = mat4(1.0f);
    //rotate by y axis
    model_rotate[0] = vec4(cos_f, 0.0, -sin_f, 0.0);
    model_rotate[2] = vec4(sin_f, 0.0f, cos_f, 0.0);
    gl_Position = projection * view * (model_rotate * input_data.mats[gl_InstanceID]) * vec4(aPos, 1.0f);
}