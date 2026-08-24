#pragma once

#include <SDL3/SDL.h>

#include "renderer.hpp"

enum class Key : usize {
    a = 4,
    b = 5,
    c = 6,
    d = 7,
    e = 8,
    f = 9,
    g = 10,
    h = 11,
    i = 12,
    j = 13,
    k = 14,
    l = 15,
    m = 16,
    n = 17,
    o = 18,
    p = 19,
    q = 20,
    r = 21,
    s = 22,
    t = 23,
    u = 24,
    v = 25,
    w = 26,
    x = 27,
    y = 28,
    z = 29,

    num1 = 30,
    num2 = 31,
    num3 = 32,
    num4 = 33,
    num5 = 34,
    num6 = 35,
    num7 = 36,
    num8 = 37,
    num9 = 38,
    num0 = 39,

    escape = 41,
    space = 44,

    enter = 40,

    f1 = 58,
    f2 = 59,
    f3 = 60,
    f4 = 61,
    f5 = 62,
    f6 = 63,
    f7 = 64,
    f8 = 65,
    f9 = 66,
    f10 = 67,
    f11 = 68,
    f12 = 69,

    del = 76,

    right = 79,
    left = 80,
    down = 81,
    up = 82,

    lshift = 225,

};

struct Engine {
    Renderer *renderer;
    SDL_Window *window;
    const bool *keyboard_state = nullptr;

    bool show_debug_menu = false;
    bool fullscreen = false;
    bool should_close = false;

    f32 dt{};
    u64 previous;

    Array<bool, SDL_SCANCODE_COUNT> pressed = {};
    Array<bool, SDL_SCANCODE_COUNT> pressed_repeat = {};

    char text[256];

    bool isKeyDown(Key key) { return keyboard_state[static_cast<usize>(key)]; }
    bool isKeyPressed(Key key) { return pressed[static_cast<usize>(key)]; }
    bool isKeyPressedRepeat(SDL_Scancode key) { return pressed_repeat[key]; }
    glm::vec2 getMousePosition() {
        glm::vec2 result;
        SDL_GetMouseState(&result.x, &result.y);
        return result;
    }
};

enum class GameResult { ongoing, success, failure };

GameResult gameInit(Allocator &gpa, Engine &engine, Renderer &renderer);
GameResult gameUpdate(Allocator &gpa, Engine &engine, Renderer &renderer);
void gameDraw(Renderer &renderer);
void gameQuit(Allocator &gpa);
