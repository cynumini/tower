#version 460

layout(set = 1, binding = 0) uniform UBO {
    vec2 scale;
    vec2 offset;
};

layout(location=0) in vec2 position;

void main() {
    gl_Position = vec4((position + offset) * scale, 0, 1);
}
