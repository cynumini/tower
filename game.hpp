#pragma once

#include <skn_math.cpp>

#include "unagi.hpp"

struct Game {
    static void init(Unagi *unagi);
    static void deinit(Unagi *unagi);
    static vec2 update(Unagi *unagi, Fixed<Instance> *instances);
    static void updateUI(Unagi *unagi, Fixed<Instance> *instance);
};
