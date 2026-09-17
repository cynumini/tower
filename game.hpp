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
        widths['0'] = 5;
        widths['2'] = 5;
        widths['6'] = 5;
        widths['8'] = 5;
        widths['9'] = 5;
        widths['?'] = 5;
        widths['M'] = 7;
        widths['x'] = 5;
        widths['y'] = 5;
        this->texture = texture;
    }
};

struct GameResult {
    size_t instances_len;
    size_t ui_instance_offset;
    vec2 camera;
};

struct Instance {
    vec2 position;
    vec2 size;
    Rect uv;
    Color color;
    float rotation;
};

struct Game {
    vec2 player_size;
    vec2 player_position;
    Font font;
    bool attack;
    bool invertory_visible;
    bool cast_spell;

    void init(Engine *engine);
    GameResult update(Engine *engine, Arena *a,
                          Fixed<Instance> instances);
};
