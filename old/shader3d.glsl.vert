#version 460

layout(set=1, binding=0) uniform UBO {
    mat4 projection;
    mat4 view;
};

layout(location=0) in vec3 vertexPosition;
layout(location=1) in vec4 color;

layout(location=0) out vec4 outColor;

void main() {
    gl_Position = projection * vec4(vertexPosition, 1);
    outColor = color;
}
