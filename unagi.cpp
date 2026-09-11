#include <stddef.h>

#include <skn_math.cpp>
#include <skn_sdl.cpp>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#include "build/shader.frag.hpp"
#include "build/shader.vert.hpp"

#include "game.hpp"

const uint MAX_INSTANCES = 1U << 2U; // 4

#define KB(value) ((value) * 1024)

Slice<char> allocFormatSentinel(Allocator *allocator, const char *format, ...) {
    va_list args;
    va_start(args, format);
    va_list args_copy;
    va_copy(args_copy, args);
    const int len = SDL_vsnprintf(0, 0, format, args_copy);
    va_end(args_copy);
    assert(len >= 0);
    auto memory = alloc<char>(allocator, len + 1);
    SDL_vsnprintf(memory.ptr, memory.len, format, args);
    va_end(args);
    return memory;
}

struct State {
    ivec2 screen;
    SDL_Window *window;
    SDL_GPUDevice *device;
    SDL_GPUSampler *sampler;
    SDL_GPUTexture *default_texture;
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    SDL_GPUBuffer *instance_buffer;
    Texture character_texture;
    Texture font_texture;
    Texture atlas;
    SDL_GPUTransferBuffer *instance_transfer_buffer;
    SDL_GPUTextureSamplerBinding texture_sampler_bindings[MAX_TEXTURE_SAMPLERS];
    DebugAllocator debug_allocator;
};

