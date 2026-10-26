#pragma once

#include "map.hpp"

struct Structure {
    static constexpr int MAX_SIZE = 32;
    std::array<float, 3> pos;
    std::array<std::array<std::array<Cell, MAX_SIZE>, MAX_SIZE>, MAX_SIZE> data;
    std::vector<Object> objects;
};

void structure_init(const Engine &engine, std::vector<Structure> &structures);
