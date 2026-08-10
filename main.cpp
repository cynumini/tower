#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

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

inline constexpr usize MAX_OBJECTS = 128;

struct Vector2 {
    f32 x{};
    f32 y{};

    void normalize() {
        float length = sqrtf(this->x * this->x + this->y * this->y);
        if (length != 0) {
            this->x /= length;
            this->y /= length;
        }
    }

    constexpr Vector2 operator*(float n) const { return {this->x * n, this->y * n}; }
    constexpr Vector2 &operator+=(const Vector2 &other) {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }

    constexpr Vector2 add(Vector2 other) const { return {x + other.x, y + other.y}; }
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

    static constexpr Matrix4 orthographic(float left, float right, float bottom, float top, float near, float far) {
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

inline constexpr usize MAX_INSTANCES_LEN = 1024;

struct Vertex {
    Vector2 position{};
};

struct Instance {
    Vector2 position{};
    Vector2 size{};
    Rectangle uv{};
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

    static bool _compar() {}

    void sort(bool (*compar)(const T &, const T &)) {
        for (usize i = 1; i < len; i++) {
            T key = data[i];
            usize j = i - 1;
            while (j >= 0 and compar(data[j], key)) {
                data[j + 1] = data[j];
                j--;
            }
            data[j + 1] = key;
        }
    }

    constexpr void add(const T &value) {
        assert(len < N);
        data[len] = value;
        len++;
    }
};

struct Game {
    float dt{};
    float fps{};

    int frames{};
    Uint64 fps_timer{};
    bool should_close = false;
    u64 previous{};

    Array<bool, SDL_SCANCODE_COUNT> pressed = {};
    const bool *keyboard_state = nullptr;

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
                }
                pressed[event.key.scancode] = true;
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

    void endFrame() { pressed = {}; }

    bool isKeyDown(SDL_Scancode key) { return keyboard_state[key]; }
};

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

    std::vector<Texture> textures;

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

    usize loadTexture(const std::string &filename) {
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
        instances.clear();
        instances_texture.clear();
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
            struct UBO {
                Matrix4 mvp = Matrix4::orthographic(0, 1280, 720, 0, 0, 1);
            } ubo;
            SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(ubo));

