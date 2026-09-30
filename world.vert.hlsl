cbuffer UBO : register(b0, space1)
{
    float4x4 view;
    float4x4 projection;
};

struct Input {
    float2 vertex_position   : TEXCOORD0;
    float3 instance_position : TEXCOORD1;
    float2 size              : TEXCOORD2;
    float4 uv                : TEXCOORD3;
    float4 color_in          : TEXCOORD4;
    uint   face              : TEXCOORD5;
};

struct Output {
    float4 position  : SV_Position;
    float2 uv        : TEXCOORD0;
    float4 color_out : TEXCOORD1;
};

Output main(Input input) {
    Output output;

    float2 p = input.vertex_position * input.size;
    float3 position;

    switch (input.face) {
        case 0: position = float3(0,  p.x,  p.y); break; // X+
        case 1: position = float3(0, -p.x,  p.y); break; // X-
        case 2: position = float3(-p.x, 0,  p.y); break; // Y+
        case 3: position = float3( p.x, 0,  p.y); break; // Y-
        case 4: position = float3( p.x,  p.y, 0); break; // Z+
        case 5: position = float3(-p.x,  p.y, 0); break; // Z-
    }


    output.position = mul(
        mul(float4(position + input.instance_position, 1), view),
        projection
    );

    output.uv = input.uv.xy + (input.vertex_position + float2(0.5F, 0.5F)) * input.uv.zw;
    output.color_out = input.color_in;

    return output;
}
