#include "SDL3_image/SDL_image.h"
#include <cstddef>
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

struct Vector2 {
    float x;
    float y;

    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2 &other) const { return {x + other.x, y + other.y}; }
};

struct Rectangle {
    Vector2 position;
    Vector2 size;

    Rectangle() : position{0, 0}, size{0, 0} {}
    Rectangle(float x, float y, float width, float height) : position({x, y}), size({width, height}) {}
    Rectangle(Vector2 position, Vector2 size) : position(position), size(size) {}
};

struct Matrix4 {
    float v[4][4]{};

    static constexpr Matrix4 identity() {
        Matrix4 m{};
        m.v[0][0] = 1.0f;
        m.v[1][1] = 1.0f;
        m.v[2][2] = 1.0f;
        m.v[3][3] = 1.0f;
        return m;
    }

    static constexpr Matrix4 ortho(float left, float right, float bottom, float top, float near, float far) {
        Matrix4 m{};
        float rl = right - left;
        float tb = top - bottom;
        float fn = far - near;
        m.v[0][0] = 2.f / rl;
        m.v[1][1] = 2.f / tb;
        m.v[2][2] = 2.f / fn;
        m.v[3][0] = -((right + left) / (rl));
        m.v[3][1] = -((top + bottom) / (tb));
        m.v[3][2] = -((far + near) / (fn));
        m.v[3][3] = 1;
        return m;
    }
};

struct UBO {
    Matrix4 mvp = Matrix4::ortho(0, 1280, 720, 0, 0, 1);
} ubo;

struct Vertex {
    Vector2 position;
};

struct Instance {
    Vector2 position;
    Vector2 size;
    Vector2 uv_min;
    Vector2 uv_max;
};

constexpr size_t MAX_INSTANCES_LEN = 1024;

struct Texture {
    SDL_GPUTexture *texture = nullptr;
    int width = 0;
    int height = 0;
};

struct Game {
    SDL_Window *window = nullptr;
    SDL_GPUDevice *device = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
    SDL_GPUSampler *sampler = nullptr;
    SDL_GPUBuffer *vertex_buffer = nullptr;
    SDL_GPUBuffer *instance_buffer = nullptr;
    SDL_GPUBuffer *index_buffer = nullptr;
    std::vector<Instance> instances;
    std::vector<size_t> instances_texture;

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

