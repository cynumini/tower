cbuffer UBO : register(b0, space1)
{
    int2 screen;
    float2 camera;
};

struct Input {
    float2 vertex_position   : TEXCOORD0;
    float2 instance_position : TEXCOORD1;
    float2 size              : TEXCOORD2;
    float2 uv_position       : TEXCOORD3;
    float2 uv_size           : TEXCOORD4;
    float4 color_in          : TEXCOORD5;
    float rotation           : TEXCOORD6;
};

struct Output {
    float4 position  : SV_Position;
    float2 uv        : TEXCOORD0;
    float4 color_out : TEXCOORD1;
};

Output main(Input input) {
    Output output;

    float2 proj_scale = 2.0 / screen;
    float2 proj_offset = -screen / 2.0;
    proj_scale.y *= -1;

    float2 position = input.vertex_position * input.size;

    float2 center = input.size * 0.5;
    position -= center;

    float c = cos(input.rotation);
    float s = sin(input.rotation);

    position = float2(
        position.x * c - position.y * s,
        position.x * s + position.y * c
    );

    position += center + input.instance_position;

    position = (position + proj_offset + camera) * proj_scale;

    output.position = float4(position, 0.0, 1.0);
    output.uv = input.uv_position + input.vertex_position * input.uv_size;
    output.color_out = input.color_in;

    return output;
}
