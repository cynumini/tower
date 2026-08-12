#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstream>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

using f32 = float;
using f64 = double;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u8 = uint8_t;
using usize = size_t;

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

constexpr static float SCREEN_WIDTH = 640;
constexpr static float SCREEN_HEIGHT = 360;

inline constexpr usize MAX_OBJECTS = 1024;

struct Vector2 {
    f32 x{};
    f32 y{};

    constexpr Vector2 normalize() const {
        Vector2 result = *this;
        float length = sqrtf(result.x * result.x + result.y * this->y);
        if (length != 0) {
            result.x /= length;
            result.y /= length;
        }
        return result;
    }

    constexpr Vector2 operator*(float n) const { return {this->x * n, this->y * n}; }
    constexpr Vector2 &operator+=(Vector2 other) {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }

    constexpr Vector2 operator+(Vector2 other) const { return {x + other.x, y + other.y}; }
    constexpr Vector2 operator-(Vector2 other) const { return {x - other.x, y - other.y}; }
    constexpr Vector2 scale(Vector2 other) const { return {x * other.x, y * other.y}; }

    void print() { printf("Vector2 {x = %f, y = %f}\n", this->x, this->y); }
};

struct Rectangle {
    Vector2 position{};
    Vector2 size{};

    bool checkCollision(const Rectangle &other) const {
        bool collision = false;
        if ((this->position.x < (other.position.x + other.size.x) and (this->position.x + this->size.x) > other.position.x) and
            (this->position.y < (other.position.y + other.size.y) and (this->position.y + this->size.y) > other.position.y))
            collision = true;
        return collision;
    }

    Vector2 center() { return Vector2{position.x + size.x / 2, position.y + size.y / 2}; }

    constexpr float x_min() const { return this->position.x; }
    constexpr float y_min() const { return this->position.y; }
    constexpr float x_max() const { return this->position.x + this->size.x; }
    constexpr float y_max() const { return this->position.y + this->size.y; }

    Vector2 overlapSize(Rectangle other) {
        auto dx = fmin(this->x_max(), other.x_max()) - fmax(this->x_min(), other.x_min());
        auto dy = fmin(this->y_max(), other.y_max()) - fmax(this->y_min(), other.y_min());
        if (dx >= 0 and dy >= 0) {
            return {dx, dy};
        }
        return {};
    }

    constexpr Rectangle scale(Vector2 other) const { return {this->position.scale(other), this->size.scale(other)}; }
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

    static constexpr Matrix4 translation(float x, float y, float z) {
        auto m = Matrix4::identity();
        m.v[3][0] = x;
        m.v[3][1] = y;
        m.v[3][2] = z;
        return m;
    }

    static constexpr Matrix4 orthographic(float left, float right, float bottom, float top, float near, float far) {
        auto m = Matrix4::identity();
        float rl = right - left;
        float tb = top - bottom;
        float fn = far - near;
        m.v[0][0] = 2.f / rl;
        m.v[1][1] = 2.f / tb;
        m.v[2][2] = 2.f / fn;
        m.v[3][0] = -((right + left) / (rl));
        m.v[3][1] = -((top + bottom) / (tb));
        m.v[3][2] = -((far + near) / (fn));
        return m;
    }

    using v4 = float[4];
    constexpr v4 &operator[](usize i) { return v[i]; }
    constexpr const v4 &operator[](usize i) const { return v[i]; }

    constexpr Matrix4 operator*(const Matrix4 &other) const {
        Matrix4 result{};
        for (usize col = 0; col < 4; col++) {
            for (usize row = 0; row < 4; row++) {
                for (usize i = 0; i < 4; i++) {
                    result.v[col][row] += (*this)[i][row] * other[col][i];
                }
            }
        }
        return result;
    }

    constexpr bool operator==(const Matrix4 &other) const {
        for (usize col = 0; col < 4; col++)
            for (usize row = 0; row < 4; row++)
                if (*this[col][row] != other[col][row])
                    return false;
        return true;
    }
};

inline constexpr usize MAX_INSTANCES_LEN = 2048;

struct Vertex {
    Vector2 position{};
};

struct Color {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;
};

