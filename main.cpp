#include "SDL3_image/SDL_image.h"
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

#define SDL_ENSURE(check, message)                                                                                                                             \
    do {                                                                                                                                                       \
        if (!(check)) {                                                                                                                                        \
            SDL_Log("Couldn't %s: %s", (message), SDL_GetError());                                                                                             \
            abort();                                                                                                                                           \
        }                                                                                                                                                      \
    } while (false)

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

struct Vertex {
    Vector2 pos;
    Vector2 uv;
};

struct Texture {
    SDL_GPUTexture *texture = nullptr;
};

struct Game {
    SDL_Window *window = nullptr;
    SDL_GPUDevice *device = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
    SDL_GPUSampler *sampler = nullptr;
    SDL_GPUBuffer *vertex_buffer = nullptr;
    SDL_GPUBuffer *index_buffer = nullptr;

    // share between begin and end
    SDL_GPUTexture *swapchain_texture = nullptr;
    SDL_GPURenderPass *render_pass = nullptr;
    SDL_GPUCommandBuffer *command_buffer = nullptr;

    bool should_close = false;
    std::vector<Texture> textures;

    SDL_GPUShader *load_shader(const std::string &filename, SDL_GPUShaderStage stage, u32 num_uniform_buffers, u32 num_samplers) {
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

    Game() {
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
        SDL_SetAppMetadata("Tower", "0.1", "cynumini.tower");

        SDL_ENSURE(SDL_Init(SDL_INIT_VIDEO), "initialize SDL");

        this->window = SDL_CreateWindow("Tower", 1280, 720, 0);
        SDL_ENSURE(window, "create window");

        this->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
        SDL_ENSURE(this->device, "create gpu device");

        SDL_ENSURE(SDL_ClaimWindowForGPUDevice(this->device, this->window), "claim window for gpu device");

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
        vertex_buffer_description.pitch = sizeof(Vertex);
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

        SDL_GPUSamplerCreateInfo sampler_create_info{};
        sampler = SDL_CreateGPUSampler(device, &sampler_create_info);
        SDL_ENSURE(sampler, "create gpu sampler");

        u16 indices[] = {
            0, 1, 2, 2, 1, 3,
        };
        constexpr auto vertex_buffer_size = sizeof(Vertex) * 4;
        constexpr auto index_buffer_size = sizeof(indices);
        SDL_GPUBufferCreateInfo buffer_create_info{};
        buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        buffer_create_info.size = vertex_buffer_size;
        vertex_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
        buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        buffer_create_info.size = index_buffer_size;
        index_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);

        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
        transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_buffer_create_info.size = index_buffer_size;
        auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
        auto transfer_memory = (char *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        memcpy(transfer_memory, indices, index_buffer_size);
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
        command_buffer = SDL_AcquireGPUCommandBuffer(device);
        auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        SDL_GPUTransferBufferLocation transfer_buffer_location{};
        transfer_buffer_location.transfer_buffer = transfer_buffer;
        SDL_GPUBufferRegion buffer_region{};
        buffer_region.buffer = index_buffer;
        buffer_region.size = index_buffer_size;
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    }

    ~Game() {
        for (auto texture : textures) {
            SDL_ReleaseGPUTexture(device, texture.texture);
        }
        SDL_ReleaseGPUSampler(device, sampler);
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        SDL_ReleaseGPUBuffer(device, index_buffer);
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    bool shouldClose() { return should_close; }

    size_t loadTexture(const std::string &filename) {
        Texture texture{};
        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
        auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        texture.texture = IMG_LoadGPUTexture(device, copy_pass, filename.c_str(), nullptr, nullptr);
        SDL_ENSURE(texture.texture, "load gpu texture");
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(command_buffer);
        textures.push_back(std::move(texture));
        return textures.size() - 1;
    };

    void begin() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT: {
                should_close = true;
                break;
            }
            case SDL_EVENT_KEY_DOWN: {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    should_close = true;
                }
                break;
            }
            }
        }

        command_buffer = SDL_AcquireGPUCommandBuffer(device);

        SDL_ENSURE(command_buffer, "acquire gpu command buffer");
    }

    void end() {

        SDL_ENSURE(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr),
                   "wait and acquire gpu swapchain texture");
        if (swapchain_texture) {
            SDL_GPUColorTargetInfo color_target_info{};
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = {1, 1, 1, 1};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;
            render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
            SDL_GPUBufferBinding binding{};
            binding.buffer = vertex_buffer;
            SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);
            binding.buffer = index_buffer;
            SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(ubo));
            SDL_GPUTextureSamplerBinding texture_sampler_bindings{};
            texture_sampler_bindings.texture = textures[0].texture;
            texture_sampler_bindings.sampler = sampler;
            SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_bindings, 1);
            SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    void drawTexture(size_t texture) {
        constexpr auto vertex_buffer_size = sizeof(Vertex) * 4;
        struct VertexData {
            Vector2 pos;
            Vector2 uv;
        } vertices[] = {
            {{{-0.5, 0.5}}, {{0, 0}}},  // tl
            {{{0.5, 0.5}}, {{1, 0}}},   // tr
            {{{-0.5, -0.5}}, {{0, 1}}}, // bl
            {{{0.5, -0.5}}, {{1, 1}}},  // br
        };
        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
        transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_buffer_create_info.size = vertex_buffer_size;
        auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
        auto transfer_memory = (char *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        memcpy(transfer_memory, vertices, vertex_buffer_size);
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
        auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        SDL_GPUTransferBufferLocation transfer_buffer_location{};
        transfer_buffer_location.transfer_buffer = transfer_buffer;
        SDL_GPUBufferRegion buffer_region{};
        buffer_region.buffer = vertex_buffer;
        buffer_region.size = vertex_buffer_size;
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);
        SDL_EndGPUCopyPass(copy_pass);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    Game game;

    auto texture = game.loadTexture("texture.png");

    while (!game.shouldClose()) {
        game.begin();
        game.drawTexture(texture);
        game.end();
    }
    return 0;
}
