#version 330 core
out float FragColor;
in vec2 TexCoords;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D tNoise;

uniform vec3 samples[64];
uniform float radius = 0.5;
uniform float bias = 0.025;
uniform mat4 projection;

const vec2 noiseScale = vec2(800.0/4.0,600.0/4.0);


void main()
{
    vec3 fragPos = texture(gPosition,TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal,TexCoords).rgb);
    vec3 rvec =  normalize(texture(tNoise,TexCoords * noiseScale).xyz);
    vec3 tangent = normalize(rvec - normal * dot(rvec,normal));
//    if(normal.x > normal.y)
//    {
//        tangent = normalize(vec3(-normal.z,0,normal.x));
//    }
//    else
//    {
//        tangent = normalize(vec3(0,-normal.z,normal.y));
//        }
    vec3 bitangent = cross(normal,tangent);
    mat3 TBN = mat3(tangent,bitangent,normal);
    float occlusion = 0.0;
    for(int i =0;i < 64;++i)
    {
        vec3 sample = TBN * samples[i];
        sample = fragPos + sample * radius;

        //project view-pos to projection-pos
        vec4 tmp = vec4(sample,1.0);
        tmp = projection * tmp;
        tmp.xyz /= tmp.w;
        tmp.xyz = tmp.xyz * 0.5 + 0.5;
        float sampleDepth = texture(gPosition,tmp.xy).z;
        //clamp大概也可以
        //防止在边缘处出现occlusion突变
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= sample.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / 64.0);

    FragColor = occlusion;
}
