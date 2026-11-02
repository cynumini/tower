struct Input {
    float4 color : TEXCOORD1;
};

float4 main(Input input) : SV_Target {
    if (input.color.a < 0.01) discard;
    return input.color;
}
