#version 460

layout(set=1, binding=0) uniform UBO {
    mat4 projection;
    mat4 view;
};

layout(location=0) in vec2 vertexPosition;
layout(location=1) in vec2 instancePosition;
layout(location=2) in vec2 size;
layout(location=3) in float rotation;
layout(location=4) in vec2 uvPosition;
layout(location=5) in vec2 uvSize;
layout(location=6) in vec4 inColor;
layout(location=7) in uint inFlags;

layout(location=0) out vec2 outUv;
layout(location=1) out vec4 outColor;
layout(location=2) out uint outFlags;

void main() {
    vec2 position = (vertexPosition - 0.5) * size;

    float c = cos(rotation);
    float s = sin(rotation);

    position = vec2(
        position.x * c - position.y * s,
        position.x * s + position.y * c
    );

    position += instancePosition;

    gl_Position = projection * view * vec4(position, 0.0, 1.0);

    outUv = uvPosition + vertexPosition * uvSize;
    outColor = inColor;
    outFlags = inFlags;
}