static constexpr Color WHITE = {1, 1, 1, 1};
static constexpr Color GRAY = {0.5, 0.5, 0.5, 1};
static constexpr Color BLACK = {0, 0, 0, 1};
static constexpr Color BLUE = {0, 0, 1, 1};

struct Instance {
    Vector2 position{};
    Vector2 size{};
    Rectangle uv{};
    Color color = WHITE;
    int use_texture = true;
};

struct Texture {
    SDL_GPUTexture *texture = nullptr;
    int width{};
    int height{};
};

#define SDL_ENSURE(check, message)                                                                                                                             \
    do {                                                                                                                                                       \
        if (!(check)) {                                                                                                                                        \
            SDL_Log("Couldn't %s: %s", (message), SDL_GetError());                                                                                             \
            abort();                                                                                                                                           \
        }                                                                                                                                                      \
    } while (false)

template <typename T, usize N> struct Array {
    T data[N]{};

    constexpr T &operator[](usize i) { return data[i]; }
    constexpr const T &operator[](usize i) const { return data[i]; }
    constexpr T *begin() { return data; }
    constexpr T *end() { return data + N; }
};

template <typename T, usize N> struct FixedArray {
    T data[N]{};
    usize len = 0;

    constexpr T &operator[](usize i) { return data[i]; }
    constexpr const T &operator[](usize i) const { return data[i]; }
    constexpr T *begin() { return data; }
    constexpr T *end() { return data + len; }

    template <typename F> void sort(F compar) {
        for (int i = 1; i < (int)len; i++) {
            T key = data[i];
            int j = i - 1;
            while (j >= 0 and compar(data[j], key)) {
                data[j + 1] = data[j];
                j--;
            }
            data[j + 1] = key;
        }
    }

    constexpr usize add(const T &value) {
        assert(len < N);
        auto index = len;
        data[index] = value;
        len++;
        return index;
    }

    constexpr void clear() { len = 0; }
};

Matrix4 base_model_view = Matrix4::orthographic(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 1);

struct Game {

    float dt{};
    float fps{};

    int frames{};
    Uint64 fps_timer{};
    bool should_close = false;
    u64 previous{};

    Array<bool, SDL_SCANCODE_COUNT> pressed = {};
    Array<bool, SDL_SCANCODE_COUNT> pressed_repeat = {};
    const bool *keyboard_state = nullptr;
    bool fullscreen = false;

    SDL_Window *window;

    Game(SDL_Window *window) : window(window) {}

    bool shouldClose() { return should_close; }

    void beginFrame() {
        frames++;
        if (keyboard_state == NULL) {
            keyboard_state = SDL_GetKeyboardState(nullptr);
        }
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
                } else if (event.key.scancode == SDL_SCANCODE_F11) {
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
                float width = (float)event.window.data1;
                float height = (float)event.window.data2;
                float x_offset = 0, y_offset = 0;
                auto aspect_ratio = width / height;
                if (16.0 / 9.0 > aspect_ratio) {
                    auto scale = SCREEN_WIDTH / width;
                    width *= scale;
                    height *= scale;
                    y_offset = (height - SCREEN_HEIGHT) / 2;

                } else {
                    auto scale = SCREEN_HEIGHT / height;
                    width *= scale;
                    height *= scale;
                    x_offset = (width - SCREEN_WIDTH) / 2;
                }
                base_model_view = Matrix4::orthographic(0, width, height, 0, 0, 1) * Matrix4::translation(x_offset, y_offset, 0);
                break;
            }
            }
        }

        u64 now = SDL_GetTicks();
        dt = float(now - previous) / 1000.f;
        previous = now;

        if (now - fps_timer >= 1000) {
            fps = float(frames) * 1000.0f / float(now - fps_timer);
            frames = 0;
            fps_timer = now;
        }
    }

    void endFrame() {
        pressed = {};
        pressed_repeat = {};
    }

    bool isKeyDown(SDL_Scancode key) { return keyboard_state[key]; }
    bool isKeyPressed(SDL_Scancode key) { return pressed[key]; }
    bool isKeyPressedRepeat(SDL_Scancode key) { return pressed_repeat[key]; }
};

constexpr usize MAX_TEXTURES_LEN = 128;

