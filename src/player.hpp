#pragma once

#include <glm/glm.hpp>

struct Player {
    glm::vec3 pos;
};

void updatePlayer(Player &player);
