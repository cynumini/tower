#pragma once

#include "shared.cpp"

#include "scene_game.cpp"
#include "scene_menu.cpp"

SceneGame scene_game;
SceneMenu scene_menu;

void Game::init(Engine *engine) {
    scene_menu.init(engine);
    scene_game.init(engine);
}

void Game::update(Engine *engine, WorldInstances *instances, Vec2 logical) {
    switch (current_scene) {
    case Scene::menu:
        scene_menu.update(engine, instances, logical);
        break;
    case Scene::game:
        scene_game.update(engine, instances, logical);
        break;
    }
}

void Game::updateUI(Engine *engine, Fixed<UIInstance> *instances, Vec2i render_size) {
    switch (current_scene) {
    case Scene::menu:
        scene_menu.updateUI(this, engine, instances, render_size);
        break;
    case Scene::game:
        scene_game.updateUI(this, engine, instances, render_size);
        break;
    }
}

void Game::deinit(Engine *engine) {
    scene_menu.deinit(engine);
    scene_game.deinit(engine);
}

static Game game;
