Texture2D atlas : register(t0, space2);
SamplerState atlas_sampler : register(s0, space2);

struct Input {
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

float4 main(Input input) : SV_Target {
    return atlas.Sample(atlas_sampler, input.uv) * input.color;
}
