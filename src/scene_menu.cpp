#pragma once

#include "shared.cpp"

struct SceneMenu {
    void init([[maybe_unused]]Engine *engine) {}

    void update([[maybe_unused]]Engine *engine, [[maybe_unused]]WorldInstances *instances, [[maybe_unused]]Vec2 logical) {}

    void updateUI(Game *game, Engine *engine, Fixed<UIInstance> *instances, Vec2i render_size) {
        OldSlice<const char> text_play = "Play (ENTER)";
        OldSlice<const char> text_exit = "Exit (ESC)";
        f32 text_play_w = engine->measureText(text_play);
        f32 text_exit_w = engine->measureText(text_exit);
        f32 y = 2;
        f32 height = 26;
        engine->drawText(
            instances, text_play,
            {render_size.x / 2.F - text_play_w / 2.F, render_size.y / 2.F - height / 2 + y});
        y += 12;
        engine->drawText(
            instances, text_exit,
            {render_size.x / 2.F - text_exit_w / 2.F, render_size.y / 2.F - height / 2 + y});

        if (engine->isKeyJustPressed(Key::enter)) {
            game->current_scene = Scene::game;
        }
    };

    void deinit([[maybe_unused]]Engine *engine) {}
};
