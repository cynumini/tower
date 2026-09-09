#version 460

layout(location=0) in vec2 uv;
layout(location=1) in vec4 color;
layout(location=2) in flat uint texture_index;

layout(location=0) out vec4 color_out;

layout(set=2, binding=0) uniform sampler2D texture0;
layout(set=2, binding=1) uniform sampler2D texture1;
layout(set=2, binding=2) uniform sampler2D texture2;
layout(set=2, binding=3) uniform sampler2D texture3;
layout(set=2, binding=4) uniform sampler2D texture4;
layout(set=2, binding=5) uniform sampler2D texture5;
layout(set=2, binding=6) uniform sampler2D texture6;
layout(set=2, binding=7) uniform sampler2D texture7;
layout(set=2, binding=8) uniform sampler2D texture8;
layout(set=2, binding=9) uniform sampler2D texture9;
layout(set=2, binding=10) uniform sampler2D texture10;
layout(set=2, binding=11) uniform sampler2D texture11;
layout(set=2, binding=12) uniform sampler2D texture12;
layout(set=2, binding=13) uniform sampler2D texture13;
layout(set=2, binding=14) uniform sampler2D texture14;
layout(set=2, binding=15) uniform sampler2D texture15;

vec4 textureAt(uint index, vec2 uv) {
    switch (index) {
        case 0: return texture(texture0, uv);
        case 1: return texture(texture1, uv);
        case 2: return texture(texture2, uv);
        case 3: return texture(texture3, uv);
        case 4: return texture(texture4, uv);
        case 5: return texture(texture5, uv);
        case 6: return texture(texture6, uv);
        case 7: return texture(texture7, uv);
        case 8: return texture(texture8, uv);
        case 9: return texture(texture9, uv);
        case 10: return texture(texture10, uv);
        case 11: return texture(texture11, uv);
        case 12: return texture(texture12, uv);
        case 13: return texture(texture13, uv);
        case 14: return texture(texture14, uv);
        case 15: return texture(texture15, uv);
    }
    return vec4(1.0);
}

void main() {
    color_out = textureAt(texture_index, uv) * color;
}