struct Renderer {
    SDL_Window *window = nullptr;
    SDL_GPUDevice *device = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
    SDL_GPUSampler *sampler = nullptr;
    SDL_GPUBuffer *vertex_buffer = nullptr;
    SDL_GPUBuffer *instance_buffer = nullptr;
    SDL_GPUBuffer *index_buffer = nullptr;
    std::vector<Instance> instances;
    std::vector<usize> instances_texture;
    FixedArray<Matrix4, MAX_INSTANCES_LEN> instances_model_view;
    FixedArray<Texture, MAX_TEXTURES_LEN> textures;

    SDL_GPUShader *load_shader(const std::string &filename, SDL_GPUShaderStage stage, u32 num_uniform_buffers, u32 num_samplers) {
        std::ifstream file(filename, std::ios::binary);
        file.seekg(0, std::ios::end);
        auto n = file.tellg();
        std::vector<u8> data((usize)n);
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
            {.location = 3, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, uv)},
            {.location = 4, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, uv) + sizeof(Vector2)},
            {.location = 5, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(Instance, color)},
            {.location = 6, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_INT, .offset = offsetof(Instance, use_texture)},
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

    Renderer() {
        // init sdl, window, gpu
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
        SDL_SetAppMetadata("Tower", "0.1", "cynumini.tower");

        SDL_ENSURE(SDL_Init(SDL_INIT_VIDEO), "initialize SDL");

        this->window = SDL_CreateWindow("Tower", (int)SCREEN_WIDTH, (int)SCREEN_HEIGHT, 0);
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
        SDL_ENSURE(index_buffer, "create index buffer");
        buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        buffer_create_info.size = sizeof(Instance) * MAX_INSTANCES_LEN;
        instance_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
        SDL_ENSURE(instance_buffer, "create instance buffer");
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
        // SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);
    }

    ~Renderer() {
        for (usize i = 0; i < textures.len; i++) {
            SDL_ReleaseGPUTexture(device, textures[i].texture);
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

    usize loadTexture(const std::string &filename) {
        Texture texture{};
        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
        auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        texture.texture = IMG_LoadGPUTexture(device, copy_pass, filename.c_str(), &texture.width, &texture.height);
        SDL_ENSURE(texture.texture, "load gpu texture");
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(command_buffer);
        textures.add(std::move(texture));
        return textures.len - 1;
    };

    void begin() {
        instances.clear();
        instances_texture.clear();
        instances_model_view.clear();
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
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, true);
        SDL_EndGPUCopyPass(copy_pass);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

        SDL_GPUTexture *swapchain_texture = nullptr;
        SDL_ENSURE(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr),
                   "wait and acquire gpu swapchain texture");
        if (swapchain_texture) {
            SDL_GPUColorTargetInfo color_target_info{};
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = {0.5, 0.5, 0.5, 1};
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

            usize last_texture = instances_texture[0];
            auto last_model_view = instances_model_view[0];
            usize start = 0;
            usize end = 0;
            auto drawCall = [this, &render_pass, &command_buffer](usize texture_index, Matrix4 model_view, usize start, usize end) {
                SDL_PushGPUVertexUniformData(command_buffer, 0, &model_view, sizeof(model_view));
                SDL_GPUTextureSamplerBinding texture_sampler_bindings{};
                texture_sampler_bindings.texture = textures[texture_index].texture;
                texture_sampler_bindings.sampler = sampler;
                SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_bindings, 1);
                SDL_DrawGPUIndexedPrimitives(render_pass, 6, (u32)(end - start), 0, 0, (u32)start);
            };
            for (usize i = 0; i < instances_texture.size(); i++) {
                bool is_same_texture = instances_texture[i] == last_texture;
                if (is_same_texture and instances_model_view[i] == last_model_view) {
                    end++;
                } else {
                    drawCall(last_texture, last_model_view, start, end);
                    start = i;
                    end = i + 1;
                    last_texture = instances_texture[i];
                    last_model_view = instances_model_view[i];
                }
            }
            drawCall(last_texture, last_model_view, start, end);
            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    void drawRectangle(Rectangle dest, Matrix4 model_view, Color color) {
        Instance instance{
            .position = dest.position,
            .size = dest.size,
            .color = color,
            .use_texture = false,
        };
        instances.push_back(instance);
        instances_texture.push_back(0); // very bad, it's just use first texture, but shader ignore it in fact
        instances_model_view.add(model_view);
    }

    void drawTexture(usize texture, Rectangle source, Rectangle dest, Matrix4 model_view, Color color = WHITE) {
        Rectangle uv = source;
        uv.position.x /= (float)textures[texture].width;
        uv.position.y /= (float)textures[texture].height;
        uv.size.x /= (float)textures[texture].width;
        uv.size.y /= (float)textures[texture].height;
        Instance instance = {
            .position = dest.position,
            .size = dest.size,
            .uv = uv,
            .color = color,
        };

        instances.push_back(instance);
        instances_texture.push_back(texture);
        instances_model_view.add(model_view);
    }
};

