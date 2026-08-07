#ifndef SDL_HPP
#define SDL_HPP

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#define SDL_ENSURE(check, message)                                      \
    do {                                                                                                                                                       \
        if (!(check)) {                                                                                                                                        \
            SDL_Log("Couldn't %s: %s", (message), SDL_GetError());                                                                                             \
            abort();                                                                                                                                           \
        }                                                                                                                                                      \
    } while (false)

#endif // SDL_HPP
