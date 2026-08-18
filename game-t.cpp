#include "engine.hpp"

static Renderer *renderer;

GameResult gameInit([[maybe_unused]]Allocator &gpa, Engine &engine) {
    using enum GameResult;

    renderer = engine.renderer;

    return ongoing;
}
GameResult gameUpdate() {
    using enum GameResult;

    return ongoing;
}
void gameDraw() {
    renderer->drawPlane({-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}, BLUE);
    //renderer->drawPlane({-1, -1, -2}, {1, -1, -2}, {1, 1, -2}, {-1, 1, -2}, RED);
    // xrenderer->drawPlane({-1, -1, 12}, {1, -1, 12}, {1, 1, 1}, {-1, 1, 12} , BLUE);

    ImGui::Text("Hello, world %d", 123);
}
void gameQuit() {}
