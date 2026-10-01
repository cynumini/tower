#include <skn_sdl.cpp>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

static Arena arena;

#include "tower.hpp"

static SDL_GPUTexture *depth;

#include "ui_pipeline.cpp"
#include "world_pipeline.cpp"

static SDL_GPUTransferBuffer *instance_transfer_buffer;

struct Font {
    u8 widths[256];
    FRectangle texture;
    float size;

    void init(FRectangle texture) {
        size = 10.0F;
        for (size_t i = 0; i < 256; i++) widths[i] = 4;
        widths[' '] = 2;
        widths['!'] = 1;
        widths[','] = 2;
        widths['.'] = 1;
        widths['0'] = 5;
        widths['1'] = 3;
        widths['2'] = 5;
        widths['6'] = 5;
        widths['8'] = 5;
        widths['9'] = 5;
        widths[':'] = 1;
        widths['?'] = 5;
        widths['A'] = 5;
        widths['I'] = 3;
        widths['M'] = 7;
        widths['O'] = 5;
        widths['P'] = 5;
        widths['S'] = 5;
        widths['Z'] = 5;
        widths['\''] = 1;
        widths['i'] = 1;
        widths['l'] = 1;
        widths['m'] = 5;
        widths['v'] = 5;
        widths['w'] = 5;
        widths['x'] = 5;
        widths['y'] = 5;
        widths['Y'] = 5;
        widths['W'] = 9;
        this->texture = texture;
    }
};

enum class KeyState : u8 { none, pressed, released };

struct Mod {
    Slice<const char> name;
    Dynamic<Slice<const char>> items;
};

static struct Time {
    u64 counter;
    u64 frames;
    float seconds;
    float frequency;
    u16 fps;
    float ms;
} time;

static struct Engine {
    Vector2i screen;
    Dynamic<Mod> mods;
    HashMap<FRectangle> sprites;
    const bool *keyboard_state;
    Font default_font;
    KeyState key_state[512];
    bool running;
    float dt;
    bool debug_mode;
    Color clear_color;

    bool is_key_pressed(Key key) const { return keyboard_state[int(key)]; }

    bool is_key_just_pressed(Key key, bool consume = true) {
        auto result = key_state[int(key)] == KeyState::pressed;
        if (consume) key_state[int(key)] = KeyState::none;
        return result;
    }

    bool is_key_just_released(Key key, bool consume = true) {
        auto result = key_state[int(key)] == KeyState::released;
        if (consume) key_state[int(key)] = KeyState::none;
        return result;
    }

    __attribute__((format(printf, 1, 2))) static void log(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
        va_end(args);
    }

    int rand(int n) { return SDL_rand(n); }

    static void drawText(Fixed<UI::Instance> *renderer, Slice<const char> text, Vector2f position,
                         float size, Color color, Font font) {
        float advance = 0;
        for (size_t i = 0; i < text.len; i++) {
            Vector2f texture_offset = {0, 0};
            const u16 c = text.ptr[i] - ' ';
            texture_offset = {
                .x = float(c % 16) * font.size,
                .y = float(c / 16) * font.size, // NOLINT
            };
            renderer->append({{position.x + advance, position.y},
                              {size, size},
                              FRectangle{font.texture + texture_offset, font.size, font.size},
                              color});
            advance += (float(font.widths[u8(text.ptr[i])]) + 1) * (size / font.size);
        }
    }

    void drawText(Fixed<UI::Instance> *renderer, Slice<const char> text, Vector2f position,
                  float size = 10.0F, Color color = WHITE) const {
        drawText(renderer, text, position, size, color, default_font);
    }

    void drawText(Fixed<UI::Instance> *renderer, Slice<char> text, Vector2f position,
                  float size = 10.0F, Color color = WHITE) const {
        drawText(renderer, {text.len, text.ptr}, position, size, color, default_font);
    }

