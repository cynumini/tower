#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game.hpp"
#include "world.hpp"
#include "ui.hpp"


int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    Game game;
    World world;
    UI ui;

    auto texture = game.loadTexture("world.png");
    auto texture_font = game.loadTexture("font.png");

    while (!game.shouldClose()) {
        world.update();
        ui.update();

        game.begin();
        world.draw();
        ui.draw();
        game.drawTexture(texture_font, {{0, 0}, {8, 8}}, {{64, 0}, {32, 32}});
        game.drawTexture(texture, {{0, 0}, {32, 32}}, {{128, 128}, {32, 32}});
        game.drawTexture(texture_font, {{0, 0}, {8, 8}}, {{64, 0}, {32, 32}});
        game.end();
    }
    return 0;
}
