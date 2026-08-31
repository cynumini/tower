#include <sakana/sakana.hpp>

#include "math.cpp"

typedef SDL_FColor Color;

const Color WHITE = {1, 1, 1, 1};
const Color RED = {1, 0.5, 0.5, 1};

static SDL_GPUBuffer *createGPUBuffer(SDL_GPUDevice *device, SDL_GPUBufferUsageFlags usage,
                                      Uint32 size) {
    const SDL_GPUBufferCreateInfo buffer_create_info = {usage, size, 0};
    auto *buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
    SDL_assert(buffer);
    return buffer;
};

static SDL_GPUShader *createGPUShader(SDL_GPUDevice *device, const char *file,
                                      SDL_GPUShaderStage stage, Uint32 num_samplers,
                                      Uint32 num_uniform_buffers) {
    size_t code_size = 0;
    auto *code = (Uint8 *)SDL_LoadFile(file, &code_size);
    SDL_assert(code);
    SDL_GPUShaderCreateInfo create_info = {};
    create_info.code_size = code_size;
    create_info.code = code;
    create_info.entrypoint = "main";
    create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    create_info.stage = stage;
    create_info.num_samplers = num_samplers;
    create_info.num_uniform_buffers = num_uniform_buffers;
    auto *shader = SDL_CreateGPUShader(device, &create_info);
    SDL_assert(shader);
    SDL_free(code);
    return shader;
};

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

struct Instance {
    vec2 position;
    vec2 size;
    Rect uv;
    Color color;
    float rotation;
};

void addInstance(Instance *instances, Uint32 *count, Instance instance, vec2 texture_size) {
    instance.uv /= texture_size;
    instances[*count] = instance;
    *count += 1;
};