    static float measureText(Slice<const char> text, float size, Font font) {
        if (text.len == 0) return 0;
        float advance = 0;
        for (size_t i = 0; i < text.len; i++) {
            advance += (float(font.widths[u8(text.ptr[i])]) + 1) * (size / font.size);
        }
        return advance - (size / font.size);
    }
    float measureText(Slice<const char> text, float size = 10.0F) const {
        return measureText(text, size, default_font);
    }

    void drawTextF(Arena *a, Fixed<UI::Instance> *instances, Vector2f pos, const char *fmt, ...)
        __attribute__((format(gnu_printf, 5, 6))) {
        va_list ap;
        va_start(ap, fmt);
        auto slice = a->vAllocPrintZ(fmt, ap);
        va_end(ap);

        drawText(instances, slice.withoutZero(), pos);
        a->free(slice);
    }

    void updateUI(Fixed<UI::Instance> *instances) {
        if (debug_mode) {
            ScopeArena scope(&arena);
            float offset_y = 2.0F;

            drawTextF(&scope.tmp, instances, {2, offset_y}, "FPS: %d", time.fps);
            offset_y += 12;
            drawTextF(&scope.tmp, instances, {2, offset_y}, "%.2fms", time.ms);
            offset_y += 12;
            drawTextF(&scope.tmp, instances, {2, offset_y}, "Pitch: %.0f", world.camera.pitch);
            offset_y += 12;
            drawTextF(&scope.tmp, instances, {2, offset_y}, "Yaw: %.0f", world.camera.yaw);
            offset_y += 12;
            drawTextF(&scope.tmp, instances, {2, offset_y}, "Roll: %.0f", world.camera.roll);
            offset_y += 12;
            drawTextF(&scope.tmp, instances, {2, offset_y},
                      "Camera: x = %.2f, y = %.2f, z = %.2f", world.camera.pos.x,
                      world.camera.pos.y, world.camera.pos.z);
            offset_y += 12;

            drawTextF(&scope.tmp, instances, {2, offset_y}, "UI instances: %d/%d", ui.last_len,
                      ui.MAX_INSTANCES);
            offset_y += 12;
            drawTextF(&scope.tmp, instances, {2, offset_y}, "World instances: %d/%d",
                      world.last_len, world.MAX_INSTANCES);
            offset_y += 12;
        }
    }

    static constexpr int MAP_SIZE = 16;

    static inline bool checkFaceVisible(u8 map[MAP_SIZE][MAP_SIZE][MAP_SIZE], Vector3i pos) {
        if (pos.x < 0 or pos.y < 0 or pos.z < 0) return true;
        if (pos.x >= MAP_SIZE or pos.y >= MAP_SIZE or pos.z >= MAP_SIZE) return true;
        return map[pos.x][pos.y][pos.z] == 0;
    }

    void updateWorld(Fixed<World::Instance> *instances) {
        float yaw = is_key_just_released(Key::kp_6) - is_key_just_released(Key::kp_4);
        float pitch = is_key_just_released(Key::kp_2) - is_key_just_released(Key::kp_8);
        float roll = is_key_just_released(Key::kp_7) - is_key_just_released(Key::kp_9);
        world.camera.yaw += yaw * 5;
        world.camera.pitch += pitch * 5;
        world.camera.roll += roll * 5;

        Vector3f velocity = {
            float(is_key_just_released(Key::right)) - float(is_key_just_released(Key::left)),
            float(is_key_just_released(Key::up)) - float(is_key_just_released(Key::down)),
            float(is_key_just_released(Key::pageup)) - float(is_key_just_released(Key::pagedown)),
        };

        world.camera.pos += velocity;

        u8 map[MAP_SIZE][MAP_SIZE][MAP_SIZE] = {};

        for (size_t x = 0; x < MAP_SIZE; x++) {
            for (size_t y = 0; y < MAP_SIZE; y++) {
                for (size_t z = 0; z < 1; z++) {
                    map[x][y][z] = (x + y + z) % 4 + 1;
                }
            }
        }

        for (int x = 0; x < MAP_SIZE; x++) {
            for (int y = 0; y < MAP_SIZE; y++) {
                for (int z = 0; z < MAP_SIZE; z++) {
                    FRectangle texture = {};
                    if (map[x][y][z] == 1) {
                        texture = sprites.get("dirt");
                    } else if (map[x][y][z] == 2) {
                        texture = sprites.get("grass");
                    } else if (map[x][y][z] == 2) {
                        texture = sprites.get("water");
                    } else if (map[x][y][z] == 3) {
                        texture = sprites.get("wood_floor");
                    } else if (map[x][y][z] == 4) {
                        texture = sprites.get("wall");
                    } else {
                        continue;
                    }
                    const Vector3i axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
                    for (int axis = 0; axis < 3; axis++)
                        for (int i = 0; i < 2; i++) {
                            const auto pos = Vector3i(x, y, z);
                            const auto offset = axes[axis] * (1 - i * 2);
                            if (!checkFaceVisible(map, pos + offset)) continue;
                            instances->append({Vector3f(offset) * 0.5F + pos,
                                               {1, 1},
                                               texture,
                                               WHITE,
                                               Face(axis * 2 + i)});
                        }
                }
            }
        }

        instances->append(
            {{0, 0, 1.5}, {1, 2}, sprites.get("character"), WHITE, Face::billboard});
    }
} engine;

