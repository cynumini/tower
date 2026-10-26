#include <skn_sdl.cpp>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#include "shared.cpp"

#include "ui_pipeline.cpp"
#include "world_pipeline.cpp"

#include "game.hpp"

static Game game;
Arena arena;

static SDL_GPUDevice *device;
static SDL_Window *window;
static SDL_GPUSampler *sampler;
static SDL_GPUTexture *depth;
static SDL_GPUTransferBuffer *instance_transfer_buffer;

static Texture atlas;

void Engine::log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
    va_end(args);
}

int Engine::rand(int n) const { return SDL_rand(n); }

Engine engine;

static struct Times {
    u64 counter;
    u64 frames;
    f32 seconds;
    f32 frequency;
    u16 fps;
    f32 ms;
} times;

void Engine::update([[maybe_unused]] WorldInstances *instances, [[maybe_unused]] Vec2 logical) {
    if (isKeyJustPressed(Key::escape)) running = true;
    if (isKeyJustPressed(Key::f3)) debug_mode = !debug_mode;
}

void Engine::updateUI(FixedVector<UIInstance> &instances, [[maybe_unused]] Vec2i render_size) {
    if (debug_mode) {
        ScopeArena scope(&arena);
        f32 offset_y = 2.0F;

        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "FPS: %d", times.fps);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "%.2fms", times.ms);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "Screen: %dx%d", screen.x, screen.y);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "Mouse: %dx%d", mouse.x, mouse.y);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "Pitch: %.0f", camera.pitch);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "Yaw: %.0f", camera.yaw);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "Roll: %.0f", camera.roll);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "Camera: x = %.2f, y = %.2f, z = %.2f",
                  camera.pos.x, camera.pos.y, camera.pos.z);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "UI instances: %d/%d",
                  ui.prev_instances_len, ui.MAX_INSTANCES);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "World square instances: %d/%d",
                  world.prev_squares_len, world.MAX_INSTANCES / 2);
        offset_y += 12;
        drawTextF(&scope.tmp, instances, {2.0F, offset_y}, "World triangle instances: %d/%d",
                  world.prev_triangles_len, world.MAX_INSTANCES / 2);
        offset_y += 12;
    }
}

struct AtlasItem {
    OldSlice<const char> name;
    SDL_Surface *surface;
};

static Vec2i last_size;

void resize(Vec2i size) {
    if (size == last_size) return;
    world.resize(size);
    ui.resize(size);

    if (depth) SDL_ReleaseGPUTexture(device, depth);
    SDL_GPUTextureCreateInfo depth_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = uint(size.x),
        .height = uint(size.y),
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    depth = SDL_CreateGPUTexture(device, &depth_info);
    SDL_assert(depth);

    last_size = size;
}

