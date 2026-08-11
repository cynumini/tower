#version 460

layout(location=0) in vec2 uv;
layout(location=1) in vec4 color;
layout(location=2) flat in int useTexture;

layout(location=0) out vec4 frag_color;

layout(set=2, binding=0) uniform sampler2D tex_sampler;

void main() {
    if (useTexture == 1) {
        frag_color = texture(tex_sampler, uv) * color;
    } else {
        frag_color = color;
    }
}
