#include <skn_sdl.cpp>
// -- clear line
#include "unagi.hpp"

static Arena arena;
static Engine engine;

static SDL_Window *window;
static SDL_GPUDevice *device;
static SDL_GPUSampler *sampler;
static SDL_GPUGraphicsPipeline *pipeline;

static SDL_GPUBuffer *vertex_buffer;
static SDL_GPUBuffer *index_buffer;
static SDL_GPUBuffer *instance_buffer;

static Texture atlas;
// -- clear line

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#include "build/shader.frag.hpp"
#include "build/shader.vert.hpp"

#include "game.hpp"

const uint MAX_INSTANCES = 16;

struct State {

    SDL_GPUTransferBuffer *instance_transfer_buffer;

    HashMap<Rect> sprites;

    u64 counter;
    u64 frames;
    float seconds;
    float frequency;

    GameState game_state;
};

static State state = {};

SDL_AppResult SDL_AppInit([[maybe_unused]] void **appstate, [[maybe_unused]] int argc,
                          [[maybe_unused]] char *argv[]) {
    arena.init(sdl_allocator, KB(7));
    const char *name = "tower";
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata(name, "0.3.0", "cynumini.tower");

    SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));

    engine.screen = {640, 360};
    window = SDL_CreateWindow(name, engine.screen.x, engine.screen.y, 0);
    SDL_CHECK(window);

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, 0);
    SDL_CHECK(device);

    SDL_CHECK(SDL_ClaimWindowForGPUDevice(device, window));

    {
        const SDL_GPUSamplerCreateInfo createinfo{};
        sampler = SDL_CreateGPUSampler(device, &createinfo);
    }
    SDL_CHECK(sampler);

    SDL_CHECK(Texture::create(device, {4096, 4096}, &atlas));

    {
        SDL_GPUGraphicsPipelineCreateInfo createinfo = {};

        createinfo.vertex_shader =
            createGPUShader(device, shader_vert_code, SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        defer(SDL_ReleaseGPUShader(device, createinfo.vertex_shader));
        SDL_CHECK(createinfo.vertex_shader);

        createinfo.fragment_shader =
            createGPUShader(device, shader_frag_code, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
        defer(SDL_ReleaseGPUShader(device, createinfo.fragment_shader));
        SDL_CHECK(createinfo.fragment_shader);

        const SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
            {0, sizeof(vec2), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0},
            {1, sizeof(Instance), SDL_GPU_VERTEXINPUTRATE_INSTANCE, 0}};
        createinfo.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
        createinfo.vertex_input_state.num_vertex_buffers = ARRAY_LEN(vertex_buffer_descriptions);

        SDL_GPUVertexAttribute vertex_attributes[] = {
            // vertex
            {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
            // instance
            {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, position)},
            {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, size)},
            {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, uv)},
            {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, uv) + sizeof(vec2)},
            {5, 1, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(Instance, color)},
            {6, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(Instance, rotation)}};
        createinfo.vertex_input_state.vertex_attributes = vertex_attributes;
        createinfo.vertex_input_state.num_vertex_attributes = ARRAY_LEN(vertex_attributes);

        const SDL_GPUColorTargetDescription color_target_description = {
            .format = SDL_GetGPUSwapchainTextureFormat(device, window),
            .blend_state = {
                .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .color_blend_op = SDL_GPU_BLENDOP_ADD,
                .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                .enable_blend = true,

            }};
        createinfo.target_info.color_target_descriptions = &color_target_description;
        createinfo.target_info.num_color_targets = 1;
        pipeline = SDL_CreateGPUGraphicsPipeline(device, &createinfo);
    }

    SDL_CHECK(pipeline);

    vec2 vertices[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    const uint VERTEX_BUFFER_SIZE = sizeof(vertices);

    i16 indices[6]{0, 1, 2, 0, 2, 3};
    const uint INDEX_BUFFER_SIZE = sizeof(indices);

    vertex_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, VERTEX_BUFFER_SIZE);
    SDL_CHECK(vertex_buffer);

    index_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_INDEX, INDEX_BUFFER_SIZE);
    SDL_CHECK(index_buffer);

    instance_buffer =
        createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(Instance) * MAX_INSTANCES);
    SDL_CHECK(instance_buffer);

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    defer(SDL_SubmitGPUCommandBuffer(command_buffer));
    SDL_CHECK(command_buffer);

    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    defer(SDL_EndGPUCopyPass(copy_pass));

    {
        auto *transfer_buffer =
            createGPUTransferBuffer(device, VERTEX_BUFFER_SIZE + INDEX_BUFFER_SIZE);
        SDL_CHECK(transfer_buffer);
        defer(SDL_ReleaseGPUTransferBuffer(device, transfer_buffer));

        {
            u8 *memory = (u8 *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
            defer(SDL_UnmapGPUTransferBuffer(device, transfer_buffer));
            SDL_CHECK(memory);

            SDL_memcpy(memory, vertices, VERTEX_BUFFER_SIZE);
            SDL_memcpy(memory + VERTEX_BUFFER_SIZE, indices, INDEX_BUFFER_SIZE);
        }
        uploadToGPUBuffer(copy_pass, transfer_buffer, 0, vertex_buffer, VERTEX_BUFFER_SIZE);
        uploadToGPUBuffer(copy_pass, transfer_buffer, VERTEX_BUFFER_SIZE, index_buffer,
                          INDEX_BUFFER_SIZE);
    }

    {
        ScopeArena scope(&arena);
        auto scratch = scope.tmp.allocator();

        auto files = globDirectory("resources/", "*.png", SDL_GLOB_CASEINSENSITIVE);
        defer(sdl_allocator.free(files.ptr));
        SDL_assert(files.len);

        Dictionary<SDL_Surface *> files_dict = {};
        defer(for (auto &file : files_dict) SDL_DestroySurface(file.value));

        for (auto &file : files) {
            auto path = scratch.allocFormatZ("resources/%s", file);

            auto *surface = SDL_LoadPNG(path.ptr);
            SDL_CHECK(surface);

            scratch.free(path);

            if (surface->format != SDL_PIXELFORMAT_RGBA32) {
                SDL_Surface *old_surface = surface;
                surface = SDL_ConvertSurface(old_surface, SDL_PIXELFORMAT_RGBA32);
                SDL_DestroySurface(old_surface);
                SDL_CHECK(surface);
            }

            files_dict.put(&scratch, file, surface);
        }

        files_dict.sort([](const void *a, const void *b) {
            using Item = Dictionary<SDL_Surface *>::Item;
            auto *s_a = (Item *)a;
            auto *s_b = (Item *)b;
            if (s_a->value->h > s_b->value->h) return -1;
            if (s_a->value->h < s_b->value->h) return 1;
            return 0;
        });

        uint x_offset = 0;
        uint y_offset = 0;
        uint y_max = 0;
        for (auto &file : files_dict) {
            auto *surface = file.value;
            auto *transfer_buffer =
                createGPUTransferBuffer(device, sizeof(Color) * surface->w * surface->h);
            defer(SDL_ReleaseGPUTransferBuffer(device, transfer_buffer));
            SDL_CHECK(transfer_buffer);

            {
                u8 *memory = (u8 *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
                defer(SDL_UnmapGPUTransferBuffer(device, transfer_buffer));
                SDL_CHECK(memory);

                const auto *src = (u8 *)surface->pixels;
                const auto row_bytes = size_t(surface->w) * 4;
                if (row_bytes == size_t(surface->pitch)) {
                    SDL_memcpy(memory, src, row_bytes * surface->h);
                } else {
                    for (size_t y = 0; y < size_t(surface->h); y++) {
                        SDL_memcpy(memory + (y * row_bytes), src + (y * surface->pitch),
                                   row_bytes);
                    }
                }
            }
            if ((x_offset + surface->w) > uint(atlas.size.x)) {
                x_offset = 0;
                y_offset += y_max;
                y_max = 0;
            }
            SDL_assert(x_offset + uint(surface->w) <= uint(atlas.size.x));
            SDL_assert(y_offset + uint(surface->h) <= uint(atlas.size.y));

            state.sprites.put(
                &scratch, scratch.dupeZ(getStem(file.key)).ptr,
                {float(x_offset), float(y_offset), float(surface->w), float(surface->h)});

            y_max = max(uint(surface->h), y_max);

            atlas.uploadToGPU(copy_pass, transfer_buffer,
                              {x_offset, y_offset, (uint)surface->w, (uint)surface->h});

            x_offset += surface->w;
        }
        auto global_allocator = arena.allocator();
        state.sprites = state.sprites.copy(&global_allocator);
    }

    state.instance_transfer_buffer =
        createGPUTransferBuffer(device, sizeof(Instance) * MAX_INSTANCES);
    SDL_CHECK(state.instance_transfer_buffer);

    state.frequency = float(SDL_GetPerformanceFrequency());
    state.counter = SDL_GetPerformanceCounter();

    gameInit(&state.game_state, &engine, state.sprites);

    engine.keyboard_state = SDL_GetKeyboardState(0);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event) {
    memset(&engine.key_down, 0, sizeof(engine.key_down));

    switch (event->type) {
    case SDL_EVENT_QUIT: {
        return SDL_APP_SUCCESS;
    }
    case SDL_EVENT_KEY_DOWN: {
        engine.key_down[event->key.scancode] = true;
        break;
    }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *appstate) {
    ScopeArena scope(&arena);
    auto frame = scope.tmp.allocator();

    auto prev = state.counter;
    state.counter = SDL_GetPerformanceCounter();
    state.frames += 1;
    engine.dt = float(state.counter - prev) / state.frequency;
    state.seconds += engine.dt;

    if (state.seconds > 0.5F) {
        engine.fps = (float)state.frames / state.seconds;
        state.frames = 0;
        state.seconds = 0;
    }

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    defer(SDL_SubmitGPUCommandBuffer(command_buffer));
    SDL_CHECK(command_buffer);

    GameResult game_update_result = {};
    {
        auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        defer(SDL_EndGPUCopyPass(copy_pass));
        {
            Instance *instances_raw = (Instance *)SDL_MapGPUTransferBuffer(
                device, state.instance_transfer_buffer, true);
            SDL_CHECK(instances_raw);

            game_update_result =
                gameUpdate(&state.game_state, &engine, &frame,
                           {.items = {instances_raw, MAX_INSTANCES}}, state.sprites);

            // SDL_Log("%d\n", game_update_result.quit);
            if (game_update_result.quit) return SDL_APP_SUCCESS;

            SDL_assert(game_update_result.instances_len <= MAX_INSTANCES);

            SDL_UnmapGPUTransferBuffer(device, state.instance_transfer_buffer);
        }

        if (game_update_result.instances_len) {
            uploadToGPUBuffer(copy_pass, state.instance_transfer_buffer, 0, instance_buffer,
                              sizeof(Instance) * game_update_result.instances_len);
        }
    }

    SDL_GPUTexture *swapchain_texture = 0;

    SDL_CHECK(
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, 0, 0));

    if (swapchain_texture) {
        SDL_GPUColorTargetInfo color_target_info = {};
        color_target_info.texture = swapchain_texture;
        const FColor clear_color = toFColor(GRAY);
        color_target_info.clear_color.r = clear_color.r;
        color_target_info.clear_color.g = clear_color.g;
        color_target_info.clear_color.b = clear_color.b;
        color_target_info.clear_color.a = clear_color.a;
        color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target_info.store_op = SDL_GPU_STOREOP_STORE;
        auto *render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, 0);

        SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
        SDL_GPUBufferBinding buffer_bindings[2] = {{vertex_buffer, 0}, {instance_buffer, 0}};
        SDL_BindGPUVertexBuffers(render_pass, 0, buffer_bindings, 2);
        const SDL_GPUBufferBinding buffer_binding = {index_buffer, 0};
        SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        const SDL_GPUTextureSamplerBinding texture_sampler_binding = {.texture = atlas.ptr,
                                                                      .sampler = sampler};
        SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
        struct UBO {
            ivec2 screen;
            vec2 camera;
        } ubo;
        ubo.screen = engine.screen;
        ubo.camera = game_update_result.camera;
        SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));

        SDL_DrawGPUIndexedPrimitives(render_pass, 6, game_update_result.ui_instance_offset, 0, 0,
                                     0);
        ubo.camera = {};
        SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));
        SDL_DrawGPUIndexedPrimitives(render_pass, 6,
                                     game_update_result.instances_len -
                                         game_update_result.ui_instance_offset,
                                     0, 0, game_update_result.ui_instance_offset);

        SDL_EndGPURenderPass(render_pass);
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit([[maybe_unused]] void *appstate, [[maybe_unused]] SDL_AppResult result) {

    SDL_ReleaseGPUTransferBuffer(device, state.instance_transfer_buffer);

    SDL_ReleaseGPUTexture(device, atlas.ptr);

    SDL_ReleaseGPUBuffer(device, instance_buffer);
    SDL_ReleaseGPUBuffer(device, index_buffer);
    SDL_ReleaseGPUBuffer(device, vertex_buffer);

    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseGPUSampler(device, sampler);
    SDL_ReleaseWindowFromGPUDevice(device, window);

    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);

    sdl_allocator.free(arena.mem);
}

void unagiLog(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
    va_end(args);
}
