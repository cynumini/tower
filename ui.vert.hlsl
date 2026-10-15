cbuffer UBO : register(b0, space1)
{
    float4x4 projection;
    float2 atlas_size;
};

struct Input {
    float2 vertex_position   : TEXCOORD0;
    float2 instance_position : TEXCOORD1;
    float2 size              : TEXCOORD2;
    float4 uv                : TEXCOORD3;
    float4 color_in          : TEXCOORD4;
};

struct Output {
    float4 position  : SV_Position;
    float2 uv        : TEXCOORD0;
    float4 color_out : TEXCOORD1;
};

Output main(Input input) {
    Output output;

    output.position = mul(
        float4(input.vertex_position * input.size + input.instance_position, 0, 1),
        projection
    );

    float2 uv = input.vertex_position;
    // float2 padding = 0.5F / atlas_size;
    float2 padding = 0;
    float2 uv_min = input.uv.xy + padding;
    float2 uv_max = input.uv.xy + input.uv.zw - padding;
    output.uv = lerp(uv_min, uv_max, float2(uv.x, uv.y));
    output.color_out = input.color_in;

    return output;
}