struct String {
    const char *data = nullptr;
    usize len = 0;

    String(const char *s) {
        this->data = s;
        this->len = strlen(s);
    }

    const char *begin() const { return data; }

    const char *end() const { return data + len; }
};

// static void unreachable(const char *string, usize line) {
//     printf("%s:%zu: unreachable\n", string, line);
//     abort();
// }

constexpr static Array<u8, 128> defaultFontWidths() {
    Array<u8, 128> data{};
    for (auto &value : data) {
        value = 8;
    }
    for (auto i = '0'; i <= '9'; i++) {
        data[usize(i)] = 4;
    }
    for (auto i = 'A'; i <= 'Z'; i++) {
        data[usize(i)] = 4;
    }
    for (auto i = 'a'; i <= 'z'; i++) {
        data[usize(i)] = 4;
    }
    data[' '] = 2;
    data['!'] = 1;
    data['\''] = 1;
    data[','] = 2;
    data['-'] = 4;
    data['.'] = 1;
    data['0'] = 5;
    data['1'] = 3;
    data['2'] = 5;
    data['6'] = 5;
    data['8'] = 5;
    data['9'] = 5;
    data[':'] = 1;
    data['A'] = 5;
    data['D'] = 5;
    data['I'] = 3;
    data['M'] = 7;
    data['N'] = 5;
    data['P'] = 5;
    data['Q'] = 5;
    data['S'] = 5;
    data['T'] = 5;
    data['V'] = 5;
    data['W'] = 7;
    data['X'] = 3;
    data['Y'] = 3;
    data['f'] = 3;
    data['i'] = 1;
    data['l'] = 1;
    data['m'] = 5;
    data['t'] = 3;
    data['v'] = 5;
    data['w'] = 5;
    data['x'] = 5;
    data['y'] = 5;
    data['z'] = 3;
    return data;
}

constexpr static auto DEFAULT_FONT_WIDTHS = defaultFontWidths();

#define UNREACHABLE() unreachable(__FILE__, __LINE__)

static void drawText(Renderer &renderer, const usize texture_id, Vector2 position, float size, const String &string) {
    constexpr float font_height = 10;
    float scale = size / font_height;
    float space = 1 * scale;
    float height = size;
    float offset = position.x;
    auto widths = DEFAULT_FONT_WIDTHS;
    for (auto c : string) {
        float font_width = float(widths[usize(c)]);
        float width = font_width * scale;
        int x = c % 16;
        int y = c / 16;
        Vector2 char_position{.x = float(x) * font_height, .y = float(y) * font_height};
        renderer.drawTexture(texture_id, {char_position, {font_width, font_height}}, {{offset, position.y}, {width, height}}, base_model_view);
        offset += width + space;
    }
}

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

    Rectangle calcAbsolutePositionOfRelativeRectangle(const Rectangle &rect) const {
        auto result = rect.scale(this->scale);
        result.position += this->position;
        return result;
    }
};

struct World {
    using ObjectId = usize;
    usize texture;
    usize font_texture;
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

    Object &getObject(ObjectId id, bool is_empty_ok = true) {

        assert(id != 0);
        assert(id < objects.len);
        return objects[id];
    }

    World(usize texture, usize font_texture) : texture(texture), font_texture(font_texture) { addObject({}); }

