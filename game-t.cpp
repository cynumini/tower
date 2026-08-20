#include "engine.hpp"

Vector3 cursor_in_world = {0, 0, 0};

GameResult gameInit([[maybe_unused]] Allocator &gpa, [[maybe_unused]] Engine &engine,
                    Renderer &renderer) {
    renderer.camera_pitch = degToRad(30);
    renderer.camera_yaw = degToRad(45);

    return GameResult::ongoing;
}

GameResult gameUpdate(Engine &engine, Renderer &renderer) {
    f32 camera_speed = engine.dt * 25;

    f32 pitch_velocity = engine.isKeyDown(Key::num1) - engine.isKeyDown(Key::num2);
    f32 yaw_velocity = engine.isKeyDown(Key::num3) - engine.isKeyDown(Key::num4);
    f32 roll_velocity = engine.isKeyDown(Key::num5) - engine.isKeyDown(Key::num6);
    renderer.camera_pitch += degToRad(pitch_velocity * camera_speed);
    renderer.camera_yaw += degToRad(yaw_velocity * camera_speed);
    renderer.camera_roll += degToRad(roll_velocity * camera_speed);

    Vector3 camera_velocity = {f32(engine.isKeyDown(Key::right) - engine.isKeyDown(Key::left)),
                               f32(engine.isKeyDown(Key::up) - engine.isKeyDown(Key::down)),
                               f32(engine.isKeyDown(Key::w) - engine.isKeyDown(Key::s))};
    renderer.camera_position += camera_velocity * camera_speed;

    auto screen_center = renderer.screen / 2.0f;
    auto mouse_ndc = (engine.getMousePosition() - screen_center) / screen_center;
    mouse_ndc.y *= -1.0f;
    auto aspect_ratio = renderer.aspect_ratio;
    auto unprojection = Matrix4::orthographicUnproject({mouse_ndc, 0.5, 1}, -10 * aspect_ratio,
                                                       10 * aspect_ratio, -10, 10, -100, 100);

    auto unview =
        Matrix4::rotateZYX({renderer.camera_pitch, -renderer.camera_yaw, -renderer.camera_roll}) *
        Matrix4::translation(renderer.camera_position.x, renderer.camera_position.y,
                             renderer.camera_position.z);

    cursor_in_world = unview * unprojection;

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

    renderer.drawCube(cursor_in_world);

    ImGui::Text("Camera position x = %f, y = %f, z = %f", renderer.camera_position.x,
                renderer.camera_position.y, renderer.camera_position.z);
    ImGui::Text("Camera rotation pitch = %f, yaw = %f, roll = %f", radToDeg(renderer.camera_pitch),
                radToDeg(renderer.camera_yaw), radToDeg(renderer.camera_roll));
    ImGui::Text("Cursor x = %f, y = %f, z = %f", cursor_in_world.x, cursor_in_world.y,
                cursor_in_world.z);
    ImGui::Text("Screen x = %f, y = %f", renderer.screen.x, renderer.screen.y);
    ImGui::Text("Aspect Ratio = %f", renderer.aspect_ratio);
    ImGui::Text("%.1f FPS", renderer.imgui_io->Framerate);
}

void gameQuit() {}
