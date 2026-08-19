#include <time.h>

#include "engine.hpp"

Vector3 cursor = {0, 0, 0};

GameResult gameInit([[maybe_unused]] Allocator &gpa, Engine &engine) {
    engine.set
    engine.renderer->pitch = 30;
    engine.renderer->yaw = 45;

    return GameResult::ongoing;
}

GameResult gameUpdate(Engine &engine) {
    f32 camera_speed = engine.dt * 25;

    // TODO: pitch, yaw, roll of what?
    f32 pitch_velocity = engine.isKeyDown(Key::num1) - engine.isKeyDown(Key::num2);
    f32 yaw_velocity   = engine.isKeyDown(Key::num3) - engine.isKeyDown(Key::num4);
    f32 roll_velocity  = engine.isKeyDown(Key::num5) - engine.isKeyDown(Key::num6);
    engine.renderer->pitch += pitch_velocity * camera_speed;
    engine.renderer->yaw   += yaw_velocity   * camera_speed;
    engine.renderer->roll  += roll_velocity  * camera_speed;

    Vector3 camera_velocity = {
        f32(engine.isKeyDown(Key::right) - engine.isKeyDown(Key::left)),
        f32(engine.isKeyDown(Key::up)    - engine.isKeyDown(Key::down)),
        f32(engine.isKeyDown(Key::w)     - engine.isKeyDown(Key::s))
    };
    engine.renderer->camera_position += camera_velocity * camera_speed;

    auto mouse = engine.getMousePosition();
    auto screen_center = engine.renderer->screen / 2.0f;
    // what is mouse?
    mouse = mouse - screen_center;

    // TODO: what is 10? what is world?
    Vector4 world{
         mouse.x / screen_center.x * (10.0f * engine.renderer->aspect_ratio),
        -mouse.y / screen_center.y * 10.0f,
        0.0f,
        1.0f
    };

    // TODO: what is rotatioin?
    auto rotation = Matrix4::rotate({0, 0, 1}, engine.renderer->roll) *
                    Matrix4::rotate({0, 1, 0}, engine.renderer->yaw) *
                    Matrix4::rotate({1, 0, 0}, engine.renderer->pitch);

    // TODO: what is cursor?
    cursor = rotation * Matrix4::translation(engine.renderer->camera_position.x,
                                             engine.renderer->camera_position.y,
                                             engine.renderer->camera_position.z) * world;

    return GameResult::ongoing;
}

void gameDraw(Renderer &renderer) {
    // TODO: add level loading from file
    srand(1);
    f32 value = 0;

    for (f32 z = 0; z < 10; z++) {
        for (f32 x = 0; x < 10; x++) {
            value = value + f32(rand() % 3 - 1);
            renderer.drawCube({x, value, z});
        }
    }

    renderer.drawCube(cursor);

    ImGui::Text("Camera position x = %f, y = %f, z = %f",
                renderer.camera_position.x,
                renderer.camera_position.y,
                renderer.camera_position.z);
    ImGui::Text("Camera rotation pitch = %f, yaw = %f, roll = %f",
                renderer.pitch,
                renderer.yaw,
                renderer.roll);
    ImGui::Text("Cursor x = %f, y = %f, z = %f", cursor.x, cursor.y, cursor.z);
    ImGui::Text("Screen x = %f, y = %f", renderer.screen.x, renderer.screen.y);
    ImGui::Text("Aspect Ratio = %f", renderer.aspect_ratio);
    ImGui::Text("%.1f FPS", renderer.imgui_io->Framerate);
}

void gameQuit() {}
