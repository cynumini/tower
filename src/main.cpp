#include "engine.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#include "rect.hpp"
#include "skn_sdl.cpp"

#include "game.hpp"

static Game game;

static SDL_GPUDevice *device;
static SDL_GPUSampler *sampler;
static SDL_GPUTexture *depth;
static SDL_GPUTransferBuffer *instance_transfer_buffer;

static Texture atlas;

UI ui;
World world;
Engine engine; // NOLINT

struct AtlasItem {
    std::string name;
    SDL_Surface *surface;
};

static glm::ivec2 last_size;

void resize(glm::ivec2 size) {
    if (size == last_size) return;
    world.resize(glm::vec2(size));
    ui.resize(glm::vec2(size));

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

    const char *name = "tower";
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata(name, "0.5.0", "cynumini.tower");

    SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));



    engine.screen = {640, 360};
    engine.window = SDL_CreateWindow(name, engine.screen.x, engine.screen.y, SDL_WINDOW_RESIZABLE);
    SDL_CHECK(engine.window);

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, true, 0);
    SDL_CHECK(device);

    const char *driver = SDL_GetGPUDeviceDriver(device);

    SDL_GPUShaderFormat shader_format = SDL_strcmp(driver, "vulkan") == 0
                                            ? SDL_GPU_SHADERFORMAT_SPIRV
                                            : SDL_GPU_SHADERFORMAT_DXIL;

    SDL_CHECK(SDL_ClaimWindowForGPUDevice(device, engine.window));

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

    ui.setup(engine.window, device, shader_format);
    world.setup(engine.window, device, shader_format);

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_CHECK(command_buffer);

    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    ui.uploadStaticBuffers(device, copy_pass);
    world.uploadStaticBuffers(device, copy_pass);

    {
        int mods_len = 0;
        char **mods = SDL_GlobDirectory("mods/", "*", SDL_GLOB_CASEINSENSITIVE, &mods_len);
        SDL_CHECK(mods);

        engine.mods.reserve(mods_len);

        for (char *const *it = mods; *it; it++) {
            engine.mods.emplace_back(*it, std::vector<std::string_view>{});
        }

        for (auto &mod : engine.mods) {
            auto path = std::format("mods/{}/", mod.name);

            int items_len = 0;

            char **items =
                SDL_GlobDirectory(path.c_str(), "*.png", SDL_GLOB_CASEINSENSITIVE, &items_len);
            SDL_CHECK(items);

            mod.items.reserve(items_len);

            for (char *const *it = items; *it; it++) {
                mod.items.emplace_back(*it);
            }
            SDL_free((void *)items);
        }
        SDL_free((void *)mods);
    }

    {

        int files_len = 0;
        char **files =
            SDL_GlobDirectory("resources/", "*.png", SDL_GLOB_CASEINSENSITIVE, &files_len);
        SDL_CHECK(files);

        int mods_files_len = 0;
        char *const *mods_files =
            SDL_GlobDirectory("mods/", "*/*.png", SDL_GLOB_CASEINSENSITIVE, &mods_files_len);
        SDL_CHECK(mods_files);

        engine.sprites.reserve(size_t(files_len + mods_files_len) * 2);

        auto atlas_items = std::vector<AtlasItem>();
        atlas_items.reserve(files_len + mods_files_len);

        for (char *const *it = files; *it; it++) {
            // auto file = getStem(*it);
            auto file = std::filesystem::path(*it).stem();
            const auto path = std::format("resources/{}", *it);
            auto *surface = SDL_LoadPNG(path.c_str());
            SDL_CHECK(surface);

            if (surface->format != SDL_PIXELFORMAT_RGBA32) {
                SDL_Surface *old_surface = surface;
                surface = SDL_ConvertSurface(old_surface, SDL_PIXELFORMAT_RGBA32);
                SDL_DestroySurface(old_surface);
                SDL_CHECK(surface);
            }

            atlas_items.emplace_back(std::string(file), surface);
        }

        for (char *const *it = mods_files; *it; it++) {
            // auto file = getStem(*it);
            auto file = std::filesystem::path(*it).stem();
            auto path = std::format("mods/{}", *it);
            auto *surface = SDL_LoadPNG(path.c_str());
            SDL_CHECK(surface);

            if (surface->format != SDL_PIXELFORMAT_RGBA32) {
                SDL_Surface *old_surface = surface;
                surface = SDL_ConvertSurface(old_surface, SDL_PIXELFORMAT_RGBA32);
                SDL_DestroySurface(old_surface);
                SDL_CHECK(surface);
            }

            atlas_items.emplace_back(std::string(file), surface);
        }

        std::ranges::sort(atlas_items, [](const AtlasItem &a, const AtlasItem &b) {
            return a.surface->h > b.surface->h;
        });

        uint x_offset = 0;
        uint y_offset = 0;
        int y_max = 0;

        for (auto &[name, surface] : atlas_items) {
            auto *transfer_buffer =
                createGPUTransferBuffer(device, sizeof(Color) * surface->w * surface->h);
            SDL_CHECK(transfer_buffer);

            {
                uint8_t *memory =
                    (uint8_t *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
                SDL_CHECK(memory);

                const auto *src = (uint8_t *)surface->pixels;
                const auto row_bytes = size_t(surface->w) * 4;
                if (row_bytes == size_t(surface->pitch)) {
                    SDL_memcpy(memory, src, row_bytes * surface->h);
                } else {
                    for (size_t y = 0; y < size_t(surface->h); y++) {
                        SDL_memcpy(memory + (y * row_bytes), src + (y * surface->pitch),
                                   row_bytes);
                    }
                }
                SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
            }
            if ((x_offset + surface->w) > uint(atlas.w)) {
                x_offset = 0;
                y_offset += y_max;
                y_max = 0;
            }
            SDL_assert(x_offset + uint(surface->w) <= uint(atlas.w));
            SDL_assert(y_offset + uint(surface->h) <= uint(atlas.h));

            engine.sprites.emplace(name, toFRect({float(x_offset), float(y_offset)},
                                                 {float(surface->w), float(surface->h)}));

            y_max = std::max(surface->h, y_max);

            atlas.uploadToGPU(copy_pass, transfer_buffer, x_offset, y_offset, surface->w,
                              surface->h);

            x_offset += surface->w;
            SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        }
        SDL_free((void *)files);
    }

    instance_transfer_buffer =
        createGPUTransferBuffer(device, std::max(sizeof(UIInstance) * ui.MAX_INSTANCES,
                                                 sizeof(WorldInstance) * world.MAX_INSTANCES));
    SDL_CHECK(instance_transfer_buffer);

    resize(engine.screen);

    engine.keyboard_state = SDL_GetKeyboardState(0);
    {
        auto texture = engine.sprites.at("font");
        engine.default_font =
            createDefaultFont({.x = texture.x, .y = texture.y, .w = texture.w, .h = texture.h});
    }

    initGame(game);

    engine.times.frequency = float(SDL_GetPerformanceFrequency());
    engine.times.counter = SDL_GetPerformanceCounter();

    {
        SDL_Time ticks; // NOLINT
        SDL_assert(SDL_GetCurrentTime(&ticks));
        SDL_srand(ticks);
    }

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

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
        engine.mouse = {int(event->motion.x), int(event->motion.y)};
        engine.mouse_rel = {int(event->motion.xrel), int(event->motion.yrel)};
    } break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (event->button.button == SDL_BUTTON_LEFT) {
            engine.mouse_button_state[int(MouseButton::left)] = ButtonState::pressed;
        }
        if (event->button.button == SDL_BUTTON_MIDDLE) {
            engine.mouse_button_state[int(MouseButton::middle)] = ButtonState::pressed;
        }
        if (event->button.button == SDL_BUTTON_RIGHT) {
            engine.mouse_button_state[int(MouseButton::right)] = ButtonState::pressed;
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
    SDL_CHECK(command_buffer);

    {
        auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

        {
            auto instances = world.beginUpload(device, instance_transfer_buffer);
            updateGame(game, &instances, world.logical);

            for (auto &instance : instances.squares) instance.uv /= 4096.0F;
            for (auto &instance : instances.triangles) instance.uv /= 4096.0F;

            world.endUpload(device, copy_pass, instance_transfer_buffer, &instances);
        }

        {
            auto upload = ui.beginUpload(device, instance_transfer_buffer);

            updateGameUI(game, upload.instances, glm::ivec2(ui.logical));
            updateEngineUI(upload.instances);

            for (auto &instance : upload.instances) instance.uv /= 4096.0F;

            ui.endUpload(device, copy_pass, instance_transfer_buffer, upload.instances.size);
        }
        SDL_EndGPUCopyPass(copy_pass);
    }

    SDL_GPUTexture *swapchain_texture = 0;

    uint width = 0, height = 0;
    SDL_CHECK(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, engine.window, &swapchain_texture,
                                                    &width, &height));
    resize({int(width), int(height)});

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

        world.draw(command_buffer, render_pass, atlas, sampler, &engine.camera);
        ui.draw(command_buffer, render_pass, atlas, sampler);

        SDL_EndGPURenderPass(render_pass);
    }

    engine.key_state = {};
    engine.mouse_button_state = {};

    auto prev = engine.times.counter;
    engine.times.counter = SDL_GetPerformanceCounter();
    engine.times.frames += 1;
    engine.dt = float(engine.times.counter - prev) / engine.times.frequency;
    engine.times.seconds += engine.dt;
    engine.times.ms = engine.dt * 1000;
    if (engine.times.seconds >= 0.5F) {
        engine.times.fps =
            (uint16_t)SDL_roundf((float)engine.times.frames / engine.times.seconds);
        engine.times.frames = 0;
        engine.times.seconds = 0;
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);

    engine.mouse_rel = {};
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit([[maybe_unused]] void *appstate, [[maybe_unused]] SDL_AppResult result) {
    deinitGame(game);

    SDL_ReleaseGPUTransferBuffer(device, instance_transfer_buffer);

    SDL_ReleaseGPUTexture(device, atlas.ptr);
    SDL_ReleaseGPUTexture(device, depth);

    world.release(device);
    ui.release(device);

    SDL_ReleaseGPUSampler(device, sampler);
    SDL_ReleaseWindowFromGPUDevice(device, engine.window);

    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(engine.window);
}
