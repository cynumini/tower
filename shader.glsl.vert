#version 460

layout(set=1, binding=0) uniform UBO {
    mat4 mvp;
};

layout(location=0) in vec2 vertexPosition;
layout(location=1) in vec2 instancePosition;
layout(location=2) in vec2 size;
layout(location=3) in vec2 uvPosition;
layout(location=4) in vec2 uvSize;
layout(location=5) in vec4 inColor;
layout(location=6) in uint inFlags;

layout(location=0) out vec2 outUv;
layout(location=1) out vec4 outColor;
layout(location=2) out uint outFlags;

void main() {
    gl_Position = mvp * vec4(vertexPosition * size + instancePosition, 0, 1);
    outUv = uvPosition + vertexPosition * uvSize;
    outColor = inColor;
    outFlags = inFlags;
}
