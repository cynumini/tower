#version 460

layout(location=0) in vec2 uv;
layout(location=1) in vec4 color_in;

layout(location=0) out vec4 color_out;

layout(set=2, binding=0) uniform sampler2D sampler2d;

void main() {
    color_out = texture(sampler2d, uv) * color_in;
}
