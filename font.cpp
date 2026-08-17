#include "font.hpp"

Font::Font(Renderer &renderer, const String &font_filename, const String &texture_filename, u8 font_size) {
    auto font_data = OS::readEntireFile(font_filename);

    for (auto &element : data) {
        element = font_size;
    }

    for (usize i = 0; i < font_data.len; i++) {
        char character = (char)font_data[i];
        assert(character >= ' ' and character <= '~');

        i += 1;
        assert(font_data[i] == ',');

        i += 1;
        u8 width = 0;
        while (i < font_data.len and font_data[i] != '\n') {
            assert(font_data[i] >= '0' and font_data[i] <= '9');
            width = u8(width * 10 + (font_data[i] - '0'));
            i += 1;
        }
        assert(width != 0);

        data[usize(character)] = width;
    }

    font_data.deinit();
    texture_id = renderer.loadTexture(texture_filename);
}

void Font::drawText(Renderer &renderer, Vector2 position, f32 size, const String &string) {
    constexpr f32 font_height = 10;
    f32 scale = size / font_height;
    f32 space = 1 * scale;
    f32 height = size;
    f32 offset = position.x;
    auto widths = data;
    for (auto c : string) {
        f32 font_width = f32(widths[usize(c)]);
        f32 width = font_width * scale;
        int x = c % 16;
        int y = (c - 0x20) / 16;
        Vector2 char_position{.x = f32(x) * font_height, .y = f32(y) * font_height};
        renderer.drawTexture(texture_id, {char_position.x, char_position.y, font_width, font_height}, {offset, position.y, width, height},
                             renderer.base_model_view);
        offset += width + space;
    }
}
