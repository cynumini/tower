#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <imgui.h>

#include "base.cpp"
#include "font.cpp"
#include "renderer.cpp"

struct Object {
    enum Kind { Zero, Player, Static, Enemy };

    Kind kind{};
    Rectangle texture{};
    Vector2 center{};
    Vector2 position{};
    Vector2 scale{1, 1};
    Rectangle collision{};
    Rectangle interaction_arena{};
    int hp = 100;
    int mp = 100;
    bool alive = true;
    bool in_defence = false;
    bool inactive = false;
    u64 inactive_start = 0;

    Rectangle calcAbsolutePositionOfRelativeRectangle(const Rectangle &rect) const { return rect.scale(scale) + position; }
};

struct AppState;

constexpr usize MAX_OBJECTS = 1024;

constexpr Vector2 SCREEN = {640, 360};

struct World {
    using ObjectId = usize;
    usize texture;
    Font font;
    FixedArray<Object, MAX_OBJECTS> objects{};
    FixedArray<ObjectId, MAX_OBJECTS> render_order{};
    Vector2 camera_target{};
    bool show_collision = false;
    bool show_interaction_arena = false;
    bool is_dialog = false;
    ObjectId player_id = 0;
    ObjectId current_enemy_id = 0;
    bool battle_mode = false;
    bool pause = false;
    int select = 0;
    bool is_player_turn = true;
    u64 ticks;

    void addObject(const Object &object) {
        ObjectId id = objects.add(object);
        render_order.add(id);
        if (object.kind == Object::Player) {
            player_id = id;
        }
    }

    Object &getObject(ObjectId id) {

        assert(id != 0);
        assert(id < objects.len);
        return objects[id];
    }

    World(usize texture, Font font) : texture(texture), font(font) { addObject({}); }

    void update(AppState &app_state);

    void draw(Renderer &renderer) {
        if (battle_mode) {
            constexpr f32 height = 28;
            Color selection[4] = {BLACK, BLACK, BLACK, BLACK};
            selection[select] = BLUE;
            auto &player = getObject(player_id);
            auto &enemy = getObject(current_enemy_id);
            static char buffer[128];
            sprintf(buffer, "HP: %d MP: %d", player.hp, player.mp);
            font.drawText(renderer, {128 - 18, 256 - 32 - 10}, 10, buffer);
            renderer.drawTexture(texture, player.texture, {128, 256 - 32, 32, 64}, renderer.base_model_view);
            sprintf(buffer, "HP: %d MP: %d", enemy.hp, enemy.mp);
            font.drawText(renderer, {SCREEN.x - 128 - 18, 64 - 10}, 10, buffer);
            renderer.drawTexture(texture, enemy.texture, {SCREEN.x - 128, 64, 32, 32}, renderer.base_model_view);

            renderer.drawRectangle({4, SCREEN.y - (height + 4), 74, height}, renderer.base_model_view, selection[0]);
            font.drawText(renderer, {8, SCREEN.y - 28}, 20, "ATTACK");
            renderer.drawRectangle({193 - 4, SCREEN.y - (height + 4), 72, height}, renderer.base_model_view, selection[1]);
            font.drawText(renderer, {193, SCREEN.y - 28}, 20, "DEFEND");
            renderer.drawRectangle({382 - 4, SCREEN.y - (height + 4), 56 + 8, height}, renderer.base_model_view, selection[2]);
            font.drawText(renderer, {382, SCREEN.y - 28}, 20, "ITEMS");
            renderer.drawRectangle({568 - 4, SCREEN.y - (height + 4), 64 + 8, height}, renderer.base_model_view, selection[3]);
            font.drawText(renderer, {568, SCREEN.y - 28}, 20, "ESCAPE");
            return;
        }
        for (auto id : render_order) {
            if (id == 0)
                continue;
            auto &object = this->getObject(id);
            if (!object.alive)
                continue;
            auto position = object.position - (object.center * object.scale);
            auto size = Vector2{object.texture.w * object.scale.x, object.texture.h * object.scale.y};
            auto model_view = renderer.base_model_view * Matrix4::translation(-camera_target.x + SCREEN.x / 2, -camera_target.y + SCREEN.y / 2, 0);
            renderer.drawTexture(texture, object.texture, {position, size}, model_view, object.inactive ? GRAY : WHITE);
            if (show_collision) {
                renderer.drawRectangle(object.calcAbsolutePositionOfRelativeRectangle(object.collision), model_view, {0, 0, 1, 0.5f});
            }
            if (show_interaction_arena) {
                renderer.drawRectangle(object.calcAbsolutePositionOfRelativeRectangle(object.interaction_arena), model_view, {1, 0, 0, 0.5f});
            }
        }
    }
};

