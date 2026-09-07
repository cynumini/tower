#include <unagi.cpp>

#include "math.cpp"

#include "../build/shader.frag.hpp"
#include "../build/shader.vert.hpp"

struct Timer {
    float elapsed = 0;
    float duration;
};

static Timer timerInit(float duration) { return {.duration = duration}; };
static void timerReset(Timer *timer) { timer->elapsed = 0; };
static bool advanceTimerAndCheck(Timer *timer, float dt) {
    timer->elapsed += dt;
    if (timer->elapsed >= timer->duration) {
        timerReset(timer);
        return true;
    }
    return false;
};

const Uint32 INSTANCE_CAPACITY = 1U << 9U; // 2^8 = 512

struct Instance {
    Rect rect;
    Rect uv;
    FColor color;
    float rotation;
    u32 texture_index;
};

struct Renderer {
    Instance *instances;
    Pipeline *pipeline;
    Uint32 count;
};

void addInstance(Renderer *renderer, Rect rect, Rect uv, FColor color, f32 rotation,
                 Texture texture) {
    SDL_assert(renderer->count < INSTANCE_CAPACITY);
    renderer->instances[renderer->count] = {
        .rect = rect,
        .uv = uv / texture.size,
        .color = color,
        .rotation = rotation,
        .texture_index = bindTexture(renderer->pipeline, texture),
    };
    renderer->count += 1;
}

Texture loadTexture(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass, const char *filename) {
    Texture texture = {};
    int width = 0;
    int height = 0;
    texture.ptr = IMG_LoadGPUTexture(device, copy_pass, filename, &width, &height);
    texture.size = {float(width), float(height)};
    SDL_assert(texture.ptr);
    return texture;
};

const f32 FONT_SIZE = 10;

struct Font {
    u8 widths[256];
    Texture texture;
};

struct InvertorySlot {
    u8 item_id;
    u8 count;
};

Font initFont(Texture texture) {
    Font font{};
    for (usize i = 0; i < 256; i++) {
        font.widths[i] = 4;
    }
    font.widths['0'] = 5;
    font.widths['2'] = 5;
    font.widths['6'] = 5;
    font.widths['8'] = 5;
    font.widths['9'] = 5;
    font.widths['?'] = 5;
    font.widths['M'] = 7;
    font.widths['x'] = 5;
    font.widths['y'] = 5;
    font.texture = texture;
    return font;
}

Slice<const char> createString(const char *c_str) {
    return {.ptr = c_str, .len = SDL_strlen(c_str)};
}

f32 measureText(Slice<const char> text, Font font) {
    if (text.len == 0) return 0;
    f32 advance = 0;
    for (usize i = 0; i < text.len; i++) advance += f32(font.widths[u8(text.ptr[i])]) + 1;
    return advance - 1;
}

void drawText(Renderer *renderer, Slice<const char> text, Font font, vec2 position) {
    f32 advance = 0;
    for (usize i = 0; i < text.len; i++) {
        vec2 texture_offset = {0, 0};
        const u8 c = text.ptr[i] - ' ';
        texture_offset = {
            .x = f32(c % 16) * 10.0F,
            .y = f32(c / 16) * 10.0F, // NOLINT
        };
        addInstance(renderer, {position.x + advance, position.y, 10.0F, 10.0F},
                    {texture_offset.x, texture_offset.y, 10.0F, 10.0F}, BLACK, 0.0F,
                    font.texture);
        advance += f32(font.widths[u8(text.ptr[i])]) + 1;
    }
}

const f32 ENEMY_KNOCKBACK_MAX_SPEED = 100;
const f32 ENEMY_KNOCKBACK_FRICTION = 250;
const vec2 ENEMY_SIZE = {16.0F, 32.0F};

struct Enemy {
    vec2 position;
    FColor tint = WHITE;
    int hp = 5;
    bool invincible = false;
    Timer invincibility_timer = timerInit(0.2F);
    vec2 knockback_direction = {};
    float knockback_speed = 0.0F;
};

void enemy_take_damage(Enemy *enemy, vec2 direction, InvertorySlot *invertory) {
    enemy->knockback_direction = direction;
    enemy->knockback_speed = ENEMY_KNOCKBACK_MAX_SPEED;
    enemy->hp -= 1;
    enemy->invincible = true;
    if (enemy->hp == 0) {
        invertory[0].count += 1;
    }
}

