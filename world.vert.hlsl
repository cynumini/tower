cbuffer UBO : register(b0, space1)
{
    float4x4 view;
    float4x4 projection;
};

struct Input {
    float3 pos   : TEXCOORD0;
    float4 color : TEXCOORD1;
};

struct Output {
    float4 pos : SV_Position;
    float4 color : TEXCOORD0;
};

Output main(Input input) {
    Output output;
    output.pos = mul(projection, mul(view, float4(input.pos, 1)));
    output.color = input.color;
    return output;
}
