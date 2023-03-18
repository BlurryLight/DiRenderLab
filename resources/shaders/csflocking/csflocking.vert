// This shader is for rapid prototyping
#version 450 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;

out VS_OUT
{
//output : view space Fragpos and normal
    vec3 vFragPos;
    vec3 vNormal;
//world space Fragpos and normal
    vec3 wFragPos;
    vec3 wNormal;
    vec3 Color;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

struct flock_member
{
    vec4 position; 
    vec4 velocity;
};

layout(binding = 0,std430) buffer FlockMember
{
    flock_member member[];
} flock_data;

float rand(vec2 c){
	return fract(sin(dot(c.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 choose_color(float f)
{
    float R = sin(f * 6.2831853);
    float G = sin((f + 0.3333) * 6.2831853);
    float B = sin((f + 0.6666) * 6.2831853);

    return vec3(R, G, B) * 0.25 + vec3(0.75);
}
void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0) + flock_data.member[gl_InstanceID].position;
    vs_out.wFragPos = worldPos.xyz;
    vec4 viewPos = view * worldPos;
    vs_out.vFragPos = viewPos.xyz;

    mat3 inverse_transpose = transpose(inverse(mat3(view *model)));
    vs_out.vNormal = normalize(inverse_transpose * aNormal);

    inverse_transpose = transpose(inverse(mat3(model)));
    vs_out.wNormal = normalize(inverse_transpose * aNormal);

    vs_out.Color = choose_color(fract(gl_InstanceID / float(1237))); 
    gl_Position = projection * viewPos;
}
