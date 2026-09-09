#pragma once

#include <skn.cpp>

struct Game;

void gameInit(Game **game, Slice<u8> mem);
void gameUpdate(Game *game);
