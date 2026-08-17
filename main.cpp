#include <stdlib.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <imgui.h>

#include "engine.hpp"

[[noreturn]]
static void unreachable(const char *string, usize line) {
    printf("%s:%zu: unreachable\n", string, line);
    abort();
}

#define UNREACHABLE() unreachable(__FILE__, __LINE__)

static SDL_AppResult AppResultFromGameResult(GameResult game_result) {
    switch (game_result) {
    case GameResult::ongoing:
        return SDL_APP_CONTINUE;
    case GameResult::success:
        return SDL_APP_SUCCESS;
    case GameResult::failure:
        return SDL_APP_FAILURE;
    }
    UNREACHABLE();
}

static Engine engine;

SDL_AppResult SDL_AppInit(void **, [[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("Tower", "0.3.0", "cynumini.tower");
    SDL_ENSURE(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD), "initialize SDL");
    f32 scale_factor = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_ENSURE(scale_factor != 0.0f, "get display content scale");
    engine.window = SDL_CreateWindow("Tower", int(SCREEN.x * scale_factor), int(SCREEN.y * scale_factor), SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_ENSURE(engine.window, "create window");
    engine.keyboard_state = SDL_GetKeyboardState(nullptr);
    engine.renderer = new Renderer{engine.window, SCREEN, scale_factor};

    return AppResultFromGameResult(gameInit(&engine));
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *app_state, [[maybe_unused]] SDL_Event *event) {
    ImGui_ImplSDL3_ProcessEvent(event);

    switch (event->type) {
    case SDL_EVENT_QUIT: {
        return SDL_APP_SUCCESS;
    }
    case SDL_EVENT_KEY_DOWN: {
        if (event->key.scancode == SDL_SCANCODE_F3) {
            engine.show_debug_menu = !engine.show_debug_menu;
        }
        if (event->key.scancode == SDL_SCANCODE_F10) {
            return SDL_APP_SUCCESS;
        }
        if (event->key.scancode == SDL_SCANCODE_F11) {
            engine.fullscreen = !engine.fullscreen;
            SDL_SetWindowFullscreen(engine.window, engine.fullscreen);
        }
        if (!event->key.repeat) {
            engine.pressed[event->key.scancode] = true;
        }
        engine.pressed_repeat[event->key.scancode] = true;
        break;
    }
    case SDL_EVENT_WINDOW_RESIZED: {
        f32 width = (f32)event->window.data1;
        f32 height = (f32)event->window.data2;
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
        engine.renderer->base_model_view = Matrix4::orthographic(0, width, height, 0, 0, 1) * Matrix4::translation(x_offset, y_offset, 0);
        break;
    }
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *app_state) {
    u64 now = SDL_GetTicks();
    engine.dt = f32(now - engine.previous) / 1000.f;
    engine.previous = now;
    if (engine.should_close)
        return SDL_APP_SUCCESS;

    auto result = AppResultFromGameResult(gameUpdate());
    if (result != SDL_APP_CONTINUE) {
        return result;
    }
    engine.renderer->frameBegin();

    gameDraw();
    engine.renderer->frameEnd();
    engine.pressed = {};
    engine.pressed_repeat = {};
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *, [[maybe_unused]] SDL_AppResult result) {
    gameQuit();
    delete engine.renderer;
    SDL_DestroyWindow(engine.window);
}
