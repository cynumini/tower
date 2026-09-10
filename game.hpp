#include <skn_math.cpp>

struct Instance {
    vec2 position;
    vec2 size;
    Rect uv;
    Color color;
    f32 rotation;
    uint texture_index;
};

uint gameUpdate(Slice<Instance> instances);