i32 main() {
    App app = sakanaInit("tower", "0.3.0", "cynumini.tower");
    defer(sakanaDeinit(app));

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(app.device);
    SDL_assert(command_buffer);
    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    SDL_GPUTexture *texture = 0;
    vec2 texture_size;
    {
        int width = 0;
        int height = 0;
        texture = IMG_LoadGPUTexture(app.device, copy_pass, "resources/world.png", &width, &height);
        texture_size = {float(width), float(height)};
    }
    defer(SDL_ReleaseGPUTexture(app.device, texture));
    SDL_assert(texture);

    vec2 vertices[4] = {{-0.5, -0.5}, {0.5, -0.5}, {0.5, 0.5}, {-0.5, 0.5}};
    const Uint16 indices[6]{0, 1, 2, 0, 2, 3};

    const SDL_GPUSamplerCreateInfo sampler_create_info = {};
    auto *sampler = SDL_CreateGPUSampler(app.device, &sampler_create_info);
    defer(SDL_ReleaseGPUSampler(app.device, sampler));

    const Uint32 INSTANCE_CAPACITY = 1U << 4U; // 2^4 = 16

    auto *vertex_buffer = createGPUBuffer(app.device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertices));
    defer(SDL_ReleaseGPUBuffer(app.device, vertex_buffer));
    auto *index_buffer = createGPUBuffer(app.device, SDL_GPU_BUFFERUSAGE_INDEX, sizeof(indices));
    defer(SDL_ReleaseGPUBuffer(app.device, index_buffer));
    auto *instance_buffer =
        createGPUBuffer(app.device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(Instance) * INSTANCE_CAPACITY);
    defer(SDL_ReleaseGPUBuffer(app.device, instance_buffer));

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {};
    transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buffer_create_info.size = sizeof(vertices) + sizeof(indices);
    auto *transfer_buffer = SDL_CreateGPUTransferBuffer(app.device, &transfer_buffer_create_info);
    SDL_assert(transfer_buffer);
    auto *transfer_buffer_data =
        (Uint8 *)SDL_MapGPUTransferBuffer(app.device, transfer_buffer, false);
    SDL_assert(transfer_buffer_data);
    SDL_memcpy(transfer_buffer_data, (Uint8 *)vertices, sizeof(vertices));
    SDL_memcpy(transfer_buffer_data + sizeof(vertices), (Uint8 *)indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(app.device, transfer_buffer);

    SDL_GPUTransferBufferLocation source{transfer_buffer, 0};
    SDL_GPUBufferRegion destination = {vertex_buffer, 0, sizeof(vertices)};
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);

    source.offset = sizeof(vertices);
    destination.buffer = index_buffer;
    destination.size = sizeof(indices);
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_ReleaseGPUTransferBuffer(app.device, transfer_buffer);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    auto *vertex_shader =
        createGPUShader(app.device, "shader.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    auto *fragment_shader =
        createGPUShader(app.device, "shader.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.vertex_shader = vertex_shader;
    pipeline_create_info.fragment_shader = fragment_shader;
    const SDL_GPUVertexBufferDescription vertex_buffer_descriptions[2] = {
        {0, sizeof(vec2), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0},
        {1, sizeof(Instance), SDL_GPU_VERTEXINPUTRATE_INSTANCE, 0}};
    pipeline_create_info.vertex_input_state.vertex_buffer_descriptions =
        (SDL_GPUVertexBufferDescription *)vertex_buffer_descriptions;
    pipeline_create_info.vertex_input_state.num_vertex_buffers =
        SDL_arraysize(vertex_buffer_descriptions);
    const SDL_GPUVertexAttribute vertex_attributes[] = {
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
        {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, position)},
        {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, size)},
        {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, uv)},
        {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, uv) + sizeof(vec2)},
        {5, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(Instance, color)},
        {6, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(Instance, rotation)},

    };

    pipeline_create_info.vertex_input_state.vertex_attributes =
        (SDL_GPUVertexAttribute *)vertex_attributes;
    pipeline_create_info.vertex_input_state.num_vertex_attributes =
        SDL_arraysize(vertex_attributes);
    pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    SDL_GPUColorTargetDescription color_target_description = {};
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(app.device, app.window);
    color_target_description.blend_state.enable_blend = true;

    color_target_description.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_target_description.blend_state.dst_color_blendfactor =
        SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target_description.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;

    color_target_description.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color_target_description.blend_state.dst_alpha_blendfactor =
        SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target_description.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    pipeline_create_info.target_info.color_target_descriptions = &color_target_description;
    pipeline_create_info.target_info.num_color_targets = 1;
    auto *pipeline = SDL_CreateGPUGraphicsPipeline(app.device, &pipeline_create_info);
    defer(SDL_ReleaseGPUGraphicsPipeline(app.device, pipeline));

    SDL_ReleaseGPUShader(app.device, vertex_shader);
    SDL_ReleaseGPUShader(app.device, fragment_shader);

    const auto *keyboard_state = SDL_GetKeyboardState(0);

    auto previous = SDL_GetTicks();

    vec2 player_position = {app.screen.x / 2.0F, app.screen.y / 2.0F};
    const vec2 PLAYER_SIZE = {16.0F, 32.0F};

    vec2 direction = {0, 1};

    auto walking_timer = timerInit(0.2F);
    int walking_frame = 0;

    bool attack = false;
    auto attack_timer = timerInit(0.1);

    vec2 atlas_offset = {0.0F, 32.0F};
    bool flip_x = false;

    const float ENEMY_KNOCKBACK_MAX_SPEED = 100;
    const float ENEMY_KNOCKBACK_FRICTION = 250;

    const size_t ENEMY_COUNT = 10;
    struct Enemy {
        vec2 position;
        vec2 size = {16.0F, 32.0F};
        Color tint = WHITE;
        int hp = 5;
        bool invincible = false;
        Timer invincibility_timer = timerInit(0.2F);
        vec2 knockback_direction = {};
        float knockback_speed = 0.0F;
    } enemies[ENEMY_COUNT] = {};

    {
        SDL_Time ticks; // NOLINT
        SDL_assert(SDL_GetCurrentTime(&ticks));
        SDL_srand(ticks);
    }

    for (size_t i = 0; i < ENEMY_COUNT; i++) {
        enemies[i].position.x = float(SDL_rand(Sint32(app.screen.x)));
        enemies[i].position.y = float(SDL_rand(Sint32(app.screen.y)));
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT: {
                running = false;
            } break;
            case SDL_EVENT_KEY_DOWN: {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    running = false;
                }
                if (event.key.scancode == SDL_SCANCODE_SPACE) {
                    attack = true;
                }
            } break;
            default: {
            } break;
            }
        }

        auto current = SDL_GetTicks();
        const float dt = float(current - previous) / 1000.F;
        previous = current;

        vec2 velocity{
            float(keyboard_state[SDL_SCANCODE_D]) - float(keyboard_state[SDL_SCANCODE_A]),
            float(keyboard_state[SDL_SCANCODE_S]) - float(keyboard_state[SDL_SCANCODE_W])};

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
                if (checkCollisonSAT(rectFromVec2(attack_position, attack_size), attack_angle,
                                     rectFromVec2(enemies[i].position, enemies[i].size), 0) and
                    !enemies[i].invincible) {
                    enemies[i].knockback_direction = direction;
                    enemies[i].knockback_speed = ENEMY_KNOCKBACK_MAX_SPEED;
                    enemies[i].hp -= 1;
                    enemies[i].invincible = true;
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

        // if (collision) {
        //     player.x = prev.x;
        //     player.y = prev.y;
        // } else {
        //     prev.x = player.x;
        //     prev.y = player.y;
        // }

        // pre draw
        command_buffer = SDL_AcquireGPUCommandBuffer(app.device);
        SDL_assert(command_buffer);

        auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {};
        transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_buffer_create_info.size = sizeof(Instance) * INSTANCE_CAPACITY;
        auto *transfer_buffer = SDL_CreateGPUTransferBuffer(app.device, &transfer_buffer_create_info);
        SDL_assert(transfer_buffer);

        auto *transfer_buffer_data =
            (Instance *)SDL_MapGPUTransferBuffer(app.device, transfer_buffer, true);
        SDL_assert(transfer_buffer_data);

        Uint32 instance_count = 0;

        // filling instance
        if (flip_x) {
            uv.x += uv.w;
            uv.w *= -1;
        }
        addInstance(transfer_buffer_data, &instance_count,
                    {player_position, PLAYER_SIZE, uv, WHITE, 0}, texture_size);

        for (size_t i = 0; i < ENEMY_COUNT; i++) {
            if (enemies[i].hp > 0) {
                addInstance(transfer_buffer_data, &instance_count,
                            {enemies[i].position,
                             enemies[i].size,
                             {0.0F, 0.0F, 16.0F, 32.0F},
                             enemies[i].tint,
                             0},
                            texture_size);
            }
        }

        SDL_qsort(transfer_buffer_data, instance_count, sizeof(Instance),
                  [](const void *a, const void *b) -> int {
                      const auto *A = (const Instance *)a;
                      const auto *B = (const Instance *)b;
                      if (A->position.y < B->position.y) return -1;
                      if (B->position.y < A->position.y) return 1;
                      return 0;
                  });

        if (attack) {
            addInstance(
                transfer_buffer_data, &instance_count,
                {attack_position, attack_size, {96.0F, 0.0F, 16.0F, 32.0F}, WHITE, attack_angle},
                texture_size);
        }

        SDL_assert(instance_count < INSTANCE_CAPACITY);

        SDL_UnmapGPUTransferBuffer(app.device, transfer_buffer);

        const SDL_GPUTransferBufferLocation source{transfer_buffer, 0};
        const SDL_GPUBufferRegion destination = {instance_buffer, 0,
                                                 sizeof(Instance) * INSTANCE_CAPACITY};
        SDL_UploadToGPUBuffer(copy_pass, &source, &destination, true);

        SDL_EndGPUCopyPass(copy_pass);

        SDL_ReleaseGPUTransferBuffer(app.device, transfer_buffer);

        // draw

        SDL_GPUTexture *swapchain_texture = 0;

        SDL_assert(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, app.window,
                                                         &swapchain_texture, 0, 0));

        if (swapchain_texture != 0) {
            SDL_GPUColorTargetInfo color_target_info = {};
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = {0.5, 0.5, 0.5, 1};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;
            auto *render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, 0);
            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
            SDL_GPUBufferBinding buffer_binding = {vertex_buffer, 0};
            SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);
            buffer_binding.buffer = instance_buffer;
            SDL_BindGPUVertexBuffers(render_pass, 1, &buffer_binding, 1);
            buffer_binding.buffer = index_buffer;
            SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_assert(texture);
            SDL_assert(sampler);
            const SDL_GPUTextureSamplerBinding texture_sampler_binding = {texture, sampler};
            SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
            struct UBO {
                vec2 screen;
                vec2 camera;
            } ubo;
            ubo.screen = app.screen;
            ubo.camera = player_position;
            SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));
            SDL_DrawGPUIndexedPrimitives(render_pass, 6, instance_count, 0, 0, 0);

            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    return 0;
}