SDL_AppResult SDL_AppInit(void **appstate, [[maybe_unused]] i32 argc,
                          [[maybe_unused]] char *argv[]) {
    const char *name = "tower";
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata(name, "0.3.0", "cynumini.tower");
    SDL_CHECK(SDL_Init(SDL_INIT_VIDEO), "initialize SDL");
    auto *state = (State *)SDL_malloc(sizeof(State));
    // TODO: zeros on init by default
    *state = {.atlas = {.size = {4096, 4096}}};
    *appstate = state;
    state->debug_allocator = debugAllocatorInit(&sdl_allocator);
    Allocator *allocator = &state->debug_allocator.allocator;
    state->screen = {640, 360};
    state->window = SDL_CreateWindow(name, state->screen.x, state->screen.y, 0);
    SDL_CHECK(state->window, "create window");
    state->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, 0);
    SDL_CHECK(state->device, "create gpu device");
    SDL_CHECK(SDL_ClaimWindowForGPUDevice(state->device, state->window),
              "claim window for gpu device");
    {
        const SDL_GPUSamplerCreateInfo createinfo{};
        state->sampler = SDL_CreateGPUSampler(state->device, &createinfo);
        SDL_CHECK(state->sampler, "create gpu sampler")
    }
    state->default_texture = createGPUTexture(state->device, {1, 1});
    SDL_CHECK(state->default_texture, "create gpu texture");
    state->atlas.ptr = createGPUTexture(state->device, state->atlas.size);
    SDL_CHECK(state->default_texture, "create gpu texture");
    {
        SDL_GPUGraphicsPipelineCreateInfo createinfo = {};
        createinfo.vertex_shader =
            createGPUShader(state->device, shader_vert_code, SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        SDL_CHECK(createinfo.vertex_shader, "create gpu vertex shader");
        createinfo.fragment_shader =
            createGPUShader(state->device, shader_frag_code, SDL_GPU_SHADERSTAGE_FRAGMENT,
                            MAX_TEXTURE_SAMPLERS, 0);
        SDL_CHECK(createinfo.fragment_shader, "create gpu fragment shader");
        const SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
            {0, sizeof(vec2), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0},
            {1, sizeof(Instance), SDL_GPU_VERTEXINPUTRATE_INSTANCE, 0}};
        createinfo.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
        createinfo.vertex_input_state.num_vertex_buffers =
            SDL_arraysize(vertex_buffer_descriptions);
        SDL_GPUVertexAttribute vertex_attributes[] = {
            // vertex
            {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
            // instance
            {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, position)},
            {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, size)},
            {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, uv)},
            {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, uv) + sizeof(vec2)},
            {5, 1, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(Instance, color)},
            {6, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(Instance, rotation)},
            {7, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, offsetof(Instance, texture_index)}};
        createinfo.vertex_input_state.vertex_attributes = vertex_attributes;
        createinfo.vertex_input_state.num_vertex_attributes = SDL_arraysize(vertex_attributes);
        const SDL_GPUColorTargetDescription color_target_description = {
            .format = SDL_GetGPUSwapchainTextureFormat(state->device, state->window),
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
        state->pipeline = SDL_CreateGPUGraphicsPipeline(state->device, &createinfo);
        SDL_ReleaseGPUShader(state->device, createinfo.vertex_shader);
        SDL_ReleaseGPUShader(state->device, createinfo.fragment_shader);
        SDL_CHECK(state->pipeline, "create gpu graphics pipeline");
    }
    vec2 vertices[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    const uint VERTEX_BUFFER_SIZE = sizeof(vertices);
    i16 indices[6]{0, 1, 2, 0, 2, 3};
    const uint INDEX_BUFFER_SIZE = sizeof(indices);
    state->vertex_buffer =
        createGPUBuffer(state->device, SDL_GPU_BUFFERUSAGE_VERTEX, VERTEX_BUFFER_SIZE);
    SDL_CHECK(state->vertex_buffer, "create vertex buffer");
    state->index_buffer =
        createGPUBuffer(state->device, SDL_GPU_BUFFERUSAGE_INDEX, INDEX_BUFFER_SIZE);
    SDL_CHECK(state->index_buffer, "create index buffer");
    state->instance_buffer = createGPUBuffer(state->device, SDL_GPU_BUFFERUSAGE_VERTEX,
                                             sizeof(Instance) * MAX_INSTANCES);
    SDL_CHECK(state->instance_buffer, "create instance buffer");

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(state->device);
    SDL_CHECK(command_buffer, "acquire gpu command buffer");
    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    {
        auto *transfer_buffer = createGPUTransferBuffer(
            state->device, sizeof(Color) + VERTEX_BUFFER_SIZE + INDEX_BUFFER_SIZE);
        SDL_CHECK(transfer_buffer, "create gpu transfer buffer");

        {
            u8 *memory = (u8 *)SDL_MapGPUTransferBuffer(state->device, transfer_buffer, false);
            SDL_CHECK(memory, "map gpu transfer buffer");

            *(Color *)memory = WHITE;
            memory += sizeof(Color);

            SDL_memcpy(memory, vertices, VERTEX_BUFFER_SIZE);
            memory += VERTEX_BUFFER_SIZE;

            SDL_memcpy(memory, indices, INDEX_BUFFER_SIZE);

            SDL_UnmapGPUTransferBuffer(state->device, transfer_buffer);
        }
        {
            SDL_GPUTextureTransferInfo source = {};
            source.transfer_buffer = transfer_buffer;
            SDL_GPUTextureRegion destination = {};
            destination.texture = state->default_texture;
            destination.w = 1;
            destination.h = 1;
            destination.d = 1;
            SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);
        }
        uploadToGPUBuffer(copy_pass, transfer_buffer, sizeof(Color), state->vertex_buffer,
                          VERTEX_BUFFER_SIZE);
        uploadToGPUBuffer(copy_pass, transfer_buffer, sizeof(Color) + VERTEX_BUFFER_SIZE,
                          state->index_buffer, INDEX_BUFFER_SIZE);

        SDL_ReleaseGPUTransferBuffer(state->device, transfer_buffer);

        state->character_texture =
            loadTexture(state->device, copy_pass, "resources/character.png");
        SDL_CHECK(state->character_texture.ptr, "load world texture");
        state->font_texture = loadTexture(state->device, copy_pass, "resources/font.png");
        SDL_CHECK(state->font_texture.ptr, "load font texture");
    }
    {
        auto *transfer_buffer = createGPUTransferBuffer(
            state->device, sizeof(Color) * state->atlas.size.x * state->atlas.size.y);
        SDL_CHECK(transfer_buffer, "create gpu transfer buffer");

        {
            Color *memory =
                (Color *)SDL_MapGPUTransferBuffer(state->device, transfer_buffer, false);
            SDL_CHECK(memory, "map gpu transfer buffer");
            for (usize i = 0; i < usize(state->atlas.size.x) * usize(state->atlas.size.y); i++) {
                memory[i] = RED;
            }
            SDL_UnmapGPUTransferBuffer(state->device, transfer_buffer);
        }

        // TODO: Now actually load this to gpu
        auto files = sliceFromZeroSentinelArray(
            SDL_GlobDirectory("resources/", "*.png", SDL_GLOB_CASEINSENSITIVE, 0));
        debugAllocatorOwn(&state->debug_allocator, files);
        SDL_CHECK(files.len, "find any resources")
        for (auto &file : files) {
            auto path = allocFormatSentinel(allocator, "resources/%s", file);
            SDL_Log("%s", path.ptr);
            auto *surface = SDL_LoadPNG(path.ptr);
            SDL_Log("%d %d %x", surface->w, surface->h, surface->format);
            SDL_CHECK(surface, "load png")
            SDL_DestroySurface(surface);
            free(allocator, path);
        }
        free(allocator, files);

        {
            SDL_GPUTextureTransferInfo source = {};
            source.transfer_buffer = transfer_buffer;
            SDL_GPUTextureRegion destination = {};
            destination.texture = state->atlas.ptr;
            destination.w = state->atlas.size.x;
            destination.h = state->atlas.size.y;
            destination.d = 1;
            SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);
        }

        SDL_ReleaseGPUTransferBuffer(state->device, transfer_buffer);
    }
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    state->instance_transfer_buffer =
        createGPUTransferBuffer(state->device, sizeof(Instance) * MAX_INSTANCES);
    SDL_CHECK(state->instance_transfer_buffer, "create gpu transfer buffer");

    for (uint i = 0; i < MAX_TEXTURE_SAMPLERS; i++) {
        state->texture_sampler_bindings[i].texture = state->default_texture;
        state->texture_sampler_bindings[i].sampler = state->sampler;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    auto *state = (State *)appstate;

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(state->device);
    SDL_CHECK(command_buffer, "acquire gpu command buffer");

    uint instances_len = 0;

    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    {
        {
            Instance *instances_raw = (Instance *)SDL_MapGPUTransferBuffer(
                state->device, state->instance_transfer_buffer, true);
            SDL_CHECK(instances_raw, "map gpu transfer buffer");

            instances_len = gameUpdate({instances_raw, MAX_INSTANCES});

            SDL_CHECK(instances_len <= MAX_INSTANCES, "too many instances");

            SDL_UnmapGPUTransferBuffer(state->device, state->instance_transfer_buffer);
        }

        uploadToGPUBuffer(copy_pass, state->instance_transfer_buffer, 0, state->instance_buffer,
                          sizeof(Instance) * instances_len);
    }
    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUTexture *swapchain_texture = 0;

    SDL_CHECK(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, state->window,
                                                    &swapchain_texture, 0, 0),
              "wait and acquire gpu swapchain texture");

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

        SDL_BindGPUGraphicsPipeline(render_pass, state->pipeline);
        SDL_GPUBufferBinding buffer_bindings[2] = {{state->vertex_buffer, 0},
                                                   {state->instance_buffer, 0}};
        SDL_BindGPUVertexBuffers(render_pass, 0, buffer_bindings, 2);
        const SDL_GPUBufferBinding buffer_binding = {state->index_buffer, 0};
        SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        // TODO: auto bind
        state->texture_sampler_bindings[0].texture = state->character_texture.ptr;
        state->texture_sampler_bindings[1].texture = state->atlas.ptr;
        for (uint i = 2; i < MAX_TEXTURE_SAMPLERS; i++) {
            state->texture_sampler_bindings[i].texture = state->default_texture;
        }

        SDL_BindGPUFragmentSamplers(render_pass, 0, state->texture_sampler_bindings,
                                    MAX_TEXTURE_SAMPLERS);
        struct UBO {
            ivec2 screen;
            vec2 camera;
        } ubo{};
        ubo.screen = {state->screen};
        SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));
        SDL_DrawGPUIndexedPrimitives(render_pass, 6, instances_len, 0, 0, 0);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, [[maybe_unused]] SDL_AppResult result) {
    auto *state = (State *)appstate;

    SDL_ReleaseGPUTransferBuffer(state->device, state->instance_transfer_buffer);

    SDL_ReleaseGPUTexture(state->device, state->character_texture.ptr);
    SDL_ReleaseGPUTexture(state->device, state->font_texture.ptr);
    SDL_ReleaseGPUTexture(state->device, state->atlas.ptr);

    SDL_ReleaseGPUBuffer(state->device, state->instance_buffer);
    SDL_ReleaseGPUBuffer(state->device, state->index_buffer);
    SDL_ReleaseGPUBuffer(state->device, state->vertex_buffer);

    SDL_ReleaseGPUGraphicsPipeline(state->device, state->pipeline);
    SDL_ReleaseGPUTexture(state->device, state->default_texture);
    SDL_ReleaseGPUSampler(state->device, state->sampler);
    SDL_ReleaseWindowFromGPUDevice(state->device, state->window);

    SDL_DestroyGPUDevice(state->device);
    SDL_DestroyWindow(state->window);

    debugAllocatorDeinit(&state->debug_allocator);
    SDL_free(state);
}
