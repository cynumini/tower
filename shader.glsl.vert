#version 460

layout(set=1, binding=0) uniform UBO {
    mat4 mvp;
};

layout(location=0) in vec2 position;
layout(location=1) in vec2 uv;

layout(location=0) out vec2 out_uv;

void main() {
    gl_Position = vec4(position, 0, 1);
    out_uv = uv;
}
