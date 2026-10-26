#pragma once

#include "fixed_vector.hpp"
#include "math.hpp"
#include "shared.cpp"

void menuUpdateUI(Game &game, const Engine &engine, FixedVector<UIInstance> &instances,
                  Vector2 render_size);
