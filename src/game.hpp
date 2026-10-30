#pragma once

#include "ui_pipeline.hpp"
#include "world_pipeline.hpp"

struct Game;

struct SceneGame {
    void init();
    void update(WorldInstances *instances, glm::vec2 logical);
    void updateUI(Game *game, FixedVector<UIInstance> &instances, glm::ivec2 render_size);
    void deinit();
};

enum class Scene { menu, game };

struct Game {
    Scene current_scene;
    SceneGame scene_game;
};

void initGame(Game &self);
void updateGame(Game &self, WorldInstances *instances, glm::vec2 logical);
void updateGameUI(Game &self, FixedVector<UIInstance> &instances, glm::ivec2 render_size);
void deinitGame(Game &self);