static struct Game {
    static void init(Engine *unagi);
    static void deinit(Engine *unagi);
    static Vector2f update(Engine *unagi, Fixed<UI::Instance> *instances);
    static void updateUI(Engine *unagi, Fixed<UI::Instance> *instance);
} game;

#include "game.cpp"

struct AtlasItem {
    Slice<const char> name;
    SDL_Surface *surface;
};

void resize(int width, int height) {
    engine.screen.x = width;
    engine.screen.y = height;

    world.resize(engine.screen);
    ui.resize(engine.screen);

    if (depth) SDL_ReleaseGPUTexture(device, depth);
    SDL_GPUTextureCreateInfo depth_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = uint(width),
        .height = uint(height),
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    depth = SDL_CreateGPUTexture(device, &depth_info);
    SDL_assert(depth);
}

SDL_AppResult SDL_AppInit([[maybe_unused]] void **appstate, [[maybe_unused]] int argc,
                          [[maybe_unused]] char *argv[]) {
    arena = Arena::init(KB(5));

    const char *name = "tower";
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata(name, "0.4.0", "cynumini.tower");

    SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));

    engine.screen = {1280, 720};

    window = SDL_CreateWindow(name, engine.screen.x, engine.screen.y, SDL_WINDOW_RESIZABLE);
    SDL_CHECK(window);

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, true, 0);
    SDL_CHECK(device);

    const char *driver = SDL_GetGPUDeviceDriver(device);

    SDL_GPUShaderFormat shader_format = SDL_strcmp(driver, "vulkan") == 0
                                            ? SDL_GPU_SHADERFORMAT_SPIRV
                                            : SDL_GPU_SHADERFORMAT_DXIL;

    SDL_CHECK(SDL_ClaimWindowForGPUDevice(device, window));

    {
        const SDL_GPUSamplerCreateInfo createinfo{};
        sampler = SDL_CreateGPUSampler(device, &createinfo);
    }
    SDL_CHECK(sampler);

    SDL_CHECK(Texture::create(device, {4096, 4096}, &atlas));

    ui.init(shader_format);
    world.init(shader_format);

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    defer(SDL_SubmitGPUCommandBuffer(command_buffer));
    SDL_CHECK(command_buffer);

    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    defer(SDL_EndGPUCopyPass(copy_pass));

    ui.uploadBuffer(copy_pass);
    world.uploadBuffer(copy_pass);

    {
        ScopeArena scope(&arena);
        int mods_len = 0;
        char **mods = SDL_GlobDirectory("mods/", "*", SDL_GLOB_CASEINSENSITIVE, &mods_len);
        defer(SDL_free((void *)mods));
        SDL_CHECK(mods);

        engine.mods.init(&arena, mods_len);

        for (char *const *it = mods; *it; it++) {
            engine.mods.append(&arena, {arena.dupeConst(*it), {}});
        }

        for (auto &mod : engine.mods) {
            auto path = scope.tmp.allocPrintZ("mods/%*s/", int(mod.name.len), mod.name.ptr);

            int items_len = 0;

            char **items =
                SDL_GlobDirectory(path.ptr, "*.png", SDL_GLOB_CASEINSENSITIVE, &items_len);
            defer(SDL_free((void *)items));
            SDL_CHECK(items);

            mod.items.init(&arena, items_len);

            for (char *const *it = items; *it; it++) {
                mod.items.append(&arena, arena.dupeConst(getStem(*it)));
            }
        }
    }

    {
        ScopeArena scope(&arena);

        int files_len = 0;
        char **files =
            SDL_GlobDirectory("resources/", "*.png", SDL_GLOB_CASEINSENSITIVE, &files_len);
        defer(SDL_free((void *)files));
        SDL_CHECK(files);

        int mods_files_len = 0;
        char *const *mods_files =
            SDL_GlobDirectory("mods/", "*/*.png", SDL_GLOB_CASEINSENSITIVE, &mods_files_len);
        SDL_CHECK(mods_files);

        engine.sprites =
            HashMap<FRectangle>::init(&arena, size_t(files_len + mods_files_len) * 2);

        auto atlas_items = Dynamic<AtlasItem>::init(&scope.tmp, files_len + mods_files_len);
        defer(for (auto &item : atlas_items) SDL_DestroySurface(item.surface));

        for (char *const *it = files; *it; it++) {
            const Slice<const char> file = arena.dupeConst(getStem(*it));
            const SliceZ<char> path = scope.tmp.allocPrintZ("resources/%s", *it);
            auto *surface = SDL_LoadPNG(path.ptr);
            SDL_CHECK(surface);
            scope.tmp.free(path);

            if (surface->format != SDL_PIXELFORMAT_RGBA32) {
                SDL_Surface *old_surface = surface;
                surface = SDL_ConvertSurface(old_surface, SDL_PIXELFORMAT_RGBA32);
                SDL_DestroySurface(old_surface);
                SDL_CHECK(surface);
            }

            atlas_items.append(&scope.tmp, {file, surface});
        }

        for (char *const *it = mods_files; *it; it++) {
            const Slice<const char> file = arena.dupeConst(getStem(*it));
            const SliceZ<char> path = scope.tmp.allocPrintZ("mods/%s", *it);
            auto *surface = SDL_LoadPNG(path.ptr);
            SDL_CHECK(surface);
            scope.tmp.free(path);

            if (surface->format != SDL_PIXELFORMAT_RGBA32) {
                SDL_Surface *old_surface = surface;
                surface = SDL_ConvertSurface(old_surface, SDL_PIXELFORMAT_RGBA32);
                SDL_DestroySurface(old_surface);
                SDL_CHECK(surface);
            }

            atlas_items.append(&scope.tmp, {file, surface});
        }

        atlas_items.sort([](const void *a, const void *b) {
            auto *s_a = (AtlasItem *)a;
            auto *s_b = (AtlasItem *)b;
            if (s_a->surface->h > s_b->surface->h) return -1;
            if (s_a->surface->h < s_b->surface->h) return 1;
            return 0;
        });

        uint x_offset = 0;
        uint y_offset = 0;
        int y_max = 0;

        for (auto &[name, surface] : atlas_items) {
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

            engine.sprites.put(
                &arena, name,
                {{float(x_offset), float(y_offset)}, float(surface->w), float(surface->h)});

            y_max = max(surface->h, y_max);

            atlas.uploadToGPU(copy_pass, transfer_buffer,
                              {{x_offset, y_offset}, (uint)surface->w, (uint)surface->h});

            x_offset += surface->w;
        }
    }

    instance_transfer_buffer =
        createGPUTransferBuffer(device, max(sizeof(UI::Instance) * ui.MAX_INSTANCES,
                                            sizeof(World::Instance) * world.MAX_INSTANCES));
    SDL_CHECK(instance_transfer_buffer);

    resize(engine.screen.x, engine.screen.y);

    engine.keyboard_state = SDL_GetKeyboardState(0);
    engine.default_font.init(engine.sprites.get("font"));

    game.init(&engine);

    time.frequency = float(SDL_GetPerformanceFrequency());
    time.counter = SDL_GetPerformanceCounter();

    {
        SDL_Time ticks; // NOLINT
        SDL_assert(SDL_GetCurrentTime(&ticks));
        SDL_srand(ticks);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event) {
    switch (event->type) {
    case SDL_EVENT_WINDOW_RESIZED: {
        resize(event->window.data1, event->window.data2);
        break;
    }
    case SDL_EVENT_QUIT: {
        return SDL_APP_SUCCESS;
    }
    case SDL_EVENT_KEY_DOWN:
        if (!event->key.repeat) {
            engine.key_state[event->key.scancode] = KeyState::pressed;
        }
        break;
    case SDL_EVENT_KEY_UP:
        engine.key_state[event->key.scancode] = KeyState::released;
        break;
    default: {
        break;
    }
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *appstate) {
    if (engine.running) return SDL_APP_SUCCESS;

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    defer(SDL_SubmitGPUCommandBuffer(command_buffer));
    SDL_CHECK(command_buffer);

    {
        auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        defer(SDL_EndGPUCopyPass(copy_pass));

        {
            auto instances = world.beginUpload(instance_transfer_buffer);
            defer(world.endUpload(copy_pass, instance_transfer_buffer, instances.len));

            engine.updateWorld(&instances);

            for (auto &instance : instances) instance.uv /= 4096.0F;
        }

        {
            auto instances = ui.beginUpload(instance_transfer_buffer);
            defer(ui.endUpload(copy_pass, instance_transfer_buffer, instances.len));

            game.updateUI(&engine, &instances);
            engine.updateUI(&instances);

            for (auto &instance : instances) instance.uv /= 4096.0F;
        }
    }

    SDL_GPUTexture *swapchain_texture = 0;

    SDL_CHECK(
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, 0, 0));

    if (swapchain_texture) {
        SDL_GPUColorTargetInfo color_target_info = {};
        color_target_info.texture = swapchain_texture;
        color_target_info.clear_color = {
            .r = float(engine.clear_color.r) / 255.0F,
            .g = float(engine.clear_color.g) / 255.0F,
            .b = float(engine.clear_color.b) / 255.0F,
            .a = float(engine.clear_color.a) / 255.0F,
        };
        color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target_info.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {
            .texture = depth,
            .clear_depth = 1.0F,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
        };

        auto *render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1,
                                                   &depth_stencil_target_info);
        defer(SDL_EndGPURenderPass(render_pass));

        world.draw(command_buffer, render_pass);
        ui.draw(command_buffer, render_pass);
    }

    memset(engine.key_state, 0, sizeof(engine.key_state));

    auto prev = time.counter;
    time.counter = SDL_GetPerformanceCounter();
    time.frames += 1;
    engine.dt = float(time.counter - prev) / time.frequency;
    time.seconds += engine.dt;
    time.ms = engine.dt * 1000;
    if (time.seconds >= 0.5F) {
        time.fps = (u16)SDL_roundf((float)time.frames / time.seconds);
        time.frames = 0;
        time.seconds = 0;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit([[maybe_unused]] void *appstate, [[maybe_unused]] SDL_AppResult result) {
    game.deinit(&engine);

    SDL_ReleaseGPUTransferBuffer(device, instance_transfer_buffer);

    SDL_ReleaseGPUTexture(device, atlas.ptr);
    SDL_ReleaseGPUTexture(device, depth);

    world.deinit();
    ui.deinit();

    SDL_ReleaseGPUSampler(device, sampler);
    SDL_ReleaseWindowFromGPUDevice(device, window);

    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);

    arena.deinit();
}
