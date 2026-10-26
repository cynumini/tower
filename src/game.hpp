#pragma once

#include "shared.cpp"

struct Game {
    Scene current_scene;
    SceneGame scene_game;
};

void initGame(Game &game, Engine &engine);
void updateGame(Game &game, Engine &engine, WorldInstances *instances, Vec2 logical);
void updateGameUI(Game &game, Engine &engine, FixedVector<UIInstance> &instances, Vec2i render_size);
void deinitGame(Game &game, Engine &engine);
