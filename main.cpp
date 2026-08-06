#include "SDL3_image/SDL_image.h"
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SDL_ENSURE(check, message)                                                                                                                             \
    do {                                                                                                                                                       \
        if (!(check)) {                                                                                                                                        \
            SDL_Log("Couldn't %s: %s", (message), SDL_GetError());                                                                                             \
            return SDL_APP_FAILURE;                                                                                                                            \
        }                                                                                                                                                      \
    } while (false)

using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

static SDL_Window *window = nullptr;
static SDL_GPUDevice *device = nullptr;
static SDL_GPUGraphicsPipeline *pipeline = nullptr;
static SDL_GPUBuffer *vertex_buffer = nullptr;
static SDL_GPUBuffer *index_buffer = nullptr;
static SDL_GPUTexture *texture;
static SDL_GPUSampler *sampler;

union Vector2 {
    struct {
        float x;
        float y;
    };
    float v[2];
};

struct Matrix4 {
    float v[4][4]{};

    static constexpr Matrix4 identity() {
        Matrix4 m;
        m.v[0][0] = 1.0f;
        m.v[1][1] = 1.0f;
        m.v[2][2] = 1.0f;
        m.v[3][3] = 1.0f;
        return m;
    }
};

struct UBO {
    Matrix4 mvp = Matrix4::identity();
} ubo;

static SDL_GPUShader *load_shader(std::string filename, SDL_GPUShaderStage stage, u32 num_uniform_buffers, u32 num_samplers) {
    std::ifstream file(filename, std::ios::binary);
    file.seekg(0, std::ios::end);
    auto n = file.tellg();
    std::vector<u8> data((size_t)n);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(data.data()), n);
    SDL_GPUShaderCreateInfo create_info{};
    create_info.code_size = data.size();
    create_info.code = data.data();
    create_info.entrypoint = "main";
    create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    create_info.stage = stage;
    create_info.num_uniform_buffers = num_uniform_buffers;
    create_info.num_samplers = num_samplers;
    return SDL_CreateGPUShader(device, &create_info);
}

SDL_AppResult SDL_AppInit([[maybe_unused]] void **appstate, [[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("Tower", "0.1", "cynumini.tower");

    SDL_ENSURE(SDL_Init(SDL_INIT_VIDEO), "initialize SDL");

    window = SDL_CreateWindow("Tower", 1280, 720, 0);
    SDL_ENSURE(window, "create window");

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    SDL_ENSURE(device, "create gpu device");

    SDL_ENSURE(SDL_ClaimWindowForGPUDevice(device, window), "claim window for gpu device");

    struct VertexData {
        Vector2 pos;
        Vector2 uv;
    } vertices[] = {
        {{{-0.5, 0.5}}, {{0, 0}}},  // tl
        {{{0.5, 0.5}}, {{1, 0}}},   // tr
        {{{-0.5, -0.5}}, {{0, 1}}}, // bl
        {{{0.5, -0.5}}, {{1, 1}}},  // br
    };

    u16 indices[] = {
        0, 1, 2, 2, 1, 3,
    };

    SDL_GPUBufferCreateInfo buffer_create_info{};
    buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_create_info.size = sizeof(vertices);
    vertex_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
    buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    buffer_create_info.size = sizeof(indices);
    index_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
    transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buffer_create_info.size = sizeof(vertices) + sizeof(indices);
    auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);

    auto transfer_memory = (char *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    memcpy(transfer_memory, vertices, sizeof(vertices));
    memcpy(transfer_memory + sizeof(vertices), indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    auto copy_command_buffer = SDL_AcquireGPUCommandBuffer(device);

    auto copy_pass = SDL_BeginGPUCopyPass(copy_command_buffer);

    SDL_GPUTransferBufferLocation transfer_buffer_location{};
    transfer_buffer_location.transfer_buffer = transfer_buffer;
    SDL_GPUBufferRegion buffer_region{};
    buffer_region.buffer = vertex_buffer;
    buffer_region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);
    transfer_buffer_location.offset = sizeof(vertices);
    buffer_region.buffer = index_buffer;
    buffer_region.size = sizeof(indices);
    SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);

    texture = IMG_LoadGPUTexture(device, copy_pass, "texture.png", nullptr, nullptr);
    SDL_ENSURE(texture, "load gpu texture");

    SDL_EndGPUCopyPass(copy_pass);

    SDL_SubmitGPUCommandBuffer(copy_command_buffer);

    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUSamplerCreateInfo sampler_create_info {};
    sampler = SDL_CreateGPUSampler(device, &sampler_create_info);

    SDL_GPUVertexAttribute vertex_attributes[] = {{
                                                      .location = 0,
                                                      .buffer_slot = 0,
                                                      .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                                                      .offset = 0,
                                                  },
                                                  {
                                                      .location = 1,
                                                      .buffer_slot = 0,
                                                      .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                                                      .offset = sizeof(Vector2),
                                                  }};

    SDL_GPUGraphicsPipelineCreateInfo create_info{};
    create_info.vertex_shader = load_shader("./build/shader.spv.vert", SDL_GPU_SHADERSTAGE_VERTEX, 1, 0);
    create_info.fragment_shader = load_shader("./build/shader.spv.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
    create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    SDL_GPUVertexBufferDescription vertex_buffer_description{};
    vertex_buffer_description.slot = 0;
    vertex_buffer_description.pitch = sizeof(VertexData);
    create_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_description;
    create_info.vertex_input_state.num_vertex_buffers = 1;
    create_info.vertex_input_state.vertex_attributes = vertex_attributes;
    create_info.vertex_input_state.num_vertex_attributes = 2;
    SDL_GPUColorTargetDescription color_target_descriptions{};
    color_target_descriptions.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    create_info.target_info.color_target_descriptions = &color_target_descriptions;
    create_info.target_info.num_color_targets = 1;
    pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);
    SDL_ENSURE(pipeline, "create gpu graphics pipeline");
    SDL_ReleaseGPUShader(device, create_info.vertex_shader);
    SDL_ReleaseGPUShader(device, create_info.fragment_shader);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event) {
    switch (event->type) {
    case SDL_EVENT_QUIT: {
        return SDL_APP_SUCCESS;
    }
    case SDL_EVENT_KEY_DOWN: {
        if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
        break;
    }
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *appstate) {
    auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_ENSURE(command_buffer, "acquire gpu command buffer");

    SDL_GPUTexture *swapchain_texture;

    SDL_ENSURE(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr), "wait and acquire gpu swapchain texture");

    if (swapchain_texture) {
        SDL_GPUColorTargetInfo color_target_info{};
        color_target_info.texture = swapchain_texture;
        color_target_info.clear_color = {1, 1, 1, 1}, color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target_info.store_op = SDL_GPU_STOREOP_STORE;
        auto render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
        SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
        SDL_GPUBufferBinding binding{};
        binding.buffer = vertex_buffer;
        SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);
        binding.buffer = index_buffer;
        SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(ubo));
        SDL_GPUTextureSamplerBinding texture_sampler_bindings{};
        texture_sampler_bindings.texture = texture;
        texture_sampler_bindings.sampler = sampler;
        SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_bindings, 1);
        SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit([[maybe_unused]] void *appstate, [[maybe_unused]] SDL_AppResult result) { SDL_ReleaseGPUTexture(device, texture); }
