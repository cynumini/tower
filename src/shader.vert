#version 460

layout(set = 1, binding = 0) uniform UBO {
    vec2 screen;
    vec2 camera;
};

layout(location=0) in vec2 vertex_position;
layout(location=1) in vec2 instance_position;
layout(location=2) in vec2 instance_size;
layout(location=3) in vec2 uv_position;
layout(location=4) in vec2 uv_size;
layout(location=5) in vec4 color_in;
layout(location=6) in float rotation;
layout(location=7) in uint texture_index_in;

layout(location=0) out vec2 uv_out;
layout(location=1) out vec4 color_out;
layout(location=2) out uint texture_index_out;

void main() {
    vec2 proj_scale  = 2.0F / screen;
    vec2 proj_offset = -screen / 2.0F;
    proj_scale.y *= -1;

    vec2 position = vertex_position * instance_size;

    vec2 center = instance_size * 0.5F;
    position -= center;

    float c = cos(rotation);
    float s = sin(rotation);

    position = vec2(position.x * c - position.y * s,
                    position.x * s + position.y * c);

    position += center + instance_position;

    position = (position + proj_offset + camera) * proj_scale;
    gl_Position = vec4(position, 0.0F, 1.0F);

    uv_out = uv_position + (vertex_position * uv_size);
    color_out = color_in;
    texture_index_out = texture_index_in;
}
