#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;

SDL_AppResult SDL_AppInit([[maybe_unused]] void **appstate, [[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    SDL_SetAppMetadata("Tower", "0.1", "cynumini.tower");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    window = SDL_CreateWindow("Tower", 1280, 720, 0);

    if (!window) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent([[maybe_unused]] void *appstate, SDL_Event *event) {
    switch (event->type) {
    case SDL_EVENT_QUIT: {
        return SDL_APP_SUCCESS;
    }
    case SDL_EVENT_KEY_DOWN: {
        if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
        break;
    }
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate([[maybe_unused]] void *appstate) { return SDL_APP_CONTINUE; }

void SDL_AppQuit([[maybe_unused]] void *appstate, [[maybe_unused]] SDL_AppResult result) {}
