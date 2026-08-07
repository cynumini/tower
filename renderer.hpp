#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "math.hpp"
#include "root.hpp"
#include <sdl.hpp>

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

#endif // RENDERER_HPP
