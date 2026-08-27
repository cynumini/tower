#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

typedef SDL_FPoint vec2;
typedef SDL_FRect Rect;

static void operator/=(Rect &self, vec2 other) {
    self.x /= other.x;
    self.y /= other.y;
    self.w /= other.x;
    self.h /= other.y;
}

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
}

static SDL_GPUBuffer *createGPUBuffer(SDL_GPUDevice *device, SDL_GPUBufferUsageFlags usage,
                                      Uint32 size) {
    const SDL_GPUBufferCreateInfo buffer_create_info = {usage, size, 0};
    auto *buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
    SDL_assert(buffer);
    return buffer;
}

int main() {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_assert(SDL_SetAppMetadata("tower", "0.3.0", "cynumini.tower"));
    SDL_assert(SDL_Init(SDL_INIT_VIDEO));

    const vec2 screen = {640, 360};
    auto *window = SDL_CreateWindow("tower", int(screen.x), int(screen.y), 0);
    SDL_assert(window);
    auto *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, 0);
    SDL_assert(device);
    SDL_assert(SDL_ClaimWindowForGPUDevice(device, window));

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_assert(command_buffer);
    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    SDL_GPUTexture *texture = 0;
    vec2 texture_size;
    {
        int width = 0;
        int height = 0;
        texture = IMG_LoadGPUTexture(device, copy_pass, "resources/world.png", &width, &height);
        texture_size = {float(width), float(height)};
    }
    SDL_assert(texture);

    vec2 vertices[4] = {{-0.5, -0.5}, {0.5, -0.5}, {0.5, 0.5}, {-0.5, 0.5}};
    const Uint16 indices[6]{0, 1, 2, 0, 2, 3};
    struct Instance {
        vec2 position;
        vec2 size;
        Rect uv;
        float rotation;
    };
    const SDL_GPUSamplerCreateInfo sampler_create_info = {};
    auto *sampler = SDL_CreateGPUSampler(device, &sampler_create_info);

    const Uint32 instance_capacity = 2;

    auto *vertex_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertices));
    auto *index_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_INDEX, sizeof(indices));
    auto *instance_buffer =
        createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(Instance) * instance_capacity);

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {};
    transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buffer_create_info.size = sizeof(vertices) + sizeof(indices);
    auto *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
    SDL_assert(transfer_buffer);
    auto *transfer_buffer_data =
        (Uint8 *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    SDL_assert(transfer_buffer_data);
    SDL_memcpy(transfer_buffer_data, (Uint8 *)vertices, sizeof(vertices));
    SDL_memcpy(transfer_buffer_data + sizeof(vertices), (Uint8 *)indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUTransferBufferLocation source{transfer_buffer, 0};
    SDL_GPUBufferRegion destination = {vertex_buffer, 0, sizeof(vertices)};
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);

    source.offset = sizeof(vertices);
    destination.buffer = index_buffer;
    destination.size = sizeof(indices);
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    auto *vertex_shader =
        createGPUShader(device, "shader.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    auto *fragment_shader =
        createGPUShader(device, "shader.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);

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
        {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
        {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(vec2)},
        {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(vec2) * 2},
        {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(vec2) * 3},
        {5, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, sizeof(vec2) * 4},

    };

    pipeline_create_info.vertex_input_state.vertex_attributes =
        (SDL_GPUVertexAttribute *)vertex_attributes;
    pipeline_create_info.vertex_input_state.num_vertex_attributes =
        SDL_arraysize(vertex_attributes);
    pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    SDL_GPUColorTargetDescription color_target_description = {};
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(device, window);
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
    auto *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);

    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

    const auto *keyboard_state = SDL_GetKeyboardState(0);

    auto previous = SDL_GetTicks();
    vec2 position = {screen.x / 2.0F, screen.y / 2.0F};

    vec2 direction = {0, 1};
    float walking_timer = 0.0F;
    int walking_frame = 0;

    bool attack = false;
    float attack_timer = 0.0F;

    vec2 atlas_offset = {0.0F, 32.0F};
    bool flip_x = false;

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

        const auto length = SDL_sqrtf((velocity.x * velocity.x) + (velocity.y * velocity.y));
        if (length > 0.0F) {
            velocity.x /= length;
            velocity.y /= length;
            walking_timer += dt;
            if (walking_timer > 0.2F) {
                walking_frame += 1;
                walking_frame %= 4;
                walking_timer = 0.0F;
            }
            direction = velocity;
        } else {
            walking_frame = 0;
            walking_timer = 0;
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
        position.x += velocity.x * dt * player_speed;
        position.y += velocity.y * dt * player_speed;

        vec2 attack_position = position;
        attack_position.x += direction.x * 16.0F;
        attack_position.y += direction.y * 24.0F;
        auto attack_angle = SDL_atan2f(direction.y, direction.x);

        if (attack) {
            attack_timer += dt;
            if (attack_timer > 0.1) {
                attack = false;
                attack_timer = 0;
            }
        }

        // pre draw
        command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_assert(command_buffer);

        auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {};
        transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_buffer_create_info.size = sizeof(Instance) * instance_capacity;
        auto *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
        SDL_assert(transfer_buffer);

        auto *transfer_buffer_data =
            (Instance *)SDL_MapGPUTransferBuffer(device, transfer_buffer, true);
        SDL_assert(transfer_buffer_data);

        // filling instance
        if (flip_x) {
            uv.x += uv.w;
            uv.w *= -1;
        }
        uv /= texture_size;
        transfer_buffer_data[0] = {position, {16.F, 32.F}, uv, 0};

        Uint32 instance_count = 1;
        if (attack) {
            Rect attack_uv = {96.0F, 0.0F, 16.0F, 32.0F};
            attack_uv /= texture_size;
            transfer_buffer_data[1] = {attack_position, {16.F, 32.F}, attack_uv, attack_angle};
            instance_count++;
        }

        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

        const SDL_GPUTransferBufferLocation source{transfer_buffer, 0};
        const SDL_GPUBufferRegion destination = {instance_buffer, 0,
                                                 sizeof(Instance) * instance_capacity};
        SDL_UploadToGPUBuffer(copy_pass, &source, &destination, true);

        SDL_EndGPUCopyPass(copy_pass);

        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

        // draw

        SDL_GPUTexture *swapchain_texture = 0;

        SDL_assert(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window,
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
            SDL_PushGPUVertexUniformData(command_buffer, 0, &screen, sizeof(screen));
            SDL_DrawGPUIndexedPrimitives(render_pass, 6, instance_count, 0, 0, 0);

            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    SDL_ReleaseGPUTexture(device, texture);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
    SDL_ReleaseGPUBuffer(device, index_buffer);
    SDL_ReleaseGPUBuffer(device, instance_buffer);
    SDL_ReleaseGPUSampler(device, sampler);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
