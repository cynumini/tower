#include "game.hpp"

#include "unagi.hpp"

static void drawText(Fixed<Instance> *renderer, SliceZ<const char> text, Font font,
                     vec2 position) {
    float advance = 0;
    for (size_t i = 0; i < text.len; i++) {
        vec2 texture_offset = {0, 0};
        const u8 c = text.ptr[i] - ' ';
        texture_offset = {
            .x = float(c % 16) * 10.0F,
            .y = float(c / 16) * 10.0F, // NOLINT
        };
        renderer->append({{position.x + advance, position.y},
                          {10.0F, 10.0F},
                          Rect{font.texture.x + texture_offset.x,
                               font.texture.y + texture_offset.y, 10.0F, 10.0F} /
                              4096.0F,
                          BLACK,
                          0.0F});
        advance += float(font.widths[u8(text.ptr[i])]) + 1;
    }
}

void Game::init(Engine *engine, Arena *arena) {
    this->arena = arena;
    font.init(engine->sprites.get("font"));
    player_size = {16.0F, 32.0F};
    engine->clear_color = colorFromHex(0x8bbbffff);
}

vec2 Game::update(Engine *engine, Fixed<Instance> *instances) {
    if (engine->is_key_just_pressed(Key::escape)) engine->running = true;
    if (engine->is_key_just_pressed(Key::space)) attack = true;
    if (engine->is_key_just_pressed(Key::e)) invertory_visible = !invertory_visible;
    if (engine->is_key_just_pressed(Key::f)) cast_spell = true;

    vec2 velocity{float(engine->is_key_pressed(Key::d)) - float(engine->is_key_pressed(Key::a)),
                  float(engine->is_key_pressed(Key::s)) - float(engine->is_key_pressed(Key::w))};

    velocity = velocity.normalize();

    Rect character = engine->sprites.get("zombie");
    instances->append({{0, 0}, character.size(), character / 4096, WHITE, 0});

    return -player_position + engine->screen / 2.0F - player_size / 2.0F;
}

void Game::updateUI(Engine *engine, Fixed<Instance> *instances) const {
    ScopeArena scope(arena);
    auto buffer1 = scope.tmp.allocPrint("FPS: %d", engine->fps);
    auto buffer2 = scope.tmp.allocPrint("%.2fms", engine->ms);
    drawText(instances, {buffer1.len, buffer1.ptr}, font, {2, 2});
    drawText(instances, {buffer2.len, buffer2.ptr}, font, {2, 14});
}
