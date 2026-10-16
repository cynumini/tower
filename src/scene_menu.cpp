#pragma once

#include "shared.cpp"

struct SceneMenu {
    void init(Engine *engine) {}

    void update(Engine *engine, WorldInstances *instances) {}

    void updateUI(Game *game, Engine *engine, Fixed<UIInstance> *instances, Vec2i render_size) {
        Slice<const char> text_play = "Play (ENTER)";
        Slice<const char> text_exit = "Exit (ESC)";
        float text_play_w = engine->measureText(text_play);
        float text_exit_w = engine->measureText(text_exit);
        float y = 2;
        float height = 26;
        engine->drawText(
            instances, text_play,
            {render_size.x / 2.F - text_play_w / 2.F, render_size.y / 2.F - height / 2 + y});
        y += 12;
        engine->drawText(
            instances, text_exit,
            {render_size.x / 2.F - text_exit_w / 2.F, render_size.y / 2.F - height / 2 + y});

        if (engine->is_key_just_pressed(Key::enter)) {
            game->current_scene = Scene::game;
        }
    };

    void deinit(Engine *engine) {}
};
