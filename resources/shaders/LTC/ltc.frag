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
uniform float roughness;

#define EPS 1e-4
#define PI 3.141592653589793
#define PI2 6.283185307179586
const float LUT_SIZE = 64.0;
const float LUT_SCALE = (LUT_SIZE - 1.0) / LUT_SIZE;
const float LUT_BIAS = 0.5 / LUT_SIZE;



vec3 IntegrateEdgeVec(vec3 v1, vec3 v2)
{
    float x = dot(v1, v2);
    float y = abs(x);
    float a = 5.42031 + (3.12829 + 0.0902326 * y) * y;
    float b = 3.45068 + (4.18814 + y) * y;
    float theta_sintheta = a / b;
    if (x < 0.0)
    {
        theta_sintheta = PI * inversesqrt(1.0 - x * x) - theta_sintheta;
    }
    vec3 u = cross(v1, v2);
    return theta_sintheta * u;
}


float IntegrateLTC(in vec3 v1, in vec3 v2)
{
    return IntegrateEdgeVec(v1, v2).z;
}

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

void ClipQuadToHorizon(inout vec3 L[5], out int n)
{
    // detect clipping config
    int config = 0;
    if (L[0].z > 0.0) config += 1;
    if (L[1].z > 0.0) config += 2;
    if (L[2].z > 0.0) config += 4;
    if (L[3].z > 0.0) config += 8;

    // clip
    n = 0;

    if (config == 0)
    {
        // clip all
    }
    else if (config == 1)// V1 clip V2 V3 V4
    {
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 2)// V2 clip V1 V3 V4
    {
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 3)// V1 V2 clip V3 V4
    {
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    }
    else if (config == 4)// V3 clip V1 V2 V4
    {
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    }
    else if (config == 5)// V1 V3 clip V2 V4) impossible
    {
        n = 0;
    }
    else if (config == 6)// V2 V3 clip V1 V4
    {
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 7)// V1 V2 V3 clip V4
    {
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    }
    else if (config == 8)// V4 clip V1 V2 V3
    {
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] =  L[3];
    }
    else if (config == 9)// V1 V4 clip V2 V3
    {
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    }
    else if (config == 10)// V2 V4 clip V1 V3) impossible
    {
        n = 0;
    }
    else if (config == 11)// V1 V2 V4 clip V3
    {
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    }
    else if (config == 12)// V3 V4 clip V1 V2
    {
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    }
    else if (config == 13)// V1 V3 V4 clip V2
    {
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    }
    else if (config == 14)// V2 V3 V4 clip V1
    {
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    }
    else if (config == 15)// V1 V2 V3 V4
    {
        n = 4;
    }

    if (n == 3)
    L[3] = L[0];
    if (n == 4)
    L[4] = L[0];
}


vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv2)
{
    vec3 t1 = normalize(V - N* dot(V, N));
    //构建以相交点法线的局部坐标系，用世界坐标表示
    vec3 t2 = cross(N, t1);
    //构架世界坐标系world,注意normal要是Z轴
    //局部坐标系其实是I，那么Minv * World = I, M 就等于World,其Inv等于world^T
    mat3 Minv1 = transpose(mat3(t1, t2, N));
    mat3 Minv = Minv2 * Minv1;

    //把点光源转到局部坐标系下
    vec3 L[5];
    for (int i = 0;i < 4;i++)
    {
        L[i] = Minv* (lightPoints[i] - P);
    }
    int n = 0;
    ClipQuadToHorizon(L, n);
    if (n == 0)
    return vec3(0);
    for (int i = 0;i < 5;i++)
    {
        L[i] = normalize(L[i]);
    }


    float sum = 0.0;
    sum += IntegrateLTC(L[0], L[1]);
    sum += IntegrateLTC(L[1], L[2]);
    sum += IntegrateLTC(L[2], L[3]);
    sum += IntegrateLTC(L[3], L[0]);
    sum = twoSided ? abs(sum) : max(0.0, sum);
    return vec3(sum);
    //    return vec3(1.0) + 0.00001*sum;
}
void main()
{
    vec3 view = normalize(camPos - fs_in.wFragPos);
    vec3 normal = fs_in.wNormal;
    vec3 position = fs_in.wFragPos;

    float ndotv = saturate(dot(normal, view));
    vec2 uv = vec2(roughness, sqrt(1.0 - ndotv));//wtf?
    uv = uv * LUT_SCALE + LUT_BIAS;
    vec4 t1 = texture(ltc_1, uv);
    vec4 t2 = texture(ltc_2, uv);

    mat3 Minv = mat3(
    vec3(t1.x, 0, t1.y),
    vec3(0, 1, 0),
    vec3(t1.z, 0, t1.w)
    );

    vec3 spec = LTC_Evaluate(normal, view, position, Minv);
    vec3 diff = LTC_Evaluate(normal, view, position, mat3(1));
    vec3 scol = vec3(1.0);
    vec3 spec2 = spec * (scol * t2.x + (1.0 - scol) * t2.y);
    //return max(0.0, sum);
    //    FragColor = vec4(spec + texture(diffuseTexture, fs_in.TexCoords).xyz, 1.0);
    //    FragColor = vec4(0.00000001 * spec + 0.0000001 * spec2 + diff * texture(diffuseTexture, fs_in.TexCoords).xyz, 1.0);

    vec3 res = spec2 + diff * texture(diffuseTexture, fs_in.TexCoords).xyz;
    //    res = pow(res, vec3(1 / 2.2));
    FragColor = vec4(res, 1.0);
}