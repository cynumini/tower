#pragma once

#include <skn.cpp>
#include <skn/color.cpp>
#include <skn/rect.cpp>
#include <skn/vec.cpp>

#include <string_view>

#include "fixed_vector.hpp"

static Arena arena;

enum class Key : u8 {
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

enum class ButtonState : u8 { none, pressed, released };

struct UIInstance {
    Vec2 position;
    Vec2 size;
    skn::Rect uv;
    skn::Color color;
};

enum class Face : u32 { y_pos, x_pos, y_neg, x_neg, z_pos, z_neg, billboard, slope };

struct WorldInstance {
    Vec3 position;
    Vec2 size;
    skn::Rect uv;
    skn::Color color;
    Face face;
    f32 rotation;
};

struct WorldInstances {
    Fixed<WorldInstance> squares;
    Fixed<WorldInstance> triangles;
};

struct Mod {
    OldSlice<const char> name;
    Dynamic<OldSlice<const char>> items;
};

#include "font.cpp"

struct Camera {
    f32 pitch = 60;
    f32 yaw = 45;
    f32 roll = 0;
    Vec3 pos;
};

struct Engine {
    Dynamic<Mod> mods;
    HashMap<skn::Rect> sprites;
    const bool *keyboard_state;
    Font default_font;
    ButtonState key_state[512];
    ButtonState mouse_button_state[8];
    Vec2i mouse;
    Vec2i screen;
    bool running;
    f32 dt;
    bool debug_mode;
    skn::Color clear_color;
    Camera camera;

    bool isKeyPressed(Key key) const { return keyboard_state[int(key)]; }

    bool isKeyJustPressed(Key key) const { return key_state[int(key)] == ButtonState::pressed; }

    bool isKeyJustReleased(Key key) const { return key_state[int(key)] == ButtonState::released; }

    bool isMouseButtonJustPressed(MouseButton button) const {
        return mouse_button_state[int(button)] == ButtonState::pressed;
    }

    static f32 measureText(std::string_view text, f32 size, Font font) {
        if (text.size() == 0) return 0;
        f32 advance = 0;
        for (u8 c : text) advance += (f32(font.widths[c]) + 1) * (size / font.size);
        return advance - (size / font.size);
    }

    f32 measureText(std::string_view text, f32 size = 10.0F) const {
        return measureText(text, size, default_font);
    }

    static void drawText(FixedVector<UIInstance> &renderer, std::string_view text, Vec2i position,
                         f32 size, skn::Color color, Font font) {
        f32 advance = 0;
        for (u8 c : text) {
            const u16 i = c - ' ';
            Vec2 texture_offset = {f32(i % 16) * font.size, int(i / 16.F) * font.size};
            append(renderer, Vec2(int(position.x + advance), position.y), Vec2(size, size),
                   skn::Rect{font.texture.position() + texture_offset, {font.size, font.size}},
                   color);
            advance += (f32(font.widths[c]) + 1) * (size / font.size);
        }
    }

    void drawText(FixedVector<UIInstance> &renderer, std::string_view text, Vec2i position,
                  f32 size = 10.0F, skn::Color color = skn::WHITE) const {
        drawText(renderer, text, position, size, color, default_font);
    }

    void drawTextF(Arena *a, FixedVector<UIInstance> &instances, Vec2 pos, const char *fmt, ...)
        __attribute__((format(gnu_printf, 5, 6))) {
        va_list ap;
        va_start(ap, fmt);
        auto slice = a->vAllocPrintZ(fmt, ap);
        va_end(ap);
        drawText(instances, slice.ptr, pos);
        a->free(slice);
    }

    void drawWorldSymbol(Fixed<WorldInstance> *instances, Vec3 pos, char c, f32 size,
                         skn::Color color, Font font) {
        const u16 i = c - ' ';
        Vec2 texture_offset = {f32(i % 16) * font.size, int(i / 16.F) * font.size};
        f32 width = font.widths[int(c)];
        instances->append({
            .position = pos,
            .size = {width * (size / font.size) / 10.F, size / 10.F},
            .uv = skn::Rect{font.texture.position() + texture_offset, {width, font.size}},
            .color = color,
            .face = Face::billboard,
            .rotation = 0,
        });
    }

    __attribute__((format(printf, 1, 2))) static void log(const char *fmt, ...);

    int rand(int n) const;

    void updateUI(FixedVector<UIInstance> &instances, Vec2i size);

    void update(WorldInstances *instances, Vec2 logical);
};

extern Engine engine;

enum class Scene { menu, game };

struct Game {
    Scene current_scene;

    void init(Engine *engine);

    void update(Engine *engine, WorldInstances *instances, Vec2 logical);

    void updateUI(Engine *engine, FixedVector<UIInstance> &instances, Vec2i render_size);

    void deinit(Engine *engine);
};
