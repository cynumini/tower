#include "engine.hpp"

#include "csv.hpp"

glm::vec3 cursor_in_world = {0, 0, 0};
f32 y = 0;

constexpr u8 MAP_SIZE = 10;
u8 map[MAP_SIZE][MAP_SIZE][MAP_SIZE]{};

void loadMap(Allocator &gpa) {
    auto csv = CSV::init(gpa, "level.csv", true);

    Optional<DynamicArray<String>> row;
    while (true) {
        row = csv.readline(gpa);
        if (!row.has_value) break;
        auto id = row.payload.data[0].parseNumber<u8>();
        auto x = row.payload.data[1].parseNumber<u8>();
        auto y = row.payload.data[2].parseNumber<u8>();
        auto z = row.payload.data[3].parseNumber<u8>();
        map[x][y][z] = id;
        row.payload.deinit(gpa);
    }
    csv.deinit(gpa);
}

void clearMap() {
    for (u8 x = 0; x < MAP_SIZE; x++) {
        for (u8 y = 0; y < MAP_SIZE; y++) {
            for (u8 z = 0; z < MAP_SIZE; z++) {
                map[x][y][z] = 0;
            }
        }
    }
}

GameResult gameInit(Allocator &gpa, [[maybe_unused]] Engine &engine, Renderer &renderer) {
    renderer.camera_position.x = -10;
    renderer.camera_position.y = 10;
    renderer.camera_position.z = -10;
    renderer.camera_pitch = glm::radians(30.f);
    renderer.camera_yaw = glm::radians(45.f);
    loadMap(gpa);

    return GameResult::ongoing;
}

GameResult gameUpdate(Allocator &gpa, Engine &engine, Renderer &renderer) {
    f32 camera_speed = engine.dt * 25;

    f32 pitch_velocity = engine.isKeyDown(Key::num1) - engine.isKeyDown(Key::num2);
    f32 yaw_velocity = engine.isKeyDown(Key::num3) - engine.isKeyDown(Key::num4);
    f32 roll_velocity = engine.isKeyDown(Key::num5) - engine.isKeyDown(Key::num6);
    renderer.camera_pitch += glm::radians(pitch_velocity * camera_speed);
    renderer.camera_yaw += glm::radians(yaw_velocity * camera_speed);
    renderer.camera_roll += glm::radians(roll_velocity * camera_speed);

    glm::vec3 camera_velocity = {f32(engine.isKeyDown(Key::right) - engine.isKeyDown(Key::left)),
                                 f32(engine.isKeyDown(Key::up) - engine.isKeyDown(Key::down)),
                                 f32(engine.isKeyDown(Key::w) - engine.isKeyDown(Key::s))};
    renderer.camera_position += camera_velocity * camera_speed;

    // auto screen_center = renderer.screen / 2.0f;
    // auto mouse_ndc = (engine.getMousePosition() - screen_center) / screen_center;
    // mouse_ndc.y *= -1.0f;
    // auto aspect_ratio = renderer.aspect_ratio;
    // auto unprojection = Matrix4::orthographicUnproject({mouse_ndc, 0.5, 1}, -10 * aspect_ratio,
    //                                                    10 * aspect_ratio, -10, 10, -100, 100);

    // auto unview =
    //     Matrix4::rotateZYX({renderer.camera_pitch, -renderer.camera_yaw, -renderer.camera_roll})
    //     * Matrix4::translation(renderer.camera_position.x, renderer.camera_position.y,
    //                          renderer.camera_position.z);

    // cursor_in_world = unview * unprojection;

    if (engine.isKeyPressed(Key::f5)) {
        clearMap();
        loadMap(gpa);
    }

    if (engine.isKeyPressed(Key::f3)) {
        switch (renderer.projection_kind) {
        case Projection::perspective: {
            renderer.projection_kind = Projection::orthographic;
            break;
        }
        case Projection::orthographic: {
            renderer.projection_kind = Projection::perspective;
            break;
        }
        }
        renderer.updateProjection();
    }

    auto screen_center = renderer.screen / 2.0f;
    auto mouse_ndc = (engine.getMousePosition() - screen_center) / screen_center;
    mouse_ndc.y *= -1;

    auto inv_vp = glm::inverse(renderer.projection3d * renderer.view);

    auto start = inv_vp * glm::vec4(mouse_ndc.x, mouse_ndc.y, 0, 1);
    start /= start.w;
    auto end = inv_vp * glm::vec4(mouse_ndc.x, mouse_ndc.y, 1, 1);
    end /= end.w;
    auto ray = glm::normalize(glm::vec3(end) - glm::vec3(start));

    glm::vec3 ray_origin = glm::vec3(start);

    float t = -ray_origin.y / ray.y;

    cursor_in_world = ray_origin + ray * t;

    y += f32(engine.isKeyPressed(Key::space) - engine.isKeyPressed(Key::lshift));
    cursor_in_world.y = y;

    cursor_in_world = {glm::round(cursor_in_world.x), glm::round(cursor_in_world.y),
                       glm::round(cursor_in_world.z)};

    auto x = usize(cursor_in_world.x);
    auto y = usize(cursor_in_world.y);
    auto z = usize(cursor_in_world.z);

    if (engine.isKeyPressed(Key::enter)) {

        map[x][y][z] = 1;
    }

    if (engine.isKeyPressed(Key::del)) {

        map[x][y][z] = 0;
    }

    return GameResult::ongoing;
}

void gameDraw(Renderer &renderer) {
    // TODO: add level loading from file
    // srand(1);
    // f32 value = 0;

    for (u8 x = 0; x < MAP_SIZE; x++) {
        for (u8 y = 0; y < MAP_SIZE; y++) {
            for (u8 z = 0; z < MAP_SIZE; z++) {
                if (map[x][y][z] == 1) {
                    renderer.drawCube({f32(x), f32(y), f32(z)});
                }
            }
        }
    }

    // for (f32 z = 0; z < 10; z++) {
    //     for (f32 x = 0; x < 10; x++) {
    //         value = value + f32(rand() % 3 - 1);

    //     }
    // }

    renderer.drawCube(cursor_in_world);

    {
        using namespace ImGui;
        Text("Camera position x = %f, y = %f, z = %f", renderer.camera_position.x,
             renderer.camera_position.y, renderer.camera_position.z);
        Text("Camera rotation pitch = %f, yaw = %f, roll = %f", glm::degrees(renderer.camera_pitch),
             glm::degrees(renderer.camera_yaw), glm::degrees(renderer.camera_roll));
        Text("Cursor x = %f, y = %f, z = %f", cursor_in_world.x, cursor_in_world.y,
             cursor_in_world.z);
        Text("Screen x = %f, y = %f", renderer.screen.x, renderer.screen.y);
        Text("Aspect Ratio = %f", renderer.aspect_ratio);
        Text("%.1f FPS", renderer.imgui_io->Framerate);
    }
}

void gameQuit([[maybe_unused]]Allocator &gpa) {}
