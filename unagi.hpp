#pragma once

#include <skn.cpp>
#include <skn_math.cpp>

struct Sprite {
    uint id;
    ivec2 size;
};

Sprite *loadSprite(const char *file);

void drawSprite(Sprite *sprite, vec2 positon);
