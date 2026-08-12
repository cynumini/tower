#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#define SDL_ENSURE(check, message)                                                                                                                             \
    do {                                                                                                                                                       \
        if (!(check)) {                                                                                                                                        \
            SDL_Log("Couldn't %s: %s", (message), SDL_GetError());                                                                                             \
            abort();                                                                                                                                           \
        }                                                                                                                                                      \
    } while (false)

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
};

constexpr Vector2 SCREEN{640, 360};

struct AppState {
    SDL_Window *window;

    float main_scale;

    AppState() {
        main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        window = SDL_CreateWindow("Tower", int(SCREEN.x * main_scale), int(SCREEN.y * main_scale),
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        SDL_ENSURE(window, "create window");

    }
    ~AppState() { SDL_DestroyWindow(window); }
};

SDL_AppResult SDL_AppInit(void **app_state, [[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata("Tower", "0.3.0", "cynumini.tower");

    SDL_ENSURE(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD), "initialize SDL");

    *app_state = new AppState();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *app_state) { return SDL_APP_SUCCESS; }
SDL_AppResult SDL_AppEvent([[maybe_unused]] void *app_state, [[maybe_unused]] SDL_Event *event) { return SDL_APP_CONTINUE; }
void SDL_AppQuit(void *app_state, [[maybe_unused]] SDL_AppResult result) { delete static_cast<AppState *>(app_state); }
