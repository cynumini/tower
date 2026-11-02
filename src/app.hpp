#pragma once

#include "ui_pipeline.hpp"
#include "voxel_pipeline.hpp"

#include "game.hpp"
#include "player.hpp"

struct App;

struct Test {
    Player aplayer;
    
    void init();
    void update(WorldInstances *instances, glm::vec2 logical);
    void updateUI(App *game, FixedVector<UIInstance> &instances, glm::ivec2 render_size);
    void deinit();
};

enum class Scene { menu, game, test };

struct App {
    Scene current_scene;
    Test test;
    Game game;
};

void initApp(App &self);
void updateApp(App &self, WorldInstances *instances, glm::vec2 logical);
void updateAppUI(App &self, FixedVector<UIInstance> &instances, glm::ivec2 render_size);
void deinitApp(App &self);
