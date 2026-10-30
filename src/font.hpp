#pragma once

struct Font {
    std::array<char, 256> widths = {};
    float size{};
    SDL_FRect texture{};
};

Font createDefaultFont(SDL_FRect texture);
