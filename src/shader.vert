#version 460

layout(set = 1, binding = 0) uniform UBO {
    vec2 screen;
};

layout(location=0) in vec2 position;
layout(location=1) in vec2 instance_position;
layout(location=2) in vec2 instance_size;
layout(location=3) in vec2 uv_position;
layout(location=4) in vec2 uv_size;
layout(location=5) in float rotation;

layout(location=0) out vec2 uv_out;

void main() {
    // TODO: Check which is faster: calc it here or on the CPU
    vec2 offset = -screen / 2.0F;
    vec2 scale  = 2.0F / screen;
    scale.y *= -1.0F;
    vec2 a = position * instance_size;
    a = vec2(a.x * cos(rotation) - a.y * sin(rotation),
              a.y = a.x * sin(rotation) + a.y * cos(rotation));
    gl_Position = vec4((a + offset + instance_position) * scale, 0.0F, 1.0F);
    uv_out = uv_position + ((position + vec2(0.5, 0.5)) * uv_size);
}
