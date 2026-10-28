#pragma once

#include "shared.cpp"

struct Game {
    Scene current_scene;
    SceneGame scene_game;
};

void initGame(Game &self, Engine &engine);
void updateGame(Game &self, Engine &engine, WorldInstances *instances, FVector2 logical);
void updateGameUI(Game &self, Engine &engine, FixedVector<UIInstance> &instances, Vector2 render_size);
void deinitGame(Game &self, Engine &engine);