i32 main() {
    // TODO do a refactoring (inlcluding compression and decompression)
    App app = unagiInit("tower", "0.3.0", "cynumini.tower");
    defer(unagiDeinit(app));

    char buffer_raw[1U << 6U] = {}; // 2 ^ 6 = 64
    const Slice<char> buffer = {.ptr = buffer_raw, .len = SDL_arraysize(buffer_raw)};

    const SDL_GPUVertexAttribute vertex_attributes[] = {
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
        {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, rect)},
        {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, rect) + sizeof(vec2)},
        {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, uv)},
        {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, uv) + sizeof(vec2)},
        {5, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(Instance, color)},
        {6, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(Instance, rotation)},
        {7, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, offsetof(Instance, texture_index)},
    };

    auto pipeline =
        createPipeline(app, sizeof(Instance), INSTANCE_CAPACITY, shader_vert_code,
                       shader_frag_code, vertex_attributes, SDL_arraysize(vertex_attributes),
                       SDL_GetGPUSwapchainTextureFormat(app.device, app.window));
    defer(destroyPipeline(pipeline, app.device));

    Texture texture;
    defer(SDL_ReleaseGPUTexture(app.device, texture.ptr));

    Texture font_texture;
    defer(SDL_ReleaseGPUTexture(app.device, font_texture.ptr));
    {
        auto *command_buffer = SDL_AcquireGPUCommandBuffer(app.device);
        defer(SDL_SubmitGPUCommandBuffer(command_buffer));
        SDL_assert(command_buffer);

        auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        defer(SDL_EndGPUCopyPass(copy_pass));

        unagiUpload(app, copy_pass);

        texture = loadTexture(app.device, copy_pass, "resources/world.png");
        font_texture = loadTexture(app.device, copy_pass, "resources/font.png");

        vec2 vertices[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        uploadPipeline(pipeline, app.device, copy_pass, vertices);
    }

    const auto *keyboard_state = SDL_GetKeyboardState(0);

    auto previous = SDL_GetTicks();

    vec2 player_position = {0.0F, 0.0F};
    const vec2 PLAYER_SIZE = {16.0F, 32.0F};

    vec2 direction = {0, 1};

    auto walking_timer = timerInit(0.2F);
    int walking_frame = 0;

    bool attack = false;
    auto attack_timer = timerInit(0.1);

    vec2 atlas_offset = {0.0F, 32.0F};
    bool flip_x = false;

    const size_t ENEMY_COUNT = 100;
    Enemy enemies[ENEMY_COUNT] = {};

    {
        SDL_Time ticks; // NOLINT
        SDL_assert(SDL_GetCurrentTime(&ticks));
        SDL_srand(ticks);
    }

    const i32 MAX_X = 1000;
    const i32 MAX_Y = 1000;
    for (size_t i = 0; i < ENEMY_COUNT; i++) {
        enemies[i].position = {f32(SDL_rand(MAX_X)) - (MAX_X / 2.0F),
                               f32(SDL_rand(MAX_Y)) - (MAX_Y / 2.0F)};
    }

    const Font font = initFont(font_texture);

    InvertorySlot invertory[8 * 8] = {};
    invertory[0] = InvertorySlot{1, 1};
    bool invertory_visible = false;
    bool cast_spell = false;

    vec2 spell_position = {};
    vec2 spell_direction = {};
    const f32 SPELL_SPEED = 100;
    const vec2 SPELL_SIZE = vec2{10.0F, 10.0F};
    bool spell_alive = false;

    const f32 frequency = f32(SDL_GetPerformanceFrequency());
    u64 counter = SDL_GetPerformanceCounter();
    u64 frames = 0;
    f32 seconds = 0.0F;
    f32 fps = 0.0F;
    while (app.running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT: {
                app.running = false;
            } break;
            case SDL_EVENT_KEY_DOWN: {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    app.running = false;
                }
                if (event.key.scancode == SDL_SCANCODE_SPACE) {
                    attack = true;
                }
                if (event.key.scancode == SDL_SCANCODE_E) {
                    invertory_visible = !invertory_visible;
                }
                if (event.key.scancode == SDL_SCANCODE_F) {
                    cast_spell = true;
                }
            } break;
            default: {
            } break;
            }
        }

        auto current = SDL_GetTicks();
        const float dt = float(current - previous) / 1000.F;
        previous = current;

        vec2 velocity{f32(keyboard_state[SDL_SCANCODE_D]) - f32(keyboard_state[SDL_SCANCODE_A]),
                      f32(keyboard_state[SDL_SCANCODE_S]) - f32(keyboard_state[SDL_SCANCODE_W])};

        velocity = normalizeVec2(velocity);
        auto length = vec2Length(velocity);

        if (length > 0.0F) {
            if (advanceTimerAndCheck(&walking_timer, dt)) {
                walking_frame += 1;
                walking_frame %= 4;
            }
            direction = velocity;
        } else {
            walking_frame = 0;
            timerReset(&walking_timer);
        }

        Rect uv = {0.0F, 32.0F, 16.0F, 32.0F};

        if (velocity.y < 0) {
            atlas_offset = {0.0F, 64.0F};
        } else if (velocity.y > 0) {
            atlas_offset = {0.0F, 32.0F};
        } else if (velocity.x > 0) {
            atlas_offset = {48.0F, 64.0F};
            flip_x = false;
        } else if (velocity.x < 0) {
            atlas_offset = {48.0F, 64.0F};
            flip_x = true;
        }

        if (walking_frame == 1) {
            uv.x = atlas_offset.x + 16.0F;
        } else if (walking_frame == 3) {
            uv.x = atlas_offset.x + 32.0F;
        } else {
            uv.x = atlas_offset.x + 0.0F;
        }

        uv.y = atlas_offset.y;

        const auto player_speed = 100.0F;
        player_position += velocity * dt * player_speed;

        vec2 attack_position = player_position;
        const vec2 attack_size = {16.0F, 32.0F};
        attack_position += direction * vec2{16.0F, 24.0F};
        auto attack_angle = SDL_atan2f(direction.y, direction.x);

        if (attack) {
            if (advanceTimerAndCheck(&attack_timer, dt)) attack = false;

            for (size_t i = 0; i < ENEMY_COUNT; i++) {
                if (enemies[i].hp <= 0) continue;
                if (checkCollisionSAT(rectFromVec2(attack_position, attack_size), attack_angle,
                                      rectFromVec2(enemies[i].position, ENEMY_SIZE), 0) and
                    !enemies[i].invincible) {
                    enemy_take_damage(&enemies[i], direction, invertory);
                }
            }
        }

        if (cast_spell) {
            spell_alive = true;
            spell_position = player_position;
            spell_position += PLAYER_SIZE / 2.0F - SPELL_SIZE / 2.0F;
            spell_direction = direction;
            cast_spell = false;
        }

        if (spell_alive) {
            spell_position += spell_direction * SPELL_SPEED * dt;
            for (size_t i = 0; i < ENEMY_COUNT; i++) {
                if (enemies[i].hp <= 0) continue;
                if (checkCollisionAABB(rectFromVec2(spell_position, SPELL_SIZE),
                                       rectFromVec2(enemies[i].position, ENEMY_SIZE)) and
                    !enemies[i].invincible) {
                    enemy_take_damage(&enemies[i], spell_direction, invertory);
                    spell_alive = false;
                }
            }
        }

        for (size_t i = 0; i < ENEMY_COUNT; i++) {
            if (enemies[i].knockback_speed > 0.0F) {
                enemies[i].position +=
                    enemies[i].knockback_direction * enemies[i].knockback_speed * dt;
                enemies[i].knockback_speed -= ENEMY_KNOCKBACK_FRICTION * dt;
            } else {
                enemies[i].knockback_speed = 0.0F;
            }

            if (enemies[i].invincible) {
                enemies[i].tint = RED;
                if (advanceTimerAndCheck(&enemies[i].invincibility_timer, dt)) {
                    enemies[i].invincible = false;
                    enemies[i].tint = WHITE;
                }
            }
        }

        // pre draw
        auto *command_buffer = SDL_AcquireGPUCommandBuffer(app.device);
        SDL_assert(command_buffer);

        Uint32 ui_instance_offset = 0;

        Renderer renderer = {
            .instances = (Instance *)beginUploadInstances(&pipeline, app.device),
            .pipeline = &pipeline,
            .count = 0,
        };

        {
            auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
            defer(SDL_EndGPUCopyPass(copy_pass));

            defer(endUploadInstances(&pipeline, app.device, copy_pass));

            if (flip_x) {
                uv.x += uv.w;
                uv.w *= -1;
            }
            addInstance(&renderer, rectFromVec2(player_position, PLAYER_SIZE), uv, WHITE, 0,
                        texture);

            for (size_t i = 0; i < ENEMY_COUNT; i++) {
                if (enemies[i].hp > 0) {
                    addInstance(&renderer, rectFromVec2(enemies[i].position, ENEMY_SIZE),
                                {0.0F, 0.0F, 16.0F, 32.0F}, enemies[i].tint, 0, texture);
                }
            }

            SDL_qsort(renderer.instances, renderer.count, sizeof(Instance),
                      [](const void *a, const void *b) -> int {
                          const auto *A = (const Instance *)a;
                          const auto *B = (const Instance *)b;
                          if (A->rect.y < B->rect.y) return -1;
                          if (B->rect.y < A->rect.y) return 1;
                          return 0;
                      });

            if (attack) {
                addInstance(&renderer, rectFromVec2(attack_position, attack_size),
                            {96.0F, 0.0F, 16.0F, 32.0F}, WHITE, attack_angle, texture);
            }

            if (spell_alive) {
                addInstance(&renderer, rectFromVec2(spell_position, SPELL_SIZE),
                            {19.0F, 19.0F, 10.0F, 10.0F}, WHITE, 0, texture);
            }

            ui_instance_offset = renderer.count;
            if (invertory_visible) {
                for (usize x_i = 0; x_i < 8; x_i++) {
                    for (usize y_i = 0; y_i < 8; y_i++) {
                        const vec2 cell_size = {24.0F, 24.0F};
                        const vec2 position = vec2{f32(x_i), f32(y_i)} * cell_size +
                                              (app.screen - (cell_size * 8.0F));

                        addInstance(&renderer, rectFromVec2(position, cell_size),
                                    {112.0F, 80.0F, 16.0F, 16.0F}, WHITE, 0, texture);

                        const auto *invertory_slot = &invertory[(y_i * 8) + x_i];
                        if (invertory_slot->item_id != 0) {
                            char buffer[3] = {0, 0, 0};
                            SDL_assert(invertory_slot->count);
                            SDL_assert(invertory_slot->count < 100);
                            SDL_snprintf(buffer, 3, "%d", invertory_slot->count);
                            const auto text = createString(buffer);
                            addInstance(&renderer, rectFromVec2(position, cell_size),
                                        {112.0F, 0.0F, 16.0F, 16.0F}, WHITE, 0.0F, texture);

                            const vec2 text_offset =
                                position + (cell_size - vec2(measureText(text, font), FONT_SIZE));
                            drawText(&renderer, text, font, text_offset);
                        }
                    }
                }
            }

            bufferPrint(buffer, "%.02f FPS", fps);
            drawText(&renderer, createString(buffer.ptr), font, {0, 0});
        }

        // draw
        SDL_GPUTexture *swapchain_texture = 0;

        SDL_assert(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, app.window,
                                                         &swapchain_texture, 0, 0));

        if (swapchain_texture != 0) {
            auto *render_pass = beginRenderPass(command_buffer, swapchain_texture);
            defer(SDL_EndGPURenderPass(render_pass));

            bindPipeline(pipeline, render_pass);

            struct UBO {
                vec2 screen;
                vec2 camera;
            } ubo;
            ubo.screen = app.screen;
            ubo.camera = -player_position + app.screen / 2.0 - PLAYER_SIZE / 2.0F;
            SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));
            SDL_DrawGPUIndexedPrimitives(render_pass, 6, ui_instance_offset, 0, 0, 0);
            ubo.camera = {};
            SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));
            SDL_DrawGPUIndexedPrimitives(render_pass, 6, renderer.count - ui_instance_offset, 0,
                                         0, ui_instance_offset);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);

        auto prev = counter;
        counter = SDL_GetPerformanceCounter();
        frames += 1, seconds += f32(counter - prev) / frequency;

        if (seconds > 0.5F) {
            fps = (f32)frames / seconds, frames = 0, seconds = 0;
        }
    }

    return 0;
}
