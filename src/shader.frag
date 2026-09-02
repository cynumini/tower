#version 460

layout(location=0) in vec2 uv;
layout(location=1) in vec4 color;
layout(location=2) in flat int texture_index;

layout(location=0) out vec4 color_out;

layout(set=2, binding=0) uniform sampler2D texture0;
layout(set=2, binding=1) uniform sampler2D texture1;

void main() {
    if (texture_index == 0) {
        color_out = texture(texture0, uv) * color;
    } else if (texture_index == 1) {
        color_out = texture(texture1, uv) * color;
    }
}
