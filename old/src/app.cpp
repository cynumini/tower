#include "app.hpp"

#include "engine.hpp"
#include "test.hpp"
#include "game.hpp"

void initApp(App &self) { self.test.init(); }

void updateApp(App &self, WorldInstances *instances, glm::vec2 logical) {
    switch (self.current_scene) {
    case Scene::game:
        gameUpdate(self.game);
    case Scene::menu:
        break;
    case Scene::test:
        self.test.update(instances, logical);
        break;
    }
}

void updateAppUI(App &self, FixedVector<UIInstance> &instances, glm::ivec2 render_size) {
    switch (self.current_scene) {
    case Scene::menu: {
        auto lines = 3;
        std::string_view text_play = "Play (ENTER)";
        std::string_view text_test = "Test (1)";
        std::string_view text_exit = "Exit (ESC)";
        int text_play_w = int(measureText(text_play));
        int text_test_w = int(measureText(text_test));
        int text_exit_w = int(measureText(text_exit));
        int y = 2;
        const int height = (lines * 10) + ((lines + 1) * 2);
        drawText(instances, text_play,
                 {render_size.x / 2 - text_play_w / 2, render_size.y / 2 - height / 2 + y});
        y += 12;
        drawText(instances, text_test,
                 {render_size.x / 2 - text_test_w / 2, render_size.y / 2 - height / 2 + y});
        y += 12;
        drawText(instances, text_exit,
                 {render_size.x / 2 - text_exit_w / 2, render_size.y / 2 - height / 2 + y});

        if (isKeyJustPressed(Scancode::enter)) {
            self.current_scene = Scene::game;
            toggleRelativeMouseMode();
        }
        if (isKeyJustPressed(Scancode::sc_1)) {
            self.current_scene = Scene::test;
            toggleRelativeMouseMode();
        }
    } break;
    case Scene::test:
        self.test.updateUI(&self, instances, render_size);
        break;
    }
}

void deinitApp(App &self) { self.test.deinit(); }
