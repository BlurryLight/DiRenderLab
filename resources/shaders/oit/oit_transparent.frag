#version 450 core

layout(binding = 0) uniform sampler2D texture0;
// shader outputs
layout (location = 0) out vec4 accum;
layout (location = 1) out float reveal;

in VS_OUT
{
    vec3 vFragPos;
    vec3 vNormal;
    vec3 wFragPos;
    vec3 wNormal;
    vec2 TexCoords;
} fs_in;
void main()
{
    // accum记录所有透明像素堆叠在一起，加权出来的颜色，alpha通道记录归一化的因子
    // reveal纹理要记录Π(1 - a)的值，以便最后在和solid纹理叠加的时候计算  transplarent * (1 - Π ( 1 - a)) + solid * (
    vec4 color = texture(texture0, fs_in.TexCoords);//就按普通的方法计算shading，这里没有光照就直接采样纹理了
    // weight function
    // 见https://jcgt.org/published/0002/02/09/paper.pdf 第7页
    // 大致上这会是一个随着像素距离相机增加而weight减少的减函数(离相机越近，其贡献越多)
    // 大概是公式10的变种
    // https://img.blurredcode.com/img/test-2022-04-17-20-42-44.png?x-oss-process=style/compress(图中的Z坐标是viewspace的坐标)
    // 在乱序渲染透明物体的时候，距离相机越近，其贡献越高。所有像素的颜色按权重加权到一起，最后会在composite阶段归一化
    float weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);

    // store pixel color accumulation
    // rt0 Blend Func: GL_ONE,GL_ONE
    // rt0的SRC是vec4(color.rgb * color.a, color.a) * weight, DEST是上一个透明物体写入的透明像素值
    accum = vec4(color.rgb * color.a, color.a) * weight;

    // store pixel revealage threshold
    // rt1 Blend Func: GL_Zero,GL_ONE_MINUS_SRC_COLOR
    // reveal的SRC是color.a, Dest是x
    // blend function 是
    // SRC * GL_ZERO + Dst * (1 - SRC)
    // 所以reveal纹理的值就是 Π(1 - color.a)。 堆叠的透明物体越多，写到纹理的reveal值越低
    // 极端情况，假如透明纹理的边框不透明，color.a是1。
    // 那么写入到reveal的纹理值会是0(用renderdoc看)
    reveal = color.a;
}