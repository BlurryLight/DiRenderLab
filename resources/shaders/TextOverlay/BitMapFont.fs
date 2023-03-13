#version 450 core
out vec4 FragColor;
layout (origin_upper_left) in vec4 gl_FragCoord;
layout(binding=0) uniform isampler2D text_buffer;
layout(binding=1) uniform isampler2D font_texture;

uniform ivec4 BitMapInfo; // char pixel width, char pixel height, bitmap size width, font scale 


ivec2 CalcCharOffset(int character,ivec2 char_size)
{
    int row = character / BitMapInfo.z;
    int col = character % BitMapInfo.z;
    return ivec2(col * char_size.x,row * char_size.y);
}
void main()
{
    // 左上边界留5个像素
    ivec2 frag_coord = ivec2(gl_FragCoord.xy / BitMapInfo.w) - ivec2(5,5);
    if(any(lessThan(frag_coord,ivec2(0)))) discard;

    ivec2 char_size = BitMapInfo.xy;
    ivec2 char_location = frag_coord / char_size;
    int character = texelFetch(text_buffer,char_location,0).x;
    ivec2 texel_coord = frag_coord % char_size;
    float val = texelFetch(font_texture, CalcCharOffset(character,char_size) + ivec2(texel_coord), 0).x;
    if(val == 0.0) discard;
    FragColor = vec4(1.0,0.0,0.0,1.0);
}