#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float uBias = 0.05;
uniform int uShadowMode = 0;
//pcf
uniform float uPCFFilterSize= 5.0;
//pcss
uniform float uPCSSBlockSize= 5.0;
uniform float uPCSSLightSize= 10.0;
#define NUM_SAMPLES 100
#define EPS 1e-4
#define PI 3.141592653589793
#define PI2 6.283185307179586
#define NUM_RINGS 10

vec4 pack_depth(const in float depth)
{
    const vec4 bit_shift = vec4(256.0*256.0*256.0, 256.0*256.0, 256.0, 1.0);
    const vec4 bit_mask  = vec4(0.0, 1.0/256.0, 1.0/256.0, 1.0/256.0);
    vec4 res = fract(depth * bit_shift);
    res -= res.xxyz * bit_mask;
    return res;
}

float unpack_depth(const in vec4 rgba_depth)
{
    const vec4 bit_shift = vec4(1.0/(256.0*256.0*256.0), 1.0/(256.0*256.0), 1.0/256.0, 1.0);
    float depth = dot(rgba_depth, bit_shift);
    return depth;
}
vec2 poissonDisk[NUM_SAMPLES];
float rand_1to1(float x) {
    // -1 -1
    return fract(sin(x)*10000.0);
}

float rand_2to1(vec2 uv) {
    // 0 - 1
    const float a = 12.9898, b = 78.233, c = 43758.5453;
    float dt = dot(uv.xy, vec2(a, b)), sn = mod(dt, PI);
    return fract(sin(sn) * c);
}
void poissonDiskSamples(const in vec2 randomSeed) {

    float ANGLE_STEP = PI2 * float(NUM_RINGS) / float(NUM_SAMPLES);
    float INV_NUM_SAMPLES = 1.0 / float(NUM_SAMPLES);

    float angle = rand_2to1(randomSeed) * PI2;
    float radius = INV_NUM_SAMPLES;
    float radiusStep = radius;

    for (int i = 0; i < NUM_SAMPLES; i ++) {
        poissonDisk[i] = vec2(cos(angle), sin(angle)) * pow(radius, 0.75);
        radius += radiusStep;
        angle += ANGLE_STEP;
    }
}
float ShadowCalculation(vec3 projCoords)
{
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    if (projCoords.z > 1.0) return 1.0;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    if (closestDepth <= EPS) closestDepth = 1.0;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    vec3 normal = normalize(fs_in.Normal);
    float bias = uBias * max((1.0 - max(dot(normal, lightDir), 0.0)), 0.03);
    //0 : blocked
    //1.0: visiable
    float shadow = currentDepth - bias> closestDepth  ? 0.0 : 1.0;

    return shadow;
}
float PCF(vec3 projCoords, float filterSize) {

    int num_sumple = 0;
    vec2 texturesz = textureSize(shadowMap, 0);
    float texelSize = 1.0 / texturesz.x;
    poissonDiskSamples(projCoords.xy);
    float currentDepth = projCoords.z;
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    vec3 normal = normalize(fs_in.Normal);
    float bias = uBias * max((1.0 - max(dot(normal, lightDir), 0.0)), 0.2);
    for (int i = 0; i < NUM_SAMPLES;i++)
    {
        vec2 offset =  filterSize *  poissonDisk[i] * texelSize;
        vec2 jittered_coords = projCoords.xy + offset;
        float closestDepth = texture2D(shadowMap, jittered_coords).r;
        //        if (closestDepth <= EPS) closestDepth = 1.0;
        num_sumple += (currentDepth - bias) > closestDepth ? 0 : 1;
    }
    return float(num_sumple) / float(NUM_SAMPLES);
}
float findBlocker(vec2 uv, float zReceiver) {

    //block search size: 5x5
    //return avgBlockerDepth
    int blocker = 0;
    float totalBlockerDepth = 0.0;
    poissonDiskSamples(uv);
    vec2 texturesz = textureSize(shadowMap, 0);
    float texelSize = 1.0 / texturesz.x;
    float filterSize = uPCSSBlockSize;
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    vec3 normal = normalize(fs_in.Normal);
    float bias = uBias * max((1.0 - max(dot(normal, lightDir), 0.0)), 0.2);
    for (int i = 0; i < NUM_SAMPLES;i++)
    {
        vec2 offset =  filterSize *  poissonDisk[i] * texelSize;
        vec2 jittered_coords = uv + offset;
        float closestDepth = texture2D(shadowMap, jittered_coords).r;
        if (closestDepth - bias < zReceiver)
        {
            blocker++;
            totalBlockerDepth += closestDepth;
        }
    }
    return totalBlockerDepth / float(blocker);
}

float PCSS(vec3 projCoords){

    // STEP 1: avgblocker depth
    float avgBlockerDepth = findBlocker(projCoords.xy, projCoords.z);
    float lightSize = uPCSSLightSize;
    // STEP 2: penumbra size
    float penmubraSize = lightSize * (projCoords.z - avgBlockerDepth) / avgBlockerDepth;
    // STEP 3: filtering
    return PCF(projCoords, penmubraSize);
}

struct phong_return
{
    vec3 ambient;
    vec3 specular;
};

phong_return BlinnPhong()
{
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.3);
    // ambient
    vec3 ambient = 0.3 * color;
    // diffuse
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    // calculate shadow
    vec3 lighting = (diffuse + specular) * color;
    return phong_return(ambient, lighting);
}
void main()
{
    phong_return res = BlinnPhong();

    // perform perspective divide
    vec3 projCoords = fs_in.FragPosLightSpace.xyz / fs_in.FragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    float visibility = 1.0;
    if (uShadowMode == 0)
    {
        visibility = ShadowCalculation(projCoords);
    }
    else if (uShadowMode == 1)
    {
        visibility = PCF(projCoords, uPCFFilterSize);
    }
    else if (uShadowMode == 2)
    {
        visibility = PCSS(projCoords);
    }
    FragColor = vec4(res.ambient + res.specular * visibility, 1.0);
}