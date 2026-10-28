#pragma once

using Color = SDL_Color;

constexpr Color toColor(uint32_t value) {
    return {.r = uint8_t(value >> 24U),
            .g = uint8_t(value >> 16U),
            .b = uint8_t(value >> 8U),
            .a = uint8_t(value)};
}

constexpr Color WHITE = toColor(0xFFFFFFFF);
constexpr Color GRAY = toColor(0x808080FF);
constexpr Color BLACK = toColor(0x000000FF);

constexpr Color RED = toColor(0xFF0000FF);
constexpr Color ORANGE = toColor(0xFFA500FF);
constexpr Color YELLOW = toColor(0xFFFF00FF);
constexpr Color LIME = toColor(0xBFFF00FF);
constexpr Color GREEN = toColor(0x00FF00FF);
constexpr Color TEAL = toColor(0x008080FF);
constexpr Color CYAN = toColor(0x00FFFFFF);
constexpr Color BLUE = toColor(0x0000FFFF);
constexpr Color NAVY = toColor(0x000080FF);
constexpr Color PURPLE = toColor(0x800080FF);
constexpr Color MAGENTA = toColor(0xFF00FFFF);
constexpr Color PINK = toColor(0xFFC0CBFF);
constexpr Color BROWN = toColor(0xA52A2AFF);
