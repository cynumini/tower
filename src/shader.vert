#version 460

layout(set = 1, binding = 0) uniform UBO {
    vec2 screen;
    vec2 camera;
    bool center;
};

layout(location=0) in vec2 position;
layout(location=1) in vec2 instance_position;
layout(location=2) in vec2 instance_size;
layout(location=3) in vec2 uv_position;
layout(location=4) in vec2 uv_size;
layout(location=5) in vec4 color_in;
layout(location=6) in float rotation;
layout(location=7) in int texture_index_in;

layout(location=0) out vec2 uv_out;
layout(location=1) out vec4 color_out;
layout(location=2) out int texture_index_out;

void main() {
    // TODO: Check which is faster: calc it here or on the CPU
    vec2 proj_offset = -screen / 2.0F;
    vec2 proj_scale  = 2.0F / screen;
    proj_scale.y *= -1.0F;
    vec2 pos0 = position * instance_size; // apply instance size
    pos0 = vec2(pos0.x * cos(rotation) - pos0.y * sin(rotation),
                pos0.x * sin(rotation) + pos0.y * cos(rotation)); // apply instance rotation
    if (center) {
        pos0 += instance_position; // apply instance position
    } else {
        pos0 += instance_position + instance_size / 2; // apply instance position
    }
    pos0 += screen / 2 + camera; // apply camera
    pos0 = (pos0 + proj_offset) * proj_scale; // apply projection
    gl_Position = vec4(pos0, 0.0F, 1.0F);
    uv_out = uv_position + ((position + vec2(0.5, 0.5)) * uv_size);
    color_out = color_in;
    texture_index_out = texture_index_in;
}
