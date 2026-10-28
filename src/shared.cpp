#pragma once

#include "skn.cpp"
#include "color.hpp"

#include <string_view>

#include "fixed_vector.hpp"
#include "math.hpp"

extern Arena arena;

enum class Key : uint8_t {
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
    key_1 = 30,
    key_2 = 31,
    key_3 = 32,
    key_4 = 33,
    key_5 = 34,
    key_6 = 35,
    key_7 = 36,
    key_8 = 37,
    key_9 = 38,
    key_0 = 39,
    enter = 40,
    escape = 41,
    backspace = 42,
    tab = 43,
    space = 44,
    minus = 45,
    equals = 46,
    leftbracket = 47,
    rightbracket = 48,
    backslash = 49,
    nonushash = 50,
    semicolon = 51,
    apostrophe = 52,
    grave = 53,
    comma = 54,
    period = 55,
    slash = 56,
    capslock = 57,
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
    printscreen = 70,
    scrolllock = 71,
    pause = 72,
    insert = 73,
    home = 74,
    pageup = 75,
    del = 76,
    end = 77,
    pagedown = 78,
    right = 79,
    left = 80,
    down = 81,
    up = 82,
    numlockclear = 83,
    kp_divide = 84,
    kp_multiply = 85,
    kp_minus = 86,
    kp_plus = 87,
    kp_enter = 88,
    kp_1 = 89,
    kp_2 = 90,
    kp_3 = 91,
    kp_4 = 92,
    kp_5 = 93,
    kp_6 = 94,
    kp_7 = 95,
    kp_8 = 96,
    kp_9 = 97,
    kp_0 = 98,
    kp_period = 99
};

enum MouseButton { left, middle, right };

enum class ButtonState : uint8_t { none, pressed, released };

struct UIInstance {
    FVector2 position;
    FVector2 size;
    FRect uv;
    Color color;
};

enum class Face : uint32_t { y_pos, x_pos, y_neg, x_neg, z_pos, z_neg, billboard, slope };

struct WorldInstance {
    FVector3 position;
    FVector2 size;
    FRect uv;
    Color color;
    Face face;
    float rotation;
};

struct WorldInstances {
    Fixed<WorldInstance> squares;
    Fixed<WorldInstance> triangles;
};

struct Mod {
    OldSlice<const char> name;
    Dynamic<OldSlice<const char>> items;
};

#include "font.hpp"

struct Camera {
    float pitch = 60;
    float yaw = 45;
    float roll = 0;
    FVector3 pos;
};

struct Engine {
    Dynamic<Mod> mods;
    HashMap<FRect> sprites;
    const bool *keyboard_state;
    Font default_font;
    ButtonState key_state[512];
    ButtonState mouse_button_state[8];
    Vector2 mouse;
    Vector2 screen;
    bool running;
    float dt;
    bool debug_mode;
    Color clear_color;
    Camera camera;

    bool isKeyPressed(Key key) const { return keyboard_state[int(key)]; }

    bool isKeyJustPressed(Key key) const { return key_state[int(key)] == ButtonState::pressed; }

    bool isKeyJustReleased(Key key) const { return key_state[int(key)] == ButtonState::released; }

    bool isMouseButtonJustPressed(MouseButton button) const {
        return mouse_button_state[int(button)] == ButtonState::pressed;
    }

    static float measureText(std::string_view text, float size, Font font) {
        if (text.size() == 0) return 0;
        float advance = 0;
        for (uint8_t c : text) advance += (float(font.widths[c]) + 1) * (size / font.size);
        return advance - (size / font.size);
    }

    float measureText(std::string_view text, float size = 10.0F) const {
        return measureText(text, size, default_font);
    }

    static void drawText(FixedVector<UIInstance> &renderer, std::string_view text,
                         Vector2 position, float size, Color color, Font font) {
        float advance = 0;
        for (uint8_t c : text) {
            const uint16_t i = c - ' ';
            FVector2 texture_offset = {float(i % 16) * font.size, int(i / 16.F) * font.size};
            append(renderer, FVector2(int(position.x + advance), position.y),
                   FVector2(size, size),
                   toFRect(FVector2{font.texture.x, font.texture.y} + texture_offset,
                           {font.size, font.size}),
                   color);
            advance += (float(font.widths[c]) + 1) * (size / font.size);
        }
    }

    void drawText(FixedVector<UIInstance> &renderer, std::string_view text, Vector2 position,
                  float size = 10.0F, Color color = WHITE) const {
        drawText(renderer, text, position, size, color, default_font);
    }

    void drawTextF(Arena *a, FixedVector<UIInstance> &instances, Vector2 pos, const char *fmt,
                   ...) __attribute__((format(gnu_printf, 5, 6))) {
        va_list ap;
        va_start(ap, fmt);
        auto slice = a->vAllocPrintZ(fmt, ap);
        va_end(ap);
        drawText(instances, slice.ptr, pos);
        a->free(slice);
    }

    void drawWorldSymbol(Fixed<WorldInstance> *instances, FVector3 pos, char c, float size,
                         Color color, Font font) {
        const uint16_t i = c - ' ';
        FVector2 texture_offset = {float(i % 16) * font.size, int(i / 16.F) * font.size};
        float width = font.widths[int(c)];
        instances->append({
            .position = pos,
            .size = {width * (size / font.size) / 10.F, size / 10.F},
            .uv = toFRect(FVector2{font.texture.x, font.texture.y} + texture_offset,
                          {width, font.size}),
            .color = color,
            .face = Face::billboard,
            .rotation = 0,
        });
    }

    __attribute__((format(printf, 1, 2))) static void log(const char *fmt, ...);

    int rand(int n) const;

    void updateUI(FixedVector<UIInstance> &instances, Vector2 size);

    void update(WorldInstances *instances, FVector2 logical);
};

extern Engine engine;

enum class Scene { menu, game };

struct Game;

struct SceneGame {

    
    void init(Engine *engine);
    void update(Engine *engine, WorldInstances *instances, FVector2 logical);
    void updateUI(Game *game, Engine *engine, FixedVector<UIInstance> &instances,
                  Vector2 render_size);
    void deinit(Engine *engine);
};
