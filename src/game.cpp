#include "game.hpp"

#include "engine.hpp"
#include "scene_game.cpp"

void initGame(Game &self) { self.scene_game.init(); }

void updateGame(Game &self, WorldInstances *instances, glm::vec2 logical) {
    switch (self.current_scene) {
    case Scene::menu:
        break;
    case Scene::game:
        self.scene_game.update(instances, logical);
        break;
    }
}

void updateGameUI(Game &self, FixedVector<UIInstance> &instances, glm::ivec2 render_size) {
    switch (self.current_scene) {
    case Scene::menu: {
        std::string_view text_play = "Play (ENTER)";
        std::string_view text_exit = "Exit (ESC)";
        int text_play_w = int(measureText(text_play));
        int text_exit_w = int(measureText(text_exit));
        int y = 2;
        const int height = 26;
        drawText(
            instances, text_play,
            {render_size.x / 2 - text_play_w / 2,  render_size.y / 2 - height / 2 + y});
        y += 12;
        drawText(
            instances, text_exit,
            {render_size.x / 2 - text_exit_w / 2,  render_size.y / 2 - height / 2 + y});

        if (isKeyJustPressed(Scancode::enter)) {
            self.current_scene = Scene::game;
            toggleRelativeMouseMode();
        }
    } break;
    case Scene::game:
        self.scene_game.updateUI(&self, instances, render_size);
        break;
    }
}

void deinitGame(Game &self) { self.scene_game.deinit(); }
