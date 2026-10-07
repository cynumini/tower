cbuffer UBO : register(b0, space1)
{
    float4x4 view;
    float4x4 projection;
    float2 atlas_size;
    float yaw;
};

struct Input {
    float2 vertex_position   : TEXCOORD0;
    float3 instance_position : TEXCOORD1;
    float2 size              : TEXCOORD2;
    float4 uv                : TEXCOORD3;
    float4 color          : TEXCOORD4;
    uint   face              : TEXCOORD5;
    float  angle             : TEXCOORD6;
};

struct Output {
    float4 position  : SV_Position;
    float2 uv        : TEXCOORD0;
    float4 color : TEXCOORD1;
};

float2 rotate(float2 v, float rad) {
    float c = cos(rad);
    float s = sin(rad);
    return float2(v.x * c - v.y * s, v.x * s + v.y * c);
}

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
        case 6: {
            float2 direction = float2(cos(yaw), sin(yaw));
            position = float3(direction * p.x, p.y);
            break;
        } // billboard
    }

    position.xy = rotate(position.xy, input.angle);

    output.position = mul(
        mul(float4(position + input.instance_position, 1), view),
        projection
    );

    float2 uv = input.vertex_position + float2(0.5F, 0.5F);
    float2 padding = 0.001F / atlas_size;
    float2 uv_min = input.uv.xy + padding;
    float2 uv_max = input.uv.xy + input.uv.zw - padding;
    output.uv = lerp(uv_min, uv_max, float2(uv.x, 1.0F - uv.y));

    output.color = input.color;

    return output;
}
