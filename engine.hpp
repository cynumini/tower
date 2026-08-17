#pragma once

#include <SDL3/SDL.h>

#include "renderer.hpp"

constexpr Vector2 SCREEN = {640, 360};

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

    bool isKeyDown(SDL_Scancode key) { return keyboard_state[key]; }
    bool isKeyPressed(SDL_Scancode key) { return pressed[key]; }
    bool isKeyPressedRepeat(SDL_Scancode key) { return pressed_repeat[key]; }
};

enum class GameResult { ongoing, success, failure };

GameResult gameInit(Engine *engine);
GameResult gameUpdate();
void gameDraw();
void gameQuit();
