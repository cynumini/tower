#include "engine.hpp"

static Renderer *renderer;

GameResult gameInit(Engine *engine) {
    using enum GameResult;

    renderer = engine->renderer;

    return ongoing;
}
GameResult gameUpdate() {
    using enum GameResult;

    return ongoing;
}
void gameDraw() {
    renderer->drawPlane({-1, -1, 0}, {1, -1, 0}, {1, 1, 1}, {-1, 1, 1}, BLUE);
    renderer->drawPlane({-1, -1, 1}, {1, -1, 1}, {1, 1, 0}, {-1, 1, 0}, RED);
    // xrenderer->drawPlane({-1, -1, 12}, {1, -1, 12}, {1, 1, 1}, {-1, 1, 12} , BLUE);

    ImGui::Text("Hello, world %d", 123);
}
void gameQuit() {}
