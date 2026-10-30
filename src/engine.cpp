#include "engine.hpp"
#include "rect.hpp"

bool isKeyPressed(Scancode key) { return engine.keyboard_state[int(key)]; }

bool isKeyJustPressed(Scancode key) { return engine.key_state[int(key)] == ButtonState::pressed; }

bool isKeyJustReleased(Scancode key) {
    return engine.key_state[int(key)] == ButtonState::released;
}

bool isMouseButtonJustPressed(MouseButton button) {
    return engine.mouse_button_state[int(button)] == ButtonState::pressed;
}

float measureText(std::string_view text, float size, Font font) {
    if (text.empty()) return 0;
    float x = 0;
    auto char_size = size / font.size;
    for (uint8_t c : text) x += float((font.widths[c]) + 1) * char_size;
    return x - char_size;
}

void drawText(FixedVector<UIInstance> &renderer, std::string_view text, glm::ivec2 pos, float size,
              Color color, Font font) {
    glm::vec2 char_pos = pos;
    for (uint8_t c : text) {
        const int i = c - ' ';
        glm::vec2 texture_offset = glm::vec2(i % 16, i / 16) * font.size;
        append(renderer, char_pos, glm::vec2(size, size),
               toFRect(fRectPos(font.texture) + texture_offset, {font.size, font.size}),
               color);
        char_pos.x += (float(font.widths[c]) + 1) * (size / font.size);
    }
}

void drawWorldSymbol(FixedVector<WorldInstance> &instances, char character, glm::vec3 pos,
                     float size, Color color, Font font) {
    const int i = character - ' ';
    glm::vec2 texture_offset = glm::vec2{i % 16, i / 16} * font.size;
    glm::vec2 char_size = {float(font.widths[uint8_t(character)]), font.size};
    append(instances, pos, (char_size * size) / (font.size * font.size),
           toFRect(fRectPos(font.texture) + texture_offset, char_size), color, Face::billboard,
           0);
}

// updateEngineUI
struct DebugDraw {
    FixedVector<UIInstance> &instances;
    glm::ivec2 offset;
};

static void debugDrawLine(DebugDraw &dd, std::string_view text) {
    drawText(dd.instances, text, dd.offset);
    dd.offset.y += 12;
}

void updateEngineUI(FixedVector<UIInstance> &instances) {
    if (isKeyJustPressed(Scancode::escape)) engine.running = true;
    if (isKeyJustPressed(Scancode::f3)) engine.debug_mode = !engine.debug_mode;
    if (engine.debug_mode) {
        DebugDraw dd = {.instances = instances, .offset = {2,2}};
        debugDrawLine(dd, std::format("FPS: {}", engine.times.fps));
        debugDrawLine(dd, std::format("{:.2f}ms", engine.times.ms));
        debugDrawLine(dd, std::format("Screen: {}x{}", engine.screen.x, engine.screen.y));
        debugDrawLine(dd, std::format("Mouse: {}x{}", engine.mouse.x, engine.mouse.y));
        debugDrawLine(dd, std::format("Pitch: {:.0f}", engine.camera.pitch));
        debugDrawLine(dd, std::format("Yaw: {:.0f}", engine.camera.yaw));
        debugDrawLine(dd, std::format("Roll: {:.0f}", engine.camera.roll));
        debugDrawLine(dd,
                      std::format("Camera: x = {:.2f}, y = {:.2f}, z = {:.2f}",
                                  engine.camera.pos.x, engine.camera.pos.y, engine.camera.pos.z));
        debugDrawLine(
            dd, std::format("UI instances: {}/{}", ui.prev_instances_len, UI::MAX_INSTANCES));
        debugDrawLine(dd, std::format("World square instances: {}/{}", world.prev_squares_len,
                                      World::MAX_INSTANCES / 2));
        debugDrawLine(dd, std::format("World triangle instances: {}/{}", world.prev_triangles_len,
                                      World::MAX_INSTANCES / 2));
    }
}
