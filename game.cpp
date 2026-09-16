#include "game.hpp"

#include "unagi.hpp"

static ConstString createString(char *cstr) { return {cstr, lenZ(cstr)}; }

static void drawText(Fixed<Instance> *renderer, StringZ text, Font font, vec2 position) {
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

void gameInit(GameState *state, Engine *engine, HashMap<Rect> sprites) {
    state->font.init(sprites.get("font"));
    state->player_size = {16.0F, 32.0F};
}

GameResult gameUpdate(GameState *state, Engine *engine, AllocatorOld *a,
                      Fixed<Instance> instances, HashMap<Rect> sprites) {
    bool quit = false;
    if (engine->key_down[int(Scancode::escape)]) {
        quit = true;
    }
    if (engine->key_down[int(Scancode::space)]) {
        state->attack = true;
    }
    if (engine->key_down[int(Scancode::e)]) {
        state->invertory_visible = !state->invertory_visible;
    }
    if (engine->key_down[int(Scancode::f)]) {
        state->cast_spell = true;
    }

    vec2 velocity{float(engine->keyboard_state[int(Scancode::d)]) -
                      float(engine->keyboard_state[int(Scancode::a)]),
                  float(engine->keyboard_state[int(Scancode::s)]) -
                      float(engine->keyboard_state[int(Scancode::w)])};

    velocity = velocity.normalize();

    size_t ui_instance_offset = instances.len;
    // Rect character = sprites.get("spell2");
    // instances.append({{0, 0}, character.size(), character / 4096, WHITE, 0, 1});
    auto buffer = a->allocFormatZ("%.02f FPS", engine->fps);
    drawText(&instances, buffer, state->font, {0, 0});

    return {instances.len, ui_instance_offset,
            -state->player_position + engine->screen / 2.0F - state->player_size / 2.0F, quit};

}
