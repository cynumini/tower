#include "game.hpp"

#include "player.hpp"

void gameUpdate(Game &game) {
    updatePlayer(game.player);

    worldColor(RED);
    worldVertex({-5.F, -5.F, 0.F});
    worldColor(GREEN);
    worldVertex({5.F, -5.F, 0.F});
    worldColor(BLUE);
    worldVertex({0.0F, 5.F, 0.F});

    worldColor(RED);
    worldVertex({-5.F, -5.F, 1.F});
    worldColor(GREEN);
    worldVertex({5.F, -5.F, 1.F});
    worldColor(BLUE);
    worldVertex({0.0F, 5.F, 1.F});
}
