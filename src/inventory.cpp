#pragma once

#include "shared.cpp"

static struct Inventory {
    struct Slot {
        u8 item_id;
        u8 count;
    };

    Slot inventory[8 * 8];
    bool visible;

    void init(Dynamic<Slice<const char>> items) {
        for (u8 i = 0; i < u8(items.len); i++) {
            inventory[i] = Slot{i, 1};
        }
    }

    void updateUI(Game *game, Engine *engine, Fixed<UIInstance> *instances, int render_width,
                  int render_height, Arena *a, Dynamic<Slice<const char>> items) {
        if (visible) {
            for (size_t x_i = 0; x_i < 8; x_i++) {
                for (size_t y_i = 0; y_i < 8; y_i++) {
                    const Vec2 cell_size = engine->sprites.get("inventory_slot").size();
                    const Vec2 position =
                        Vec2{float(x_i), float(y_i)} * cell_size +
                        (Vec2{float(render_width), float(render_height)} - (cell_size * 8.0F));
                    instances->append(
                        {position, cell_size, engine->sprites.get("inventory_slot"), WHITE});
                    const auto *invertory_slot = &inventory[(y_i * 8) + x_i];
                    if (invertory_slot->count != 0) {
                        assert(invertory_slot->count);
                        assert(invertory_slot->count < 100);
                        auto text = a->allocPrint("%d", invertory_slot->count);
                        instances->append({position, cell_size,
                                           engine->sprites.get(items[invertory_slot->item_id]),
                                           WHITE});
                        const float FONT_SIZE = 10;
                        const Vec2 text_offset =
                            position +
                            (cell_size -
                             Vec2(engine->measureText({text.len, text.ptr}), FONT_SIZE));
                        engine->drawText(instances, text, text_offset);
                    }
                }
            }
        }
    }
} inventory;
