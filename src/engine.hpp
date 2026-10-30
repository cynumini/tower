#pragma once

#include "color.hpp"
#include "fixed_vector.hpp"
#include "font.hpp"
#include "ui_pipeline.hpp"
#include "world_pipeline.hpp"

enum class Scancode : uint8_t {
    a = SDL_SCANCODE_A,
    b = SDL_SCANCODE_B,
    c = SDL_SCANCODE_C,
    d = SDL_SCANCODE_D,
    e = SDL_SCANCODE_E,
    f = SDL_SCANCODE_F,
    g = SDL_SCANCODE_G,
    h = SDL_SCANCODE_H,
    i = SDL_SCANCODE_I,
    j = SDL_SCANCODE_J,
    k = SDL_SCANCODE_K,
    l = SDL_SCANCODE_L,
    m = SDL_SCANCODE_M,
    n = SDL_SCANCODE_N,
    o = SDL_SCANCODE_O,
    p = SDL_SCANCODE_P,
    q = SDL_SCANCODE_Q,
    r = SDL_SCANCODE_R,
    s = SDL_SCANCODE_S,
    t = SDL_SCANCODE_T,
    u = SDL_SCANCODE_U,
    v = SDL_SCANCODE_V,
    w = SDL_SCANCODE_W,
    x = SDL_SCANCODE_X,
    y = SDL_SCANCODE_Y,
    z = SDL_SCANCODE_Z,

    sc_1 = SDL_SCANCODE_1,
    sc_2 = SDL_SCANCODE_2,
    sc_3 = SDL_SCANCODE_3,
    sc_4 = SDL_SCANCODE_4,
    sc_5 = SDL_SCANCODE_5,
    sc_6 = SDL_SCANCODE_6,
    sc_7 = SDL_SCANCODE_7,
    sc_8 = SDL_SCANCODE_8,
    sc_9 = SDL_SCANCODE_9,
    sc_0 = SDL_SCANCODE_0,

    enter = SDL_SCANCODE_RETURN,
    escape = SDL_SCANCODE_ESCAPE,
    backspace = SDL_SCANCODE_BACKSPACE,
    tab = SDL_SCANCODE_TAB,
    space = SDL_SCANCODE_SPACE,

    minus = SDL_SCANCODE_MINUS,
    equals = SDL_SCANCODE_EQUALS,
    leftbracket = SDL_SCANCODE_LEFTBRACKET,
    rightbracket = SDL_SCANCODE_RIGHTBRACKET,
    backslash = SDL_SCANCODE_BACKSLASH,
    nonushash = SDL_SCANCODE_NONUSHASH,
    semicolon = SDL_SCANCODE_SEMICOLON,
    apostrophe = SDL_SCANCODE_APOSTROPHE,
    grave = SDL_SCANCODE_GRAVE,
    comma = SDL_SCANCODE_COMMA,
    period = SDL_SCANCODE_PERIOD,
    slash = SDL_SCANCODE_SLASH,

    capslock = SDL_SCANCODE_CAPSLOCK,

    f1 = SDL_SCANCODE_F1,
    f2 = SDL_SCANCODE_F2,
    f3 = SDL_SCANCODE_F3,
    f4 = SDL_SCANCODE_F4,
    f5 = SDL_SCANCODE_F5,
    f6 = SDL_SCANCODE_F6,
    f7 = SDL_SCANCODE_F7,
    f8 = SDL_SCANCODE_F8,
    f9 = SDL_SCANCODE_F9,
    f10 = SDL_SCANCODE_F10,
    f11 = SDL_SCANCODE_F11,
    f12 = SDL_SCANCODE_F12,

    printscreen = SDL_SCANCODE_PRINTSCREEN,
    scrolllock = SDL_SCANCODE_SCROLLLOCK,
    pause = SDL_SCANCODE_PAUSE,
    insert = SDL_SCANCODE_INSERT,
    home = SDL_SCANCODE_HOME,
    pageup = SDL_SCANCODE_PAGEUP,
    sc_delete = SDL_SCANCODE_DELETE,
    end = SDL_SCANCODE_END,
    pagedown = SDL_SCANCODE_PAGEDOWN,
    right = SDL_SCANCODE_RIGHT,
    left = SDL_SCANCODE_LEFT,
    down = SDL_SCANCODE_DOWN,
    up = SDL_SCANCODE_UP,

    numlockclear = SDL_SCANCODE_NUMLOCKCLEAR,
    kp_divide = SDL_SCANCODE_KP_DIVIDE,
    kp_multiply = SDL_SCANCODE_KP_MULTIPLY,
    kp_minus = SDL_SCANCODE_KP_MINUS,
    kp_plus = SDL_SCANCODE_KP_PLUS,
    kp_enter = SDL_SCANCODE_KP_ENTER,
    kp_1 = SDL_SCANCODE_KP_1,
    kp_2 = SDL_SCANCODE_KP_2,
    kp_3 = SDL_SCANCODE_KP_3,
    kp_4 = SDL_SCANCODE_KP_4,
    kp_5 = SDL_SCANCODE_KP_5,
    kp_6 = SDL_SCANCODE_KP_6,
    kp_7 = SDL_SCANCODE_KP_7,
    kp_8 = SDL_SCANCODE_KP_8,
    kp_9 = SDL_SCANCODE_KP_9,
    kp_0 = SDL_SCANCODE_KP_0,
    kp_period = SDL_SCANCODE_KP_PERIOD,
};

enum class MouseButton : uint8_t {
    left = SDL_BUTTON_LEFT,
    middle = SDL_BUTTON_MIDDLE,
    right = SDL_BUTTON_RIGHT,
    x1 = SDL_BUTTON_X1,
    x2 = SDL_BUTTON_X2
};

enum class ButtonState : uint8_t { none, pressed, released };

struct Mod {
    std::string name;
    std::vector<std::string_view> items;
};

struct Times {
    uint64_t counter;
    uint64_t frames;
    float seconds;
    float frequency;
    uint16_t fps;
    float ms;
};

struct Engine {
    std::vector<Mod> mods;
    std::unordered_map<std::string, SDL_FRect> sprites;
    const bool *keyboard_state;
    std::array<ButtonState, 512> key_state;
    std::array<ButtonState, 8> mouse_button_state;
    Font default_font;
    glm::ivec2 mouse;
    glm::ivec2 screen;
    bool running;
    float dt;
    bool debug_mode;
    Color clear_color;
    Camera camera;
    Times times;
};

extern Engine engine; // NOLINT

bool isKeyPressed(Scancode key);

bool isKeyJustPressed(Scancode key);

bool isKeyJustReleased(Scancode key);

bool isMouseButtonJustPressed(MouseButton button);

float measureText(std::string_view text, float size, Font font);

inline float measureText(std::string_view text, float size = 10.0F) {
    return measureText(text, size, engine.default_font);
}

void drawText(FixedVector<UIInstance> &renderer, std::string_view text, glm::ivec2 position,
              float size, Color color, Font font);

inline void drawText(FixedVector<UIInstance> &renderer, std::string_view text, glm::ivec2 position,
                     float size = 10.0F, Color color = WHITE) {
    drawText(renderer, text, position, size, color, engine.default_font);
}

void drawWorldSymbol(FixedVector<WorldInstance> &instances, char character, glm::vec3 pos,
                     float size, Color color, Font font);

void updateEngineUI(FixedVector<UIInstance> &instances);
