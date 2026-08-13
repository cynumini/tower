#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <string>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

using f32 = float;
using f64 = double;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u8 = uint8_t;
using usize = size_t;

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

struct Vector2 {
    f32 x = 0;
    f32 y = 0;

    Vector2 operator*(f32 n) const { return {x * n, y * n}; }
    Vector2 operator+(Vector2 other) const { return {x + other.x, y + other.y}; }
    Vector2 operator-(Vector2 other) const { return {x - other.x, y - other.y}; }
    Vector2 scale(Vector2 other) const { return {x * other.x, y * other.y}; }

    void operator+=(Vector2 other) { x += other.x, y += other.y; }
    void operator/=(f32 value) { x /= value, y /= value; }

    Vector2 normalize() const {
        Vector2 result = {x, y};
        f32 length = sqrtf(x * x + y * y);
        if (length != 0) {
            result /= length;
        }
        return result;
    }
};

struct Rectangle {
    f32 x = 0;
    f32 y = 0;
    f32 w = 0;
    f32 h = 0;

    Rectangle() {}
    Rectangle(f32 x, f32 y, f32 w, f32 h) : x(x), y(y), w(w), h(h) {}
    Rectangle(Vector2 position, Vector2 size) : x(position.x), y(position.y), w(size.x), h(size.y) {}

    Rectangle scale(Vector2 other) const { return {x * other.x, y * other.y, w * other.x, h * other.y}; }
    Vector2 center() const { return {x + w / 2, y + h / 2}; }
    Vector2 position() const { return {x, y}; };
    Vector2 size() const { return {w, h}; };
    bool checkCollision(const Rectangle &other) const { return (x < other.x_max() and x_max() > other.x) and (y < other.y_max() and y_max() > other.y); }
    f32 x_max() const { return x + w; }
    f32 y_max() const { return y + h; }

    Vector2 overlapSize(Rectangle other) const {
        auto dx = fmin(x_max(), other.x_max()) - fmax(x, other.x);
        auto dy = fmin(y_max(), other.y_max()) - fmax(y, other.y);
        if (dx >= 0 and dy >= 0) {
            return {dx, dy};
        }
        return {};
    }
};

struct Matrix4 {
    f32 v[4][4]{};

    static Matrix4 identity() {
        Matrix4 m{};
        m.v[0][0] = 1.0f;
        m.v[1][1] = 1.0f;
        m.v[2][2] = 1.0f;
        m.v[3][3] = 1.0f;
        return m;
    }

    static Matrix4 translation(f32 x, f32 y, f32 z) {
        auto m = Matrix4::identity();
        m.v[3][0] = x;
        m.v[3][1] = y;
        m.v[3][2] = z;
        return m;
    }

    static Matrix4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
        f32 rl = right - left;
        f32 tb = top - bottom;
        f32 fn = far - near;
        auto m = Matrix4::translation(-((right + left) / rl), -((top + bottom) / tb), -((far + near) / fn));
        m.v[0][0] = 2.f / rl;
        m.v[1][1] = 2.f / tb;
        m.v[2][2] = 2.f / fn;
        return m;
    }

    f32 (&operator[](usize i))[4] { return v[i]; }
    const f32 (&operator[](usize i) const)[4] { return v[i]; }

    Matrix4 operator*(const Matrix4 &other) const {
        Matrix4 result{};
        for (usize col = 0; col < 4; col++) {
            for (usize row = 0; row < 4; row++) {
                for (usize i = 0; i < 4; i++) {
                    result[col][row] += v[i][row] * other[col][i];
                }
            }
        }
        return result;
    }

    bool operator==(const Matrix4 &other) const {
        for (usize col = 0; col < 4; col++)
            for (usize row = 0; row < 4; row++)
                if (v[col][row] != other[col][row])
                    return false;
        return true;
    }
};

inline constexpr usize MAX_INSTANCES = 2048;

struct Vertex {
    Vector2 position{};
};

struct Color {
    f32 r = 0;
    f32 g = 0;
    f32 b = 0;
    f32 a = 0;
};

static constexpr Color WHITE = {1, 1, 1, 1};
static constexpr Color GRAY = {0.5, 0.5, 0.5, 1};
static constexpr Color BLACK = {0, 0, 0, 1};
static constexpr Color BLUE = {0, 0, 1, 1};