    void update(Game &game) {
        ticks = SDL_GetTicks();
        if (battle_mode) {
            auto &player = getObject(player_id);
            auto &enemy = getObject(current_enemy_id);
            if (!is_player_turn) {
                auto &player = getObject(player_id);
                player.hp -= player.in_defence ? 1 : 10;
                if (player.hp <= 0) {
                    game.should_close = true;
                }
                player.in_defence = false;
                is_player_turn = true;
                return;
            }
            if (game.isKeyPressedRepeat(SDL_SCANCODE_A)) {
                if (select == 0) {
                    select = 3;
                } else {
                    select -= 1;
                }
            } else if (game.isKeyPressedRepeat(SDL_SCANCODE_D)) {
                if (select == 3) {
                    select = 0;
                } else {
                    select += 1;
                }
            } else if (game.isKeyPressedRepeat(SDL_SCANCODE_SPACE)) {
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
                constexpr float player_speed = 300;
                if (pause) {
                    if (game.isKeyPressed(SDL_SCANCODE_SPACE)) {
                        is_dialog = false;
                        pause = false;
                    }
                    continue;
                }
                Vector2 velocity = {};
                velocity.y = (float)game.isKeyDown(SDL_SCANCODE_S) - (float)game.isKeyDown(SDL_SCANCODE_W);
                velocity.x = (float)game.isKeyDown(SDL_SCANCODE_D) - (float)game.isKeyDown(SDL_SCANCODE_A);
                velocity = velocity.normalize();
                player.position += velocity * player_speed * game.dt;
                auto world_player_collision = player.calcAbsolutePositionOfRelativeRectangle(player.collision);
                for (auto static_object : objects) {
                    auto world_static_object_collision = static_object.calcAbsolutePositionOfRelativeRectangle(static_object.collision);
                    if (static_object.kind == Object::Static and world_player_collision.checkCollision(world_static_object_collision)) {
                        auto direction = world_player_collision.center() - world_static_object_collision.center();
                        auto overlap_size = world_player_collision.overlapSize(world_static_object_collision);
                        if (overlap_size.x < overlap_size.y) {
                            auto x_offset = (player.center.x - (player.collision.position.x + player.center.x)) * player.scale.x;
                            if (direction.x > 0) {
                                player.position.x = world_static_object_collision.x_max() + x_offset;
                            } else {
                                player.position.x = world_static_object_collision.x_min() - world_player_collision.size.x + x_offset;
                            }
                        } else if (overlap_size.x > overlap_size.y) {
                            auto y_offset = (player.center.y - (player.collision.position.y + player.center.y)) * player.scale.y;
                            if (direction.y > 0) {
                                player.position.y = world_static_object_collision.y_max() + y_offset;
                            } else {
                                player.position.y = world_static_object_collision.y_min() - world_player_collision.size.y + y_offset;
                            }
                        }
                    }
                    if (game.isKeyPressed(SDL_SCANCODE_SPACE)) {
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
                    printf("Battle!\n");
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

    void draw(Renderer &renderer) {
        if (battle_mode) {
            constexpr float height = 28;
            Color selection[4] = {BLACK, BLACK, BLACK, BLACK};
            selection[select] = BLUE;
            auto &player = getObject(player_id);
            auto &enemy = getObject(current_enemy_id);
            static char buffer[128];
            sprintf(buffer, "HP: %d MP: %d", player.hp, player.mp);
            drawText(renderer, font_texture, {128 - 18, 256 - 32 - 10}, 10, buffer);
            renderer.drawTexture(texture, player.texture, {{128, 256 - 32}, {32, 64}}, base_model_view);
            sprintf(buffer, "HP: %d MP: %d", enemy.hp, enemy.mp);
            drawText(renderer, font_texture, {SCREEN_WIDTH - 128 - 18, 64 - 10}, 10, buffer);
            renderer.drawTexture(texture, enemy.texture, {{SCREEN_WIDTH - 128, 64}, {32, 32}}, base_model_view);

            renderer.drawRectangle({{4, SCREEN_HEIGHT - (height + 4)}, {74, height}}, base_model_view, selection[0]);
            drawText(renderer, font_texture, {8, SCREEN_HEIGHT - 28}, 20, "ATTACK");
            renderer.drawRectangle({{193 - 4, SCREEN_HEIGHT - (height + 4)}, {72, height}}, base_model_view, selection[1]);
            drawText(renderer, font_texture, {193, SCREEN_HEIGHT - 28}, 20, "DEFEND");
            renderer.drawRectangle({{382 - 4, SCREEN_HEIGHT - (height + 4)}, {56 + 8, height}}, base_model_view, selection[2]);
            drawText(renderer, font_texture, {382, SCREEN_HEIGHT - 28}, 20, "ITEMS");
            renderer.drawRectangle({{568 - 4, SCREEN_HEIGHT - (height + 4)}, {64 + 8, height}}, base_model_view, selection[3]);
            drawText(renderer, font_texture, {568, SCREEN_HEIGHT - 28}, 20, "ESCAPE");
            return;
        }
        for (auto id : render_order) {
            if (id == 0)
                continue;
            auto &object = this->getObject(id);
            if (!object.alive)
                continue;
            auto position = object.position - (object.center.scale(object.scale));
            auto size = Vector2{object.texture.size.x * object.scale.x, object.texture.size.y * object.scale.y};
            auto model_view = base_model_view * Matrix4::translation(-camera_target.x + SCREEN_WIDTH / 2, -camera_target.y + SCREEN_HEIGHT / 2, 0);
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

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {

    Renderer renderer;
    Game game(renderer.window);

    auto world_texture = renderer.loadTexture("world.png");
    auto font_texture = renderer.loadTexture("font.png");

    World world(world_texture, font_texture);
    world.show_interaction_arena = true;

    world.addObject({
        .kind = Object::Player,
        .texture = {{96, 0}, {16, 32}},
        .center = {8, 32},
        .position = {640.f / 2.f, 360.f / 2.f},
        .collision = {{-8, -4}, {16, 4}},
    });
    srand((u32)time(0));
    for (usize i = 0; i < 10; i++) {
        auto max_x = SCREEN_WIDTH * 4;
        auto max_y = SCREEN_HEIGHT * 4;
        float x = float(random() % int(max_x));
        float y = float(random() % int(max_y));
        x -= max_x / 2;
        y -= max_y / 2;
        world.addObject({
            .kind = Object::Enemy,
            .texture = {{112, 0}, {16, 16}},
            .center = {8, 16},
            .position = {x, y},
            .collision = {{-4, -3}, {8, 3}},
            .interaction_arena = {{-8, -16}, {16, 16}},
        });
    }

    for (usize i = 0; i < 256; i++) {
        auto max_x = SCREEN_WIDTH * 4;
        auto max_y = SCREEN_HEIGHT * 4;
        float x = float(random() % int(max_x));
        float y = float(random() % int(max_y));
        x -= max_x / 2;
        y -= max_y / 2;
        world.addObject({.kind = Object::Static,
                         .texture = {{64, 0}, {32, 64}},
                         .center = {16, 64},
                         .position = {x, y},
                         .collision = {{-3, -3}, {6, 3}},
                         .interaction_arena = {{-6, -6}, {12, 12}}});
    }

    char text[256] = "";

    while (!game.shouldClose()) {
        game.beginFrame();
        world.update(game);
        renderer.begin();
        world.draw(renderer);

        drawText(renderer, font_texture, {2, 2}, 10, "Version 0.2.0");
        sprintf(text, "FPS: %f", game.fps);
        drawText(renderer, font_texture, {2, 2 * 2 + 10}, 10, text);
        sprintf(text, "x: %f, y: %f", world.camera_target.x, world.camera_target.y);
        drawText(renderer, font_texture, {2, 2 * 3 + 20}, 10, text);
        if (world.is_dialog) {
            renderer.drawRectangle({{0, SCREEN_HEIGHT / 3 * 2}, {SCREEN_WIDTH, SCREEN_HEIGHT / 3}}, base_model_view, BLACK);
            drawText(renderer, font_texture, {8, SCREEN_HEIGHT / 3 * 2 + 8}, 10, "It's a tree!");
        };

        renderer.end();
        game.endFrame();
    }
    return 0;
}
