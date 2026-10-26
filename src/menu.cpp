#include "menu.hpp"

#include <string_view>
#include "game.hpp"

void menuUpdateUI(Game &game, const Engine &engine, FixedVector<UIInstance> &instances,
                  Vector2 render_size) {
    std::string_view text_play = "Play (ENTER)";
    std::string_view text_exit = "Exit (ESC)";
    float text_play_w = engine.measureText(text_play);
    float text_exit_w = engine.measureText(text_exit);
    float y = 2;
    const float height = 26;
    engine.drawText(instances, text_play,
                    {float(render_size.x) / 2.F - text_play_w / 2.F,
                     float(render_size.y) / 2.F - height / 2 + y});
    y += 12;
    engine.drawText(instances, text_exit,
                    {float(render_size.x) / 2.F - text_exit_w / 2.F,
                     float(render_size.y) / 2.F - height / 2 + y});

    if (engine.isKeyJustPressed(Key::enter)) {
        game.current_scene = Scene::game;
    }
}
