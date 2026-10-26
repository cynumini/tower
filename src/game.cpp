#include "game.hpp"

#include "menu.hpp"
#include "scene_game.cpp"

void initGame(Game &self, Engine &engine) { self.scene_game.init(&engine); }

void updateGame(Game &self, Engine &engine, WorldInstances *instances, Vec2 logical) {
    switch (self.current_scene) {
    case Scene::menu:
        break;
    case Scene::game:
        self.scene_game.update(&engine, instances, logical);
        break;
    }
}

void updateGameUI(Game &self, Engine &engine, FixedVector<UIInstance> &instances, Vec2i render_size) {
    switch (self.current_scene) {
    case Scene::menu:
        menuUpdateUI(self, engine, instances, {.x = render_size.x, .y = render_size.y});
        break;
    case Scene::game:
        self.scene_game.updateUI(&self, &engine, instances, render_size);
        break;
    }
}

void deinitGame(Game &self, Engine &engine) { self.scene_game.deinit(&engine); }
