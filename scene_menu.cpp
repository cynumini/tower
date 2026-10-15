#pragma once

#include "shared.cpp"

struct SceneMenu {
    void init(Engine *engine) {}

    void update(Engine *engine, WorldInstances *instances) {}

    void updateUI(Game *game, Engine *engine, Fixed<UIInstance> *instances, int render_width,
                  int render_height) {
        Slice<const char> text_play = "Play (ENTER)";
        Slice<const char> text_exit = "Exit (ESC)";
        float text_play_w = engine->measureText(text_play);
        float text_exit_w = engine->measureText(text_exit);
        float y = 2;
        float height = 26;
        engine->drawText(instances, text_play,
                         {640.F / 2.F - text_play_w / 2.F, 360.F / 2.F - height / 2 + y});
        y += 12;
        engine->drawText(instances, text_exit,
                         {640.F / 2.F - text_exit_w / 2.F, 360.F / 2.F - height / 2 + y});

        if (engine->is_key_just_pressed(Key::enter)) {
            game->current_scene = Scene::game;
        }
    };

    void deinit(Engine *engine) {}
};
