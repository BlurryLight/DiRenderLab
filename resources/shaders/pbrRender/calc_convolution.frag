#version 450 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube environmentMap;

#define kPi 3.1415926

void main()
{
    vec3 normal = normalize(TexCoords);// 以(0,0,0)为原点，以Normal作为法线，建立TBN
    vec3 WorldUp = vec3(0.0, 1.0, 0.0);
    vec3 T = normalize(cross(WorldUp, normal));
    vec3 B = normalize(cross(normal, T));

    vec3 irradiance = vec3(0.0);
    //TBN矩阵的形式是[T,B,N]，用于将一个世界坐标系的坐标转入TBN空间
    //换句话说，在TBN空间内，坐标为(1,1,1)的向量转入世界坐标系应该是(T,B,N)
    //也就是 TBN * [1,1,1]^T
    mat3 TBN_to_world = mat3(T, B, normal);

    float brdf = 1.0 / kPi;
    float pdf = 1.0 / (2 * kPi);

    int n1 = 100;
    int n2 = 100;
    float phi_step = 2 * kPi / n1;
    float theta_step = 0.5 * kPi / n2;
    float nrSamples = n1 * float(n2);
    for (float phi = 0.0; phi < 2.0 * kPi; phi += phi_step)
    {
        for (float theta = 0.0; theta < 0.5 * kPi; theta += theta_step)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = TBN_to_world * tangentSample;

            float cos_theta = dot(sampleVec, normal);
            irradiance += texture(environmentMap, sampleVec).rgb * cos_theta;
            nrSamples++;
        }
    }
    irradiance = brdf * irradiance / (pdf * nrSamples);
    FragColor = vec4(irradiance, 1.0);
}
