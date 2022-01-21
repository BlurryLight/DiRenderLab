#version 330 core
out vec4 FragColor;

in VS_OUT
{
//output : view space Fragpos and normal
    vec3 vFragPos;
    vec3 vNormal;
//world space Fragpos and normal
    vec3 wFragPos;
    vec3 wNormal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D ltc_1;
uniform sampler2D ltc_2;

uniform vec3 lightCenter;
uniform vec3 lightPoints[4];
uniform vec3 camPos;

uniform bool twoSided;

#define EPS 1e-4
#define PI 3.141592653589793
#define PI2 6.283185307179586



float integrateLTC(in vec3 v1, in vec3 v2)
{
    float cosTheta = dot(v1, v2);
    float theta = acos(cosTheta);
    return cross(v1, v2).z * ((theta > 0.001) ? theta / sin(theta) : 1.0);
}


void main()
{
    vec3 view = normalize(camPos - fs_in.wFragPos);
    vec3 normal = fs_in.wNormal;
    vec3 position = fs_in.wFragPos;
    vec3 t1 = normalize(view - normal * dot(view, normal));
    //构建以相交点法线的局部坐标系，用世界坐标表示
    vec3 t2 = cross(normal, t1);
    //构架世界坐标系world,注意normal要是Z轴
    //局部坐标系其实是I，那么Minv * World = I, M 就等于World,其Inv等于world^T
    mat3 Minv = transpose(mat3(t1, t2, normal));

    //把点光源转到局部坐标系下
    vec3 L[4];
    L[0] = normalize(Minv* (lightPoints[0] - position));
    L[1] = normalize(Minv* (lightPoints[1] - position));
    L[2] = normalize(Minv* (lightPoints[2] - position));
    L[3] = normalize(Minv* (lightPoints[3] - position));

    float sum = 0.0;
    sum += integrateLTC(L[0], L[1]);
    sum += integrateLTC(L[1], L[2]);
    sum += integrateLTC(L[2], L[3]);
    sum += integrateLTC(L[3], L[0]);
    sum = twoSided ? abs(sum) : max(0.0,sum);
    //return max(0.0, sum);
    FragColor = vec4(texture(diffuseTexture,fs_in.TexCoords).xyz * sum, 1.0) ;
}