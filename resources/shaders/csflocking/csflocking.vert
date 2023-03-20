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
    //calc normal
    vec3 forward = normalize(flock_data.member[gl_InstanceID].velocity.xyz);
    vec3 world_up = vec3(0,1,0);
    vec3 right_side = cross(forward,world_up);
    vec3 bird_up = cross(right_side,forward);
    // 获得了bird的局部坐标系 right_side(x), bird_up(y),forward(z)
    mat4 ToWorld = mat4(
        vec4(right_side,0),
        vec4(bird_up,0),
        vec4(forward,0) ,
        vec4(0,0,0,1));

    vec3 Normal = mat3(ToWorld) * aNormal;
    // cs更新的是delta Pos，需要乘以ToWorld矩阵
    // aPos传入的是初始的WorldPos，直接加，注意w轴的1.0不要加两次
    vec4 worldPos = ToWorld * vec4(flock_data.member[gl_InstanceID].position.xyz,1.0) + vec4(aPos,0.0);
    vs_out.wFragPos = worldPos.xyz;
    vec4 viewPos = view * worldPos;
    vs_out.vFragPos = viewPos.xyz;


    vs_out.wNormal = Normal;
    vs_out.vNormal = mat3(view) * Normal;

    vs_out.Color = choose_color(fract(gl_InstanceID / float(1237))); 
    gl_Position = projection * viewPos;
}
