#pragma once

#include "world_pipeline.hpp"

#include "player.hpp"

struct Game {
    Player player;
};

void gameUpdate(Game &game);
