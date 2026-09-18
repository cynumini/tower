#pragma once

#include <skn_math.cpp>

#include "unagi.hpp"

struct Game {
    static void init(Engine *engine, Arena *arena);
    static vec2 update(Engine *engine, Fixed<Instance> *instances);
    static void updateUI(Engine *engine, Fixed<Instance> *instance);
};
