#pragma once

#include "cell.hpp"

struct Structure {
    static constexpr int MAX_SIZE = 32;
    glm::ivec3 pos;
    std::array<std::array<std::array<Cell, MAX_SIZE>, MAX_SIZE>, MAX_SIZE> data;
    std::vector<Object> objects;
};

void structureInit(std::vector<Structure> &structures, MaterialMap map);
