#version 460

layout(set = 1, binding = 0) uniform UBO {
    vec2 screen;
    vec2 ubo_position;
    vec2 uv_position;
    vec2 uv_size;
};

layout(location=0) in vec2 position;
layout(location=1) in vec2 uv_in;

layout(location=0) out vec2 uv_out;

void main() {
    // TODO: Check which is faster: calc it here or on the CPU
    vec2 offset = -screen / 2.0F;
    vec2 scale  = 2.0F / screen;
    scale.y *= -1.0F;
    gl_Position = vec4((position + offset + ubo_position) * scale, 0.0F, 1.0F);
    uv_out = uv_position + (uv_in * uv_size);
}
