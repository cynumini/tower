#pragma once

#include "base.hpp"
#include "renderer.hpp"

struct Font {
    Array<u8, 128> data{};
    Texture::Id texture_id = 0;

    Font() {}
    Font(Allocator &gpa, Renderer &renderer, const String &font_filename, const String &texture_filename, u8 font_size);

    void drawText(Renderer &renderer, glm::vec2 position, f32 size, const String &string);
};