struct String {
    const char *c_str = nullptr;
    usize len = 0;

    String(const char *s) {
        this->c_str = s;
        this->len = strlen(s);
    }

    const char *begin() const { return c_str; }

    const char *end() const { return c_str + len; }
};

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

constexpr Vector2 SCREEN = {640, 360};

Matrix4 base_model_view = Matrix4::orthographic(0, SCREEN.x, SCREEN.y, 0, 0, 1);

constexpr usize MAX_TEXTURES_LEN = 128;

struct Renderer {
    SDL_Window *window;

    SDL_GPUDevice *device;

    ImGuiIO *io;

    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUSampler *sampler;
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *instance_buffer;
    SDL_GPUBuffer *index_buffer;
    FixedArray<Instance, MAX_INSTANCES> instances;
    FixedArray<usize, MAX_INSTANCES> instances_texture;
    FixedArray<Matrix4, MAX_INSTANCES> instances_model_view;
    FixedArray<Texture, MAX_TEXTURES_LEN> textures;

    Renderer(SDL_Window *window, f32 scale_factor) : window(window) {
        device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, true, nullptr);
        SDL_ENSURE(device, "create gpu device");
        SDL_ENSURE(SDL_ClaimWindowForGPUDevice(device, window), "claim window for gpu device");
        SDL_ENSURE(SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC), "set gpu swapchain parameters");

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = &ImGui::GetIO();
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(scale_factor);
        style.FontScaleDpi = scale_factor;

        auto swapchain_texture_format = SDL_GetGPUSwapchainTextureFormat(device, window);

        ImGui_ImplSDL3_InitForSDLGPU(window);
        ImGui_ImplSDLGPU3_InitInfo init_info{};
        init_info.Device = device;
        init_info.ColorTargetFormat = swapchain_texture_format;
        init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
        init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
        init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
        ImGui_ImplSDLGPU3_Init(&init_info);

        {
            auto load_shader = [this](const String &filename, SDL_GPUShaderStage stage, u32 num_uniform_buffers = 0, u32 num_samplers = 0) {
                auto stream = fopen(filename.c_str, "r");
                assert(stream != nullptr);
                assert(fseek(stream, 0, SEEK_END) == 0);
                auto position = ftell(stream);
                assert(position != -1);
                auto n = (usize)position;
                assert(fseek(stream, 0, SEEK_SET) == 0);
                auto data = new u8[n];
                assert(fread(data, sizeof(u8), n, stream) == n);
                assert(fclose(stream) == 0);
                SDL_GPUShaderCreateInfo create_info{};
                create_info.code_size = n;
                create_info.code = data;
                create_info.entrypoint = "main";
                create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
                create_info.stage = stage;
                create_info.num_samplers = num_samplers;
                create_info.num_uniform_buffers = num_uniform_buffers;
                auto shader = SDL_CreateGPUShader(device, &create_info);
                delete[] data;
                return shader;
            };
            SDL_GPUGraphicsPipelineCreateInfo createinfo{};
            // vertex_shader and fragment_shader
            createinfo.vertex_shader = load_shader("./build/shader.spv.vert", SDL_GPU_SHADERSTAGE_VERTEX, 1);
            createinfo.fragment_shader = load_shader("./build/shader.spv.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);
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
            createinfo.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
            createinfo.vertex_input_state.num_vertex_buffers = SDL_arraysize(vertex_buffer_descriptions);
            createinfo.vertex_input_state.vertex_attributes = vertex_attributes;
            createinfo.vertex_input_state.num_vertex_attributes = SDL_arraysize(vertex_attributes);

            SDL_GPUColorTargetDescription color_target_descriptions{};
            color_target_descriptions.format = swapchain_texture_format;
            color_target_descriptions.blend_state.enable_blend = true;
            color_target_descriptions.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            color_target_descriptions.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            color_target_descriptions.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;

            color_target_descriptions.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            color_target_descriptions.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            color_target_descriptions.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

            createinfo.target_info.color_target_descriptions = &color_target_descriptions;
            createinfo.target_info.num_color_targets = 1;
            // create
            pipeline = SDL_CreateGPUGraphicsPipeline(device, &createinfo);
            SDL_ENSURE(pipeline, "create gpu graphics pipeline");
            SDL_ReleaseGPUShader(device, createinfo.vertex_shader);
            SDL_ReleaseGPUShader(device, createinfo.fragment_shader);
        }

        {
            SDL_GPUSamplerCreateInfo createinfo{};
            sampler = SDL_CreateGPUSampler(device, &createinfo);
            SDL_ENSURE(sampler, "create gpu sampler");
        }

        u16 indices[] = {
            0, 1, 2, 2, 1, 3,
        };

        Vertex vertices[] = {
            {{0.f, 0.f}},
            {{1.f, 0.f}},
            {{0.f, 1.f}},
            {{1.f, 1.f}},
        };

        {
            SDL_GPUBufferCreateInfo createinfo{};
            createinfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            createinfo.size = sizeof(vertices);
            vertex_buffer = SDL_CreateGPUBuffer(device, &createinfo);
            SDL_ENSURE(vertex_buffer, "create vertex buffer");

            createinfo.size = sizeof(Instance) * MAX_INSTANCES;
            instance_buffer = SDL_CreateGPUBuffer(device, &createinfo);
            SDL_ENSURE(instance_buffer, "create instance buffer");

            createinfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            createinfo.size = sizeof(indices);
            index_buffer = SDL_CreateGPUBuffer(device, &createinfo);
            SDL_ENSURE(index_buffer, "create index buffer");
        }

        {
            SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
            transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transfer_buffer_create_info.size = sizeof(vertices) + sizeof(indices);

            auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
            {
                auto transfer_memory = (u8 *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
                memcpy(transfer_memory, vertices, sizeof(vertices));
                memcpy(transfer_memory + sizeof(vertices), indices, sizeof(indices));
                SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
            }
            auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
            {
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
            }
            SDL_SubmitGPUCommandBuffer(command_buffer);
            SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        }
    }

    ~Renderer() {
        for (usize i = 0; i < textures.len; i++) {
            SDL_ReleaseGPUTexture(device, textures[i].texture);
        }
        SDL_WaitForGPUIdle(device);

        SDL_ReleaseGPUBuffer(device, instance_buffer);
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        SDL_ReleaseGPUBuffer(device, index_buffer);

        SDL_ReleaseGPUSampler(device, sampler);

        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);

        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui::DestroyContext();

        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
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
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void end() {
        ImGui::Render();
        ImDrawData *draw_data = ImGui::GetDrawData();

        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_ENSURE(command_buffer, "acquire gpu command buffer");

        if (instances.len > 0) {
            SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
            auto size = instances.len * sizeof(Instance);
            transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transfer_buffer_create_info.size = (u32)size;
            auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
            auto transfer_memory = (char *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
            memcpy(transfer_memory, instances.data, size);
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
        }

        SDL_GPUTexture *swapchain_texture;
        SDL_ENSURE(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr),
                   "wait and acquire gpu swapchain texture");
        if (swapchain_texture) {
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

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

            usize last_texture;
            Matrix4 last_model_view;
            if (instances.len > 0) {
                last_texture = instances_texture[0];
                last_model_view = instances_model_view[0];
            }
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
            for (usize i = 0; i < instances_texture.len; i++) {
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
            if (not (start == 0 and end == 0)) {
                drawCall(last_texture, last_model_view, start, end);
            }
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    void drawRectangle(Rectangle dest, Matrix4 model_view, Color color) {
        Instance instance{
            .position = dest.position(),
            .size = dest.size(),
            .color = color,
            .use_texture = false,
        };
        instances.add(instance);
        instances_texture.add(0); // very bad, it's just use first texture, but shader ignore it in fact
        instances_model_view.add(model_view);
    }

    void drawTexture(usize texture, Rectangle source, Rectangle dest, Matrix4 model_view, Color color = WHITE) {
        Rectangle uv = source;
        uv.x /= (f32)textures[texture].width;
        uv.y /= (f32)textures[texture].height;
        uv.w /= (f32)textures[texture].width;
        uv.h /= (f32)textures[texture].height;
        Instance instance = {
            .position = dest.position(),
            .size = dest.size(),
            .uv = uv,
            .color = color,
        };
        instances.add(instance);
        instances_texture.add(texture);
        instances_model_view.add(model_view);
    }
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

static void drawText(Renderer &renderer, const usize texture_id, Vector2 position, f32 size, const String &string) {
    constexpr f32 font_height = 10;
    f32 scale = size / font_height;
    f32 space = 1 * scale;
    f32 height = size;
    f32 offset = position.x;
    auto widths = DEFAULT_FONT_WIDTHS;
    for (auto c : string) {
        f32 font_width = f32(widths[usize(c)]);
        f32 width = font_width * scale;
        int x = c % 16;
        int y = c / 16;
        Vector2 char_position{.x = f32(x) * font_height, .y = f32(y) * font_height};
        renderer.drawTexture(texture_id, {char_position.x, char_position.y, font_width, font_height}, {offset, position.y, width, height}, base_model_view);
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
        result.x += position.x;
        result.y += position.y;
        return result;
    }
};

struct AppState;

constexpr usize MAX_OBJECTS = 1024;

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

    Object &getObject(ObjectId id) {

        assert(id != 0);
        assert(id < objects.len);
        return objects[id];
    }

    World(usize texture, usize font_texture) : texture(texture), font_texture(font_texture) { addObject({}); }

    void update(AppState &app_state);

    void draw(Renderer &renderer) {
        if (battle_mode) {
            constexpr f32 height = 28;
            Color selection[4] = {BLACK, BLACK, BLACK, BLACK};
            selection[select] = BLUE;
            auto &player = getObject(player_id);
            auto &enemy = getObject(current_enemy_id);
            static char buffer[128];
            sprintf(buffer, "HP: %d MP: %d", player.hp, player.mp);
            drawText(renderer, font_texture, {128 - 18, 256 - 32 - 10}, 10, buffer);
            renderer.drawTexture(texture, player.texture, {128, 256 - 32, 32, 64}, base_model_view);
            sprintf(buffer, "HP: %d MP: %d", enemy.hp, enemy.mp);
            drawText(renderer, font_texture, {SCREEN.x - 128 - 18, 64 - 10}, 10, buffer);
            renderer.drawTexture(texture, enemy.texture, {SCREEN.x - 128, 64, 32, 32}, base_model_view);

            renderer.drawRectangle({4, SCREEN.y - (height + 4), 74, height}, base_model_view, selection[0]);
            drawText(renderer, font_texture, {8, SCREEN.y - 28}, 20, "ATTACK");
            renderer.drawRectangle({193 - 4, SCREEN.y - (height + 4), 72, height}, base_model_view, selection[1]);
            drawText(renderer, font_texture, {193, SCREEN.y - 28}, 20, "DEFEND");
            renderer.drawRectangle({382 - 4, SCREEN.y - (height + 4), 56 + 8, height}, base_model_view, selection[2]);
            drawText(renderer, font_texture, {382, SCREEN.y - 28}, 20, "ITEMS");
            renderer.drawRectangle({568 - 4, SCREEN.y - (height + 4), 64 + 8, height}, base_model_view, selection[3]);
            drawText(renderer, font_texture, {568, SCREEN.y - 28}, 20, "ESCAPE");
            return;
        }
        for (auto id : render_order) {
            if (id == 0)
                continue;
            auto &object = this->getObject(id);
            if (!object.alive)
                continue;
            auto position = object.position - (object.center.scale(object.scale));
            auto size = Vector2{object.texture.w * object.scale.x, object.texture.h * object.scale.y};
            auto model_view = base_model_view * Matrix4::translation(-camera_target.x + SCREEN.x / 2, -camera_target.y + SCREEN.y / 2, 0);
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

//     while (!game.shouldClose()) {

//     }
//     return 0;
// }

struct AppState {
    SDL_Window *window;
    Renderer *renderer;
    World *world;

    usize world_texture;
    usize font_texture;

    bool show_debug_menu = false;
    bool fullscreen = false;
    bool should_close = false;

    f32 dt{};
    u64 previous;

    Array<bool, SDL_SCANCODE_COUNT> pressed = {};
    Array<bool, SDL_SCANCODE_COUNT> pressed_repeat = {};
    const bool *keyboard_state = nullptr;

    char text[256];

    AppState() {
        f32 scale_factor = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        SDL_ENSURE(scale_factor != 0.0f, "get display content scale");

        window = SDL_CreateWindow("Tower", int(SCREEN.x * scale_factor), int(SCREEN.y * scale_factor), SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        SDL_ENSURE(window, "create window");

        keyboard_state = SDL_GetKeyboardState(nullptr);

        renderer = new Renderer{window, scale_factor};

        world_texture = renderer->loadTexture("world.png");
        font_texture = renderer->loadTexture("font.png");

        world = new World{world_texture, font_texture};

        world->addObject({
            .kind = Object::Player,
            .texture = {{96, 0}, {16, 32}},
            .center = {8, 32},
            .position = {640.f / 2.f, 360.f / 2.f},
            .collision = {{-8, -4}, {16, 4}},
        });
        srand((u32)time(0));
        for (usize i = 0; i < 10; i++) {
            auto max_x = SCREEN.x * 4;
            auto max_y = SCREEN.y * 4;
            f32 x = f32(random() % int(max_x));
            f32 y = f32(random() % int(max_y));
            x -= max_x / 2;
            y -= max_y / 2;
            world->addObject({
                .kind = Object::Enemy,
                .texture = {{112, 0}, {16, 16}},
                .center = {8, 16},
                .position = {x, y},
                .collision = {{-4, -3}, {8, 3}},
                .interaction_arena = {{-8, -16}, {16, 16}},
            });
        }

        for (usize i = 0; i < 256; i++) {
            auto max_x = SCREEN.x * 4;
            auto max_y = SCREEN.y * 4;
            f32 x = f32(random() % int(max_x));
            f32 y = f32(random() % int(max_y));
            x -= max_x / 2;
            y -= max_y / 2;
            world->addObject({.kind = Object::Static,
                              .texture = {{64, 0}, {32, 64}},
                              .center = {16, 64},
                              .position = {x, y},
                              .collision = {{-3, -3}, {6, 3}},
                              .interaction_arena = {{-6, -6}, {12, 12}}});
        }
    }

    SDL_AppResult event(const SDL_Event &event) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
        case SDL_EVENT_QUIT: {
            return SDL_APP_SUCCESS;
        }
        case SDL_EVENT_KEY_DOWN: {
            if (event.key.scancode == SDL_SCANCODE_F3) {
                show_debug_menu = !show_debug_menu;
            }
            if (event.key.scancode == SDL_SCANCODE_F10) {
                return SDL_APP_SUCCESS;
            }
            if (event.key.scancode == SDL_SCANCODE_F11) {
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
            f32 width = (f32)event.window.data1;
            f32 height = (f32)event.window.data2;
            f32 x_offset = 0, y_offset = 0;
            auto aspect_ratio = width / height;
            if (16.0 / 9.0 > aspect_ratio) {
                auto scale = SCREEN.x / width;
                width *= scale;
                height *= scale;
                y_offset = (height - SCREEN.y) / 2;

            } else {
                auto scale = SCREEN.y / height;
                width *= scale;
                height *= scale;
                x_offset = (width - SCREEN.x) / 2;
            }
            base_model_view = Matrix4::orthographic(0, width, height, 0, 0, 1) * Matrix4::translation(x_offset, y_offset, 0);
            break;
        }
        }
        return SDL_APP_CONTINUE;
    }

    SDL_AppResult iterate() {
        u64 now = SDL_GetTicks();
        dt = f32(now - previous) / 1000.f;
        previous = now;
        if (should_close)
            return SDL_APP_SUCCESS;
        world->update(*this);
        renderer->begin();
        world->draw(*renderer);
        if (show_debug_menu) {
            ImGui::Begin("Debug menu");
            ImGui::Text("Version: 0.3.0");
            ImGui::Text("%.1f FPS", renderer->io->Framerate);
            ImGui::Text("x: %.2f, y: %.2f", world->camera_target.x, world->camera_target.y);
            ImGui::Checkbox("Show collisions", &world->show_collision);
            ImGui::Checkbox("Show interaction arenas", &world->show_interaction_arena);
            ImGui::End();
        }

        if (world->is_dialog) {
            renderer->drawRectangle({{0, SCREEN.y / 3 * 2}, {SCREEN.x, SCREEN.y / 3}}, base_model_view, BLACK);
            drawText(*renderer, font_texture, {8, SCREEN.y / 3 * 2 + 28}, 10, "It's a tree!");
        };

        renderer->end();
        pressed = {};
        pressed_repeat = {};
        return SDL_APP_CONTINUE;
    }

    ~AppState() {
        delete renderer;
        SDL_DestroyWindow(window);
    }

    bool isKeyDown(SDL_Scancode key) { return keyboard_state[key]; }
    bool isKeyPressed(SDL_Scancode key) { return pressed[key]; }
    bool isKeyPressedRepeat(SDL_Scancode key) { return pressed_repeat[key]; }
};

void World::update(AppState &app_state) {
    ticks = SDL_GetTicks();
    if (battle_mode) {
        auto &player = getObject(player_id);
        auto &enemy = getObject(current_enemy_id);
        if (!is_player_turn) {
            auto &player = getObject(player_id);
            player.hp -= player.in_defence ? 1 : 10;
            if (player.hp <= 0) {
                app_state.should_close = true;
            }
            player.in_defence = false;
            is_player_turn = true;
            return;
        }
        if (app_state.isKeyPressedRepeat(SDL_SCANCODE_A)) {
            if (select == 0) {
                select = 3;
            } else {
                select -= 1;
            }
        } else if (app_state.isKeyPressedRepeat(SDL_SCANCODE_D)) {
            if (select == 3) {
                select = 0;
            } else {
                select += 1;
            }
        } else if (app_state.isKeyPressedRepeat(SDL_SCANCODE_SPACE)) {
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
            constexpr f32 player_speed = 300;
            if (pause) {
                if (app_state.isKeyPressed(SDL_SCANCODE_SPACE)) {
                    is_dialog = false;
                    pause = false;
                }
                continue;
            }
            Vector2 velocity = {};
            velocity.y = (f32)app_state.isKeyDown(SDL_SCANCODE_S) - (f32)app_state.isKeyDown(SDL_SCANCODE_W);
            velocity.x = (f32)app_state.isKeyDown(SDL_SCANCODE_D) - (f32)app_state.isKeyDown(SDL_SCANCODE_A);
            velocity = velocity.normalize();
            player.position += velocity * player_speed * app_state.dt;
            auto world_player_collision = player.calcAbsolutePositionOfRelativeRectangle(player.collision);
            for (auto static_object : objects) {
                auto world_static_object_collision = static_object.calcAbsolutePositionOfRelativeRectangle(static_object.collision);
                if (static_object.kind == Object::Static and world_player_collision.checkCollision(world_static_object_collision)) {
                    auto direction = world_player_collision.center() - world_static_object_collision.center();
                    auto overlap_size = world_player_collision.overlapSize(world_static_object_collision);
                    if (overlap_size.x < overlap_size.y) {
                        auto x_offset = (player.center.x - (player.collision.x + player.center.x)) * player.scale.x;
                        if (direction.x > 0) {
                            player.position.x = world_static_object_collision.x_max() + x_offset;
                        } else {
                            player.position.x = world_static_object_collision.x - world_player_collision.w + x_offset;
                        }
                    } else if (overlap_size.x > overlap_size.y) {
                        auto y_offset = (player.center.y - (player.collision.y + player.center.y)) * player.scale.y;
                        if (direction.y > 0) {
                            player.position.y = world_static_object_collision.y_max() + y_offset;
                        } else {
                            player.position.y = world_static_object_collision.y - world_player_collision.h + y_offset;
                        }
                    }
                }
                if (app_state.isKeyPressed(SDL_SCANCODE_SPACE)) {
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

SDL_AppResult SDL_AppInit(void **app_state, [[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("Tower", "0.3.0", "cynumini.tower");
    SDL_ENSURE(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD), "initialize SDL");
    *app_state = new AppState();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *app_state) { return ((AppState *)app_state)->iterate(); }

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *app_state, [[maybe_unused]] SDL_Event *event) { return ((AppState *)app_state)->event(*event); }

void SDL_AppQuit(void *app_state, [[maybe_unused]] SDL_AppResult result) { delete (AppState *)app_state; }
