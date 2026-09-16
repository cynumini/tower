#version 460

layout(location=0) in vec2 uv;
layout(location=1) in vec4 color;

layout(location=0) out vec4 color_out;

layout(set=2, binding=0) uniform sampler2D atlas;

void main() {
    color_out = texture(atlas, uv) * color;
}