    void createPipeline() {
        SDL_GPUGraphicsPipelineCreateInfo create_info{};
        // vertex_shader and fragment_shader
        create_info.vertex_shader = load_shader("./build/shader.spv.vert", SDL_GPU_SHADERSTAGE_VERTEX, 1, 0);
        create_info.fragment_shader = load_shader("./build/shader.spv.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
        // vertex_input_state
        SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
            {.slot = 0, .pitch = sizeof(Vertex), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0},
            {.slot = 1, .pitch = sizeof(Instance), .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE, .instance_step_rate = 0},
        };
        SDL_GPUVertexAttribute vertex_attributes[] = {
            {.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Vertex, position)},
            {.location = 1, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, position)},
            {.location = 2, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, size)},
            {.location = 3, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, uv_min)},
            {.location = 4, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, uv_max)},
        };
        create_info.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
        create_info.vertex_input_state.num_vertex_buffers = SDL_arraysize(vertex_buffer_descriptions);
        create_info.vertex_input_state.vertex_attributes = vertex_attributes;
        create_info.vertex_input_state.num_vertex_attributes = SDL_arraysize(vertex_attributes);
        // primitive_type
        create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        // target_info
        SDL_GPUColorTargetDescription color_target_descriptions{};
        color_target_descriptions.format = SDL_GetGPUSwapchainTextureFormat(device, window);
        color_target_descriptions.blend_state.enable_blend = true;
        color_target_descriptions.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        color_target_descriptions.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        color_target_descriptions.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        color_target_descriptions.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        color_target_descriptions.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        color_target_descriptions.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        create_info.target_info.color_target_descriptions = &color_target_descriptions;
        create_info.target_info.num_color_targets = 1;
        // create
        pipeline = SDL_CreateGPUGraphicsPipeline(device, &create_info);
        SDL_ENSURE(pipeline, "create gpu graphics pipeline");
        SDL_ReleaseGPUShader(device, create_info.vertex_shader);
        SDL_ReleaseGPUShader(device, create_info.fragment_shader);
    }

    Game() {
        // init sdl, window, gpu
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
        SDL_SetAppMetadata("Tower", "0.1", "cynumini.tower");

        SDL_ENSURE(SDL_Init(SDL_INIT_VIDEO), "initialize SDL");

        this->window = SDL_CreateWindow("Tower", 1280, 720, 0);
        SDL_ENSURE(window, "create window");

        this->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
        SDL_ENSURE(this->device, "create gpu device");

        SDL_ENSURE(SDL_ClaimWindowForGPUDevice(this->device, this->window), "claim window for gpu device");
        // create pipeline
        createPipeline();
        // create sampler
        SDL_GPUSamplerCreateInfo sampler_create_info{};
        sampler = SDL_CreateGPUSampler(device, &sampler_create_info);
        SDL_ENSURE(sampler, "create gpu sampler");
        // default indicies and vertices
        u16 indices[] = {
            0, 1, 2, 2, 1, 3,
        };
        Vertex vertices[] = {
            {{0.f, 0.f}},
            {{1.f, 0.f}},
            {{0.f, 1.f}},
            {{1.f, 1.f}},
        };
        // create buffers
        SDL_GPUBufferCreateInfo buffer_create_info{};
        buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        buffer_create_info.size = sizeof(vertices);
        vertex_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
        SDL_ENSURE(vertex_buffer, "create vertex buffer");
        buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        buffer_create_info.size = sizeof(indices);
        index_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
        SDL_ENSURE(vertex_buffer, "create index buffer");
        buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        buffer_create_info.size = sizeof(Instance) * MAX_INSTANCES_LEN;
        instance_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
        SDL_ENSURE(vertex_buffer, "create instance buffer");
        // upload vertex and indices
        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
        transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_buffer_create_info.size = sizeof(vertices) + sizeof(indices);
        auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
        auto transfer_memory = (u8 *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        memcpy(transfer_memory, vertices, sizeof(vertices));
        memcpy(transfer_memory + sizeof(vertices), indices, sizeof(indices));
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
        auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        SDL_GPUTransferBufferLocation transfer_buffer_location{};
        transfer_buffer_location.transfer_buffer = transfer_buffer;
        SDL_GPUBufferRegion buffer_region{};
        buffer_region.buffer = vertex_buffer;
        buffer_region.size = sizeof(vertices);
        transfer_buffer_location.offset = 0;
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);
        buffer_region.buffer = index_buffer;
        buffer_region.size = sizeof(indices);
        transfer_buffer_location.offset = sizeof(vertices);
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
        SDL_ReleaseGPUBuffer(device, instance_buffer);
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
        texture.texture = IMG_LoadGPUTexture(device, copy_pass, filename.c_str(), &texture.width, &texture.height);
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
        instances.clear();
    }

    void end() {
        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_ENSURE(command_buffer, "acquire gpu command buffer");
        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
        auto size = instances.size() * sizeof(Instance);
        transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_buffer_create_info.size = (u32)size;
        auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
        auto transfer_memory = (char *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        memcpy(transfer_memory, instances.data(), size);
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
        auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        SDL_GPUTransferBufferLocation transfer_buffer_location{};
        transfer_buffer_location.transfer_buffer = transfer_buffer;
        SDL_GPUBufferRegion buffer_region{};
        buffer_region.buffer = instance_buffer;
        buffer_region.size = (u32)size;
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);
        SDL_EndGPUCopyPass(copy_pass);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

        SDL_GPUTexture *swapchain_texture = nullptr;
        SDL_ENSURE(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr),
                   "wait and acquire gpu swapchain texture");
        if (swapchain_texture) {
            SDL_GPUColorTargetInfo color_target_info{};
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = {1, 1, 1, 1};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;
            auto render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
            SDL_GPUBufferBinding binding{};
            binding.buffer = vertex_buffer;
            SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);
            binding.buffer = instance_buffer;
            SDL_BindGPUVertexBuffers(render_pass, 1, &binding, 1);
            binding.buffer = index_buffer;
            SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(ubo));

            size_t last_texture = instances_texture[0];
            size_t start = 0;
            size_t end = 0;
            auto drawCall = [this, &render_pass](size_t texture_index, size_t start, size_t end) {
                SDL_GPUTextureSamplerBinding texture_sampler_bindings{};
                    texture_sampler_bindings.texture = textures[texture_index].texture;
                    texture_sampler_bindings.sampler = sampler;
                    SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_bindings, 1);
                    SDL_DrawGPUIndexedPrimitives(render_pass, 6, (u32)(end-start), 0, 0, (u32)start);
            };
            for (size_t i = 0; i < instances_texture.size(); i++) {
                if (instances_texture[i] == last_texture) {
                    end++;
                } else {
                    drawCall(last_texture, start, end);
                    start = i;
                    end = i + 1;
                    last_texture = instances_texture[i];
                }
            }
            drawCall(last_texture, start, end);

            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    void drawTexture(size_t texture, Rectangle source, Rectangle dest) {
        Vector2 uv_min = source.position;
        Vector2 uv_max = source.position + source.size;
        uv_min.x = uv_min.x / (float)textures[texture].width;
        uv_min.y = uv_min.y / (float)textures[texture].height;
        uv_max.x = uv_max.x / (float)textures[texture].width;
        uv_max.y = uv_max.y / (float)textures[texture].height;
        Instance instance = {
            .position = dest.position,
            .size = dest.size,
            .uv_min = uv_min,
            .uv_max = uv_max,
        };

        instances.push_back(instance);
        instances_texture.push_back(texture);
    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    Game game;

    auto texture = game.loadTexture("texture.png");
    auto texture_font = game.loadTexture("font.png");

    while (!game.shouldClose()) {
        game.begin();
        game.drawTexture(texture_font, {0, 0, 8, 8}, {64, 0, 32, 32});
        game.drawTexture(texture, {0, 0, 32, 32}, {0, 0, 32, 32});
        game.drawTexture(texture, {32, 0, 32, 32}, {64, 0, 32, 32});

        game.drawTexture(texture_font, {0, 0, 8, 8}, {64, 64, 64, 64});
        game.end();
    }
    return 0;
}