struct AppState {
    SDL_Window *window;
    Renderer *renderer;
    World *world;

    usize world_texture;
    Font font;

    bool show_debug_menu = false;
    bool fullscreen = false;
    bool should_close = false;

    f32 dt{};
    u64 previous;

    Array<bool, SDL_SCANCODE_COUNT> pressed = {};
    Array<bool, SDL_SCANCODE_COUNT> pressed_repeat = {};
    const bool *keyboard_state = nullptr;

    char text[256];

    AppState() {
        f32 scale_factor = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        SDL_ENSURE(scale_factor != 0.0f, "get display content scale");

        window = SDL_CreateWindow("Tower", int(SCREEN.x * scale_factor), int(SCREEN.y * scale_factor), SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        SDL_ENSURE(window, "create window");

        keyboard_state = SDL_GetKeyboardState(nullptr);

        renderer = new Renderer{window, SCREEN, scale_factor};

        world_texture = renderer->loadTexture("world.png");
        font = {*renderer, "font.csv", "font.png", 10};

        world = new World{world_texture, font};

        world->addObject({
            .kind = Object::Player,
            .texture = {{96, 0}, {16, 32}},
            .center = {8, 32},
            .position = {640.f / 2.f, 360.f / 2.f},
            .collision = {{-8, -4}, {16, 4}},
        });
        srand((u32)time(0));
        for (usize i = 0; i < 10; i++) {
            auto max_x = SCREEN.x * 4;
            auto max_y = SCREEN.y * 4;
            f32 x = f32(random() % int(max_x));
            f32 y = f32(random() % int(max_y));
            x -= max_x / 2;
            y -= max_y / 2;
            world->addObject({
                .kind = Object::Enemy,
                .texture = {{112, 0}, {16, 16}},
                .center = {8, 16},
                .position = {x, y},
                .collision = {{-4, -3}, {8, 3}},
                .interaction_arena = {{-8, -16}, {16, 16}},
            });
        }

        for (usize i = 0; i < 256; i++) {
            auto max_x = SCREEN.x * 4;
            auto max_y = SCREEN.y * 4;
            f32 x = f32(random() % int(max_x));
            f32 y = f32(random() % int(max_y));
            x -= max_x / 2;
            y -= max_y / 2;
            world->addObject({.kind = Object::Static,
                              .texture = {{64, 0}, {32, 64}},
                              .center = {16, 64},
                              .position = {x, y},
                              .collision = {{-3, -3}, {6, 3}},
                              .interaction_arena = {{-6, -6}, {12, 12}}});
        }
    }

    SDL_AppResult event(const SDL_Event &event) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
        case SDL_EVENT_QUIT: {
            return SDL_APP_SUCCESS;
        }
        case SDL_EVENT_KEY_DOWN: {
            if (event.key.scancode == SDL_SCANCODE_F3) {
                show_debug_menu = !show_debug_menu;
            }
            if (event.key.scancode == SDL_SCANCODE_F10) {
                return SDL_APP_SUCCESS;
            }
            if (event.key.scancode == SDL_SCANCODE_F11) {
                fullscreen = !fullscreen;
                SDL_SetWindowFullscreen(window, fullscreen);
            }
            if (!event.key.repeat) {
                pressed[event.key.scancode] = true;
            }
            pressed_repeat[event.key.scancode] = true;
            break;
        }
        case SDL_EVENT_WINDOW_RESIZED: {
            f32 width = (f32)event.window.data1;
            f32 height = (f32)event.window.data2;
            f32 x_offset = 0, y_offset = 0;
            auto aspect_ratio = width / height;
            if (16.0 / 9.0 > aspect_ratio) {
                auto scale = SCREEN.x / width;
                width *= scale;
                height *= scale;
                y_offset = (height - SCREEN.y) / 2;

            } else {
                auto scale = SCREEN.y / height;
                width *= scale;
                height *= scale;
                x_offset = (width - SCREEN.x) / 2;
            }
            renderer->base_model_view = Matrix4::orthographic(0, width, height, 0, 0, 1) * Matrix4::translation(x_offset, y_offset, 0);
            break;
        }
        }
        return SDL_APP_CONTINUE;
    }

    SDL_AppResult iterate() {
        u64 now = SDL_GetTicks();
        dt = f32(now - previous) / 1000.f;
        previous = now;
        if (should_close)
            return SDL_APP_SUCCESS;
        world->update(*this);
        renderer->frameBegin();
        world->draw(*renderer);
        if (show_debug_menu) {
            ImGui::Begin("Debug menu");
            ImGui::Text("Version: 0.3.0");
            ImGui::Text("%.1f FPS", renderer->imgui_io->Framerate);
            ImGui::Text("x: %.2f, y: %.2f", world->camera_target.x, world->camera_target.y);
            ImGui::Checkbox("Show collisions", &world->show_collision);
            ImGui::Checkbox("Show interaction arenas", &world->show_interaction_arena);
            if (ImGui::Button("Break")) {
                raise(SIGINT);
            }
            ImGui::End();
        }

        if (world->is_dialog) {
            renderer->drawRectangle({{0, SCREEN.y / 3 * 2}, {SCREEN.x, SCREEN.y / 3}}, renderer->base_model_view, BLACK);
            font.drawText(*renderer, {8, SCREEN.y / 3 * 2 + 8}, 10, "It's a tree!");
        };

        renderer->frameEnd();
        pressed = {};
        pressed_repeat = {};
        return SDL_APP_CONTINUE;
    }

    ~AppState() {
        delete renderer;
        SDL_DestroyWindow(window);
    }

    bool isKeyDown(SDL_Scancode key) { return keyboard_state[key]; }
    bool isKeyPressed(SDL_Scancode key) { return pressed[key]; }
    bool isKeyPressedRepeat(SDL_Scancode key) { return pressed_repeat[key]; }
};

