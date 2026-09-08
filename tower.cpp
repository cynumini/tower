#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

typedef SDL_Point ivec2;

#define SDL_CHECK(cond, desc)                                                                    \
    do {                                                                                         \
        if (!(cond)) {                                                                           \
            SDL_Log("Couldn't " desc ": %s", SDL_GetError());                                    \
            return SDL_APP_FAILURE;                                                              \
        }                                                                                        \
    } while (false);

struct State {
    SDL_Window *window;
    SDL_GPUDevice *device;
    SDL_GPUSampler *sampler;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    const char *name = "tower";
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata(name, "0.3.0", "cynumini.tower");
    SDL_CHECK(SDL_Init(SDL_INIT_VIDEO), "initialize SDL");
    const ivec2 screen = {640, 360};
    // TODO: zeros on init by default
    auto *state = (State *)SDL_malloc(sizeof(State));
    *state = {};
    *appstate = state;
    state->window = SDL_CreateWindow(name, screen.x, screen.y, 0);
    SDL_CHECK(state->window, "create window");
    state->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, 0);
    SDL_CHECK(state->device, "create gpu device");
    SDL_CHECK(SDL_ClaimWindowForGPUDevice(state->device, state->window),
              "claim window for gpu device");
    {
        const SDL_GPUSamplerCreateInfo createinfo{};
        state->sampler = SDL_CreateGPUSampler(state->device, &createinfo);
        SDL_CHECK(state->sampler, "create gpu sampler")
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    // TODO: restore SDL_APP_CONTINUE
    return SDL_APP_SUCCESS;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    auto *state = (State *)appstate;

    SDL_ReleaseGPUSampler(state->device, state->sampler);
    SDL_ReleaseWindowFromGPUDevice(state->device, state->window);

    SDL_DestroyGPUDevice(state->device);
    SDL_DestroyWindow(state->window);
    // TODO: notify about memory leak
    SDL_free(state);
}