SDL_AppResult SDL_AppInit([[maybe_unused]] void **appstate, [[maybe_unused]] int argc,
                          [[maybe_unused]] char *argv[]) {
    arena = Arena::init(MB(1));

    const char *name = "tower";
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata(name, "0.5.0", "cynumini.tower");

    SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));

    engine.screen = {640, 360};
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
        SDL_GPUSamplerCreateInfo createinfo = {};
        createinfo.min_filter = SDL_GPU_FILTER_NEAREST;
        createinfo.mag_filter = SDL_GPU_FILTER_NEAREST;
        createinfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        createinfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        createinfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        sampler = SDL_CreateGPUSampler(device, &createinfo);
    }
    SDL_CHECK(sampler);

    atlas = Texture::create(device, 4096, 4096);

    ui.setup(window, device, shader_format);
    world.setup(window, device, shader_format);

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    defer(SDL_SubmitGPUCommandBuffer(command_buffer));
    SDL_CHECK(command_buffer);

    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    defer(SDL_EndGPUCopyPass(copy_pass));

    ui.uploadStaticBuffers(device, copy_pass);
    world.uploadStaticBuffers(device, copy_pass);

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

        engine.sprites = HashMap<skn::Rect>::init(&arena, size_t(files_len + mods_files_len) * 2);

        auto atlas_items = Dynamic<AtlasItem>::init(&scope.tmp, files_len + mods_files_len);
        defer(for (auto &item : atlas_items) SDL_DestroySurface(item.surface));

        for (char *const *it = files; *it; it++) {
            const OldSlice<const char> file = arena.dupeConst(getStem(*it));
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
            const OldSlice<const char> file = arena.dupeConst(getStem(*it));
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
                createGPUTransferBuffer(device, sizeof(skn::Color) * surface->w * surface->h);
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
            if ((x_offset + surface->w) > uint(atlas.w)) {
                x_offset = 0;
                y_offset += y_max;
                y_max = 0;
            }
            SDL_assert(x_offset + uint(surface->w) <= uint(atlas.w));
            SDL_assert(y_offset + uint(surface->h) <= uint(atlas.h));

            engine.sprites.put(
                &arena, name,
                {{f32(x_offset), f32(y_offset)}, {f32(surface->w), f32(surface->h)}});

            y_max = std::max(surface->h, y_max);

            atlas.uploadToGPU(copy_pass, transfer_buffer, x_offset, y_offset, surface->w,
                              surface->h);

            x_offset += surface->w;
        }
    }

    instance_transfer_buffer =
        createGPUTransferBuffer(device, std::max(sizeof(UIInstance) * ui.MAX_INSTANCES,
                                                 sizeof(WorldInstance) * world.MAX_INSTANCES));
    SDL_CHECK(instance_transfer_buffer);

    resize(engine.screen);

    engine.keyboard_state = SDL_GetKeyboardState(0);
    {
        auto texture = engine.sprites.get("font");
        engine.default_font =
            createDefaultFont({.x = texture.x, .y = texture.y, .w = texture.w, .h = texture.h});
    }

    initGame(game, engine);

    times.frequency = f32(SDL_GetPerformanceFrequency());
    times.counter = SDL_GetPerformanceCounter();

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
        engine.screen = {event->window.data1, event->window.data2};
        resize(engine.screen);
        break;
    }
    case SDL_EVENT_QUIT: {
        return SDL_APP_SUCCESS;
    }
    case SDL_EVENT_KEY_DOWN:
        if (!event->key.repeat) {
            engine.key_state[event->key.scancode] = ButtonState::pressed;
        }
        break;
    case SDL_EVENT_KEY_UP:
        engine.key_state[event->key.scancode] = ButtonState::released;
        break;
    case SDL_EVENT_MOUSE_MOTION: {
        engine.mouse = {event->motion.x, event->motion.y};
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (event->button.button == SDL_BUTTON_LEFT) {
            engine.mouse_button_state[MouseButton::left] = ButtonState::pressed;
        }
        if (event->button.button == SDL_BUTTON_MIDDLE) {
            engine.mouse_button_state[MouseButton::middle] = ButtonState::pressed;
        }
        if (event->button.button == SDL_BUTTON_RIGHT) {
            engine.mouse_button_state[MouseButton::right] = ButtonState::pressed;
        }
        break;
    }
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
            auto instances = world.beginUpload(device, instance_transfer_buffer);
            defer(world.endUpload(device, copy_pass, instance_transfer_buffer, &instances));

            updateGame(game, engine, &instances, world.logical);
            engine.update(&instances, world.logical);

            for (auto &instance : instances.squares) instance.uv /= 4096.0F;
            for (auto &instance : instances.triangles) instance.uv /= 4096.0F;
        }

        {
            auto upload = ui.beginUpload(device, instance_transfer_buffer);
            defer(
                ui.endUpload(device, copy_pass, instance_transfer_buffer, upload.instances.size));

            updateGameUI(game, engine, upload.instances, ui.logical);
            engine.updateUI(upload.instances, ui.logical);

            for (auto &instance : upload.instances) instance.uv /= 4096.0F;
        }
    }

    SDL_GPUTexture *swapchain_texture = 0;

    uint width = 0, height = 0;
    SDL_CHECK(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture,
                                                    &width, &height));
    resize({width, height});

    if (swapchain_texture) {
        SDL_GPUColorTargetInfo color_target_info = {};
        color_target_info.texture = swapchain_texture;
        color_target_info.clear_color = {
            .r = f32(engine.clear_color.r) / 255.0F,
            .g = f32(engine.clear_color.g) / 255.0F,
            .b = f32(engine.clear_color.b) / 255.0F,
            .a = f32(engine.clear_color.a) / 255.0F,
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

        world.draw(command_buffer, render_pass, atlas, sampler, &engine.camera);
        ui.draw(command_buffer, render_pass, atlas, sampler);
    }

    memset(engine.key_state, 0, sizeof(engine.key_state));
    memset(engine.mouse_button_state, 0, sizeof(engine.mouse_button_state));

    auto prev = times.counter;
    times.counter = SDL_GetPerformanceCounter();
    times.frames += 1;
    engine.dt = f32(times.counter - prev) / times.frequency;
    times.seconds += engine.dt;
    times.ms = engine.dt * 1000;
    if (times.seconds >= 0.5F) {
        times.fps = (u16)SDL_roundf((f32)times.frames / times.seconds);
        times.frames = 0;
        times.seconds = 0;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit([[maybe_unused]] void *appstate, [[maybe_unused]] SDL_AppResult result) {
    deinitGame(game, engine);

    SDL_ReleaseGPUTransferBuffer(device, instance_transfer_buffer);

    SDL_ReleaseGPUTexture(device, atlas.ptr);
    SDL_ReleaseGPUTexture(device, depth);

    world.release(device);
    ui.release(device);

    SDL_ReleaseGPUSampler(device, sampler);
    SDL_ReleaseWindowFromGPUDevice(device, window);

    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);

    arena.deinit();
}