            usize last_texture = instances_texture[0];
            usize start = 0;
            usize end = 0;
            auto drawCall = [this, &render_pass](usize texture_index, usize start, usize end) {
                SDL_GPUTextureSamplerBinding texture_sampler_bindings{};
                texture_sampler_bindings.texture = textures[texture_index].texture;
                texture_sampler_bindings.sampler = sampler;
                SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_bindings, 1);
                SDL_DrawGPUIndexedPrimitives(render_pass, 6, (u32)(end - start), 0, 0, (u32)start);
            };
            for (usize i = 0; i < instances_texture.size(); i++) {
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

    void drawTexture(usize texture, Rectangle source, Rectangle dest) {
        Rectangle uv = source;
        uv.position.x /= (float)textures[texture].width;
        uv.position.y /= (float)textures[texture].height;
        uv.size.x /= (float)textures[texture].width;
        uv.size.y /= (float)textures[texture].height;
        Instance instance = {
            .position = dest.position,
            .size = dest.size,
            .uv = uv,
        };

        instances.push_back(instance);
        instances_texture.push_back(texture);
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

static void unreachable(const char *string, usize line) {
    printf("%s:%zu: unreachable\n", string, line);
    abort();
}

constexpr static Array<u8, 128> defaultFontWidths() {
    Array<u8, 128> data{};
    for (auto &value : data) {
        value = 8;
    }
    data['!'] = 1;
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
    data[','] = 1;
    data['.'] = 1;
    data['0'] = 5;
    data['1'] = 3;
    data['2'] = 5;
    data['6'] = 5;
    data['8'] = 5;
    data['9'] = 5;
    data[':'] = 1;
    data['I'] = 3;
    data['M'] = 5;
    data['N'] = 5;
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
        renderer.drawTexture(texture_id, {char_position, {font_width, font_height}}, {{offset, position.y}, {width, height}});
        offset += width + space;
    }
}

struct Object {
    enum Kind { Player, Static };
    Kind kind;
    Rectangle texture;
    Rectangle rect;
};

struct World {
    usize texture;

    FixedArray<Object, MAX_OBJECTS> objects{};

    void addObject(const Object &object) { objects.add(object); }

    World(usize texture) : texture(texture) {}

    void update(Game &game) {
        for (auto &object : objects) {
            switch (object.kind) {
            case Object::Player: {
                constexpr float player_speed = 300;
                Vector2 velocity = {};
                velocity.y = (float)game.isKeyDown(SDL_SCANCODE_S) - (float)game.isKeyDown(SDL_SCANCODE_W);
                velocity.x = (float)game.isKeyDown(SDL_SCANCODE_D) - (float)game.isKeyDown(SDL_SCANCODE_A);
                velocity.normalize();

                object.rect.position += velocity * 300.f * game.dt;
                for (auto static_objects : objects) {
                    if (static_objects.kind == Object::Static) {
                        if (object.rect.checkCollision(static_objects.rect)) {
                            struct Sides {
                                float right;
                                float left;
                                float top;
                                float bottom;
                                Sides(Rectangle rect) {
                                    this->left = rect.position.x;
                                    this->top = rect.position.y;
                                    this->right = rect.position.x + rect.size.x;
                                    this->bottom = rect.position.y + rect.size.y;
                                }

                                bool isRightCollision(const Sides &other, Vector2 velocity) {
                                    if (velocity.x == 0) return false;
                                    return this->right > other.left and this->left < other.left;
                                };
                                bool isLeftCollision(const Sides &other) { return this->left<other.right and this->right> other.right; };
                                bool isBottomCollision(const Sides &other) { return this->bottom > other.top and this->top < other.top; };
                                bool isTopCollision(const Sides &other) { return this->top<other.bottom and this->bottom> other.bottom; };
                            };
                            auto p_sides = Sides(object.rect);
                            auto so_sides = Sides(static_objects.rect);
                            if (p_sides.isRightCollision(so_sides, velocity)) {
                                object.rect.position.x = so_sides.left - object.rect.size.x;
                            } else if (p_sides.isLeftCollision(so_sides)) {
                                object.rect.position.x = so_sides.right;
                            } else if (p_sides.isBottomCollision(so_sides)) {
                                object.rect.position.y = so_sides.top - object.rect.size.y;
                            } else if (p_sides.isTopCollision(so_sides)) {
                                object.rect.position.y = so_sides.bottom;
                            }
                        }
                    }
                }
                break;
            }
            case Object::Static: {
                break;
            }
            }
        }

        objects.sort([](const Object &a, const Object &b) { return a.rect.position.y + a.rect.size.y > b.rect.position.y + b.rect.size.y; });
    };

    void draw(Renderer &renderer) {
        for (auto object : objects) {
            renderer.drawTexture(texture, object.texture, object.rect);
        }
    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    Game game;
    Renderer renderer;

    auto world_texture = renderer.loadTexture("world.png");
    auto font_texture = renderer.loadTexture("font.png");

    World world(world_texture);

    world.addObject({Object::Player, {{96, 0}, {16, 32}}, {{550, 250}, {16 * 3, 32 * 3}}});
    world.addObject({Object::Static, {{64, 0}, {32, 64}}, {{200, 200}, {32 * 3, 64 * 3}}});
    world.addObject({Object::Static, {{64, 0}, {32, 64}}, {{900, 200}, {32 * 3, 64 * 3}}});
    world.addObject({Object::Static, {{64, 0}, {32, 64}}, {{550, 500}, {32 * 3, 64 * 3}}});

    char text[256] = "";

    while (!game.shouldClose()) {
        game.beginFrame();
        world.update(game);
        renderer.begin();
        world.draw(renderer);
        sprintf(text, "FPS: %f", game.fps);
        drawText(renderer, font_texture, {8, 8}, 30, "Version 0.1.0");
        drawText(renderer, font_texture, {8, 38}, 30, text);
        renderer.end();
        game.endFrame();
    }
    return 0;
}
