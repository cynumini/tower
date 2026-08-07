#version 460

layout(set=1, binding=0) uniform UBO {
    mat4 mvp;
};

layout(location=0) in vec2 vertexPosition;
layout(location=1) in vec2 instancePosition;
layout(location=2) in vec2 size;
layout(location=3) in vec2 uvMin;
layout(location=4) in vec2 uvMax;

layout(location=0) out vec2 out_uv;

void main() {
    gl_Position = mvp * vec4(vertexPosition * size + instancePosition, 0, 1);
    out_uv = mix(uvMin, uvMax, vertexPosition);
}