void World::update(AppState &app_state) {
    ticks = SDL_GetTicks();
    if (battle_mode) {
        auto &player = getObject(player_id);
        auto &enemy = getObject(current_enemy_id);
        if (!is_player_turn) {
            auto &player = getObject(player_id);
            player.hp -= player.in_defence ? 1 : 10;
            if (player.hp <= 0) {
                app_state.should_close = true;
            }
            player.in_defence = false;
            is_player_turn = true;
            return;
        }
        if (app_state.isKeyPressedRepeat(SDL_SCANCODE_A)) {
            if (select == 0) {
                select = 3;
            } else {
                select -= 1;
            }
        } else if (app_state.isKeyPressedRepeat(SDL_SCANCODE_D)) {
            if (select == 3) {
                select = 0;
            } else {
                select += 1;
            }
        } else if (app_state.isKeyPressedRepeat(SDL_SCANCODE_SPACE)) {
            switch (select) {
            case 0: {

                enemy.hp -= 25;
                if (enemy.hp <= 0) {
                    enemy.alive = false;
                    battle_mode = false;
                    pause = false;
                    return;
                } else {
                    is_player_turn = false;
                }
                break;
            }
            case 1: {
                player.in_defence = true;
                is_player_turn = false;
                break;
            }
            case 2: {
                player.hp += 25;
                is_player_turn = false;
                break;
            }
            case 3: {
                battle_mode = false;
                pause = false;
                enemy.inactive = true;
                enemy.inactive_start = ticks;
                return;
                break;
            }
            }
        }
        return;
    }
    for (ObjectId object_id = 1; object_id < objects.len; object_id++) {
        auto &object = getObject(object_id);
        if (!object.alive)
            continue;
        switch (object.kind) {
        case Object::Player: {
            auto &player = object;
            constexpr f32 player_speed = 300;
            if (pause) {
                if (app_state.isKeyPressed(SDL_SCANCODE_SPACE)) {
                    is_dialog = false;
                    pause = false;
                }
                continue;
            }
            Vector2 velocity = {};
            velocity.y = (f32)app_state.isKeyDown(SDL_SCANCODE_S) - (f32)app_state.isKeyDown(SDL_SCANCODE_W);
            velocity.x = (f32)app_state.isKeyDown(SDL_SCANCODE_D) - (f32)app_state.isKeyDown(SDL_SCANCODE_A);
            velocity = velocity.normalize();
            player.position += velocity * player_speed * app_state.dt;
            auto world_player_collision = player.calcAbsolutePositionOfRelativeRectangle(player.collision);
            for (auto static_object : objects) {
                auto world_static_object_collision = static_object.calcAbsolutePositionOfRelativeRectangle(static_object.collision);
                if (static_object.kind == Object::Static and world_player_collision.checkCollision(world_static_object_collision)) {
                    auto direction = world_player_collision.center() - world_static_object_collision.center();
                    auto overlap_size = world_player_collision.overlapSize(world_static_object_collision);
                    if (overlap_size.x < overlap_size.y) {
                        auto x_offset = (player.center.x - (player.collision.x + player.center.x)) * player.scale.x;
                        if (direction.x > 0) {
                            player.position.x = world_static_object_collision.x_max() + x_offset;
                        } else {
                            player.position.x = world_static_object_collision.x - world_player_collision.w + x_offset;
                        }
                    } else if (overlap_size.x > overlap_size.y) {
                        auto y_offset = (player.center.y - (player.collision.y + player.center.y)) * player.scale.y;
                        if (direction.y > 0) {
                            player.position.y = world_static_object_collision.y_max() + y_offset;
                        } else {
                            player.position.y = world_static_object_collision.y - world_player_collision.h + y_offset;
                        }
                    }
                }
                if (app_state.isKeyPressed(SDL_SCANCODE_SPACE)) {
                    auto world_static_object_interaction_arena = static_object.calcAbsolutePositionOfRelativeRectangle(static_object.interaction_arena);
                    if (static_object.kind == Object::Static and world_player_collision.checkCollision(world_static_object_interaction_arena)) {
                        is_dialog = true;
                        pause = true;
                    }
                }
            }

            camera_target = player.position;
            break;
        }
        case Object::Static: {
            break;
        }
        case Object::Enemy: {
            Object &enemy = object;
            if (enemy.inactive) {
                auto end = enemy.inactive_start + (1000 * 10);
                if (ticks > end) {
                    enemy.inactive = false;
                }
            }
            if (battle_mode or enemy.inactive)
                continue;
            Object &player = getObject(player_id);
            auto world_enemy_interaction_arena = enemy.calcAbsolutePositionOfRelativeRectangle(enemy.interaction_arena);
            auto player_collision = player.calcAbsolutePositionOfRelativeRectangle(player.collision);

            if (world_enemy_interaction_arena.checkCollision(player_collision)) {
                current_enemy_id = object_id;
                battle_mode = true;
                pause = true;
            }
            break;
        }
        case Object::Zero: {
            break;
        }
        }
    }

    render_order.sort([this](const ObjectId &a_id, const ObjectId &b_id) {
        if (a_id == 0 or b_id == 0)
            return true;
        auto &a = this->getObject(a_id);
        auto &b = this->getObject(b_id);
        return a.position.y > b.position.y;
    });
};

SDL_AppResult SDL_AppInit(void **app_state, [[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("Tower", "0.3.0", "cynumini.tower");
    SDL_ENSURE(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD), "initialize SDL");
    *app_state = new AppState();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *app_state) { return ((AppState *)app_state)->iterate(); }

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *app_state, [[maybe_unused]] SDL_Event *event) { return ((AppState *)app_state)->event(*event); }

void SDL_AppQuit(void *app_state, [[maybe_unused]] SDL_AppResult result) { delete (AppState *)app_state; }
