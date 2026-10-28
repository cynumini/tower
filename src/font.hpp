#pragma once

#include "math.hpp"

struct Font {
    std::array<char, 256> widths = {};
    float size{};
    FRect texture{};
};

Font createDefaultFont(FRect texture);
