#version 460

layout(set=1, binding=0) uniform UBO {
    mat4 mvp;
};

layout(location=0) in vec2 vertexPosition;
layout(location=1) in vec2 uv;
layout(location=2) in vec2 instancePosition;

layout(location=0) out vec2 out_uv;

void main() {
    gl_Position = mvp * vec4(vertexPosition + instancePosition, 0, 1);
    out_uv = uv;
}
