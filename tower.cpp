#include "tower.hpp"

#include "unagi.hpp"

struct Game {
    Sprite *world;
};

void gameInit(Game **game, Slice<u8> mem) {
    *game = (Game *)mem.ptr;
    (*game)->world = loadSprite("resources/world.png");
}

void gameUpdate(Game *game) {
    drawSprite(game->world, {0, 0});
}
