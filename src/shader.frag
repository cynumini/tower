#version 460

layout(location=0) in vec2 uv;

layout(location=0) out vec4 color;

layout(set=2, binding=0) uniform sampler2D sampler2d;

void main() {
    color = texture(sampler2d, uv) * vec4(1, 1, 1, 1);
}
