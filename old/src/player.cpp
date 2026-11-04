#include "player.hpp"

#include "engine.hpp"

void updatePlayer(Player &player) {
    constexpr float mouse_sensitivity = 0.15F;
    if (engine.relative_mouse_mode) {
        engine.camera.yaw -= engine.mouse_rel.x * mouse_sensitivity;
        engine.camera.pitch -= engine.mouse_rel.y * mouse_sensitivity;
    }

    // const glm::vec2 velocity = rotate(
    //     normalize(glm::vec2{float(isKeyPressed(Scancode::d)) -
    //     float(isKeyPressed(Scancode::a)),
    //                         float(isKeyPressed(Scancode::w)) -
    //                         float(isKeyPressed(Scancode::s))}),
    //     glm::radians(engine.camera.yaw));

    glm::vec2 velocity =
        glm::vec2(float(isKeyPressed(Scancode::d)) - float(isKeyPressed(Scancode::a)),
                  float(isKeyPressed(Scancode::w)) - float(isKeyPressed(Scancode::s)));
    SDL_Log("%f %f", velocity.x, velocity.y);
    if (glm::length(velocity) > 0) {
        velocity = rotate(glm::normalize(velocity) * engine.dt, glm::radians(engine.camera.yaw));
    }
    // velocity *= engine.dt;

    SDL_Log("%f %f", velocity.x, velocity.y);

    player.pos.x += velocity.x;
    player.pos.y += velocity.y;

    engine.camera.pos = player.pos;
    if (isKeyPressed(Scancode::lshift)) {
        engine.camera.pos.z = 1.1;
    } else {
        engine.camera.pos.z = 1.8;
    }
}
