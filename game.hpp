#pragma once

#include <skn_math.cpp>

#include "unagi.hpp"

struct Font {
    u8 widths[256];
    Rect texture;

    void init(Rect texture) {
        for (size_t i = 0; i < 256; i++) {
            widths[i] = 4;
        }
        widths[' '] = 2;
        widths[','] = 2;
        widths['.'] = 1;
        widths['0'] = 5;
        widths['1'] = 3;
        widths['2'] = 5;
        widths['6'] = 5;
        widths['8'] = 5;
        widths['9'] = 5;
        widths[':'] = 1;
        widths['?'] = 5;
        widths['M'] = 7;
        widths['P'] = 5;
        widths['S'] = 5;
        widths['m'] = 5;
        widths['x'] = 5;
        widths['y'] = 5;
        this->texture = texture;
    }
};

struct Instance {
    vec2 position;
    vec2 size;
    Rect uv;
    Color color;
    float rotation;
};

struct Game {
    Arena *arena;

    vec2 player_size;
    vec2 player_position;
    Font font;
    bool attack;
    bool invertory_visible;
    bool cast_spell;

    void init(Engine *engine, Arena *arena);
    vec2 update(Engine *engine, Fixed<Instance> *instances);
    void updateUI(Engine *engine, Fixed<Instance> *instance) const;
};
