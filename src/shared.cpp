#pragma once

#include "engine.hpp"

#include <string_view>

#include "math.hpp"

extern Arena arena;

enum class Scene { menu, game };

struct Game;

struct SceneGame {
    void init(Engine *engine);
    void update(Engine *engine, WorldInstances *instances, FVector2 logical);
    void updateUI(Game *game, Engine *engine, FixedVector<UIInstance> &instances,
                  Vector2 render_size);
    void deinit(Engine *engine);
};
