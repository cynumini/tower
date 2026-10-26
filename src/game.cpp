#pragma once

#include "shared.cpp"

#include "menu.hpp"
#include "scene_game.cpp"

SceneGame scene_game;

void Game::init(Engine *engine) { scene_game.init(engine); }

void Game::update(Engine *engine, WorldInstances *instances, Vec2 logical) {
    switch (current_scene) {
    case Scene::menu:
        break;
    case Scene::game:
        scene_game.update(engine, instances, logical);
        break;
    }
}

void Game::updateUI(Engine *engine, FixedVector<UIInstance> &instances, Vec2i render_size) {
    switch (current_scene) {
    case Scene::menu:
        menuUpdateUI(*this, *engine, instances, {.x = render_size.x, .y = render_size.y});
        break;
    case Scene::game:
        scene_game.updateUI(this, engine, instances, render_size);
        break;
    }
}

void Game::deinit(Engine *engine) { scene_game.deinit(engine); }

static Game game;
