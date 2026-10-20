#pragma once

#include "shared.cpp"

HashMap<Rect> items;

static struct Inventory {
    struct Slot {
        const char *name;
        u8 count;
    };

    static constexpr Vec2i SIZE = {8, 8};
    Array<Slot, SIZE.x * SIZE.y> slots;
    bool visible;
    Rect slot_sprite;
    Rect select_sprite;
    size_t select = 0;

    void init(Engine *engine) {
        // items
        items.init(&arena, 1);
        items.put(&arena, "gold", engine->sprites.get("gold"));
        items.put(&arena, "iron_ore", engine->sprites.get("iron_ore"));
        items.put(&arena, "sword", engine->sprites.get("sword"));
        items.put(&arena, "hoe", engine->sprites.get("hoe"));
        items.put(&arena, "pickaxe", engine->sprites.get("pickaxe"));
        items.put(&arena, "iron_ingot", engine->sprites.get("iron_ingot"));

        // inventory
        slot_sprite = engine->sprites.get("inventory_slot");
        select_sprite = engine->sprites.get("inventory_select");
        add("sword", 1);
        add("hoe", 1);
        add("pickaxe", 1);
        add("gold", 10);
    }

    void drawSlot(Engine *engine, Fixed<UIInstance> *instances, Vec2 position, size_t index) {
        assert(index < slots.len);
        Slot *slot = &slots[index];
        instances->append({position, slot_sprite.size(), slot_sprite, WHITE});
        if (slot->count > 0) {
            instances->append({position, slot_sprite.size(), items.get(slot->name), WHITE});
            char buffer[3] = {};
            SliceZ<char> count = {3, buffer};
            sprintf(count.ptr, "%d", slot->count);
            constexpr float FONT_SIZE = 10.F;
            auto width = engine->measureText(count.withoutZero(), FONT_SIZE);
            engine->drawText(instances, count.withoutZero(),
                             position + slot_sprite.size() - Vec2{width, FONT_SIZE}, FONT_SIZE);
        }
    }

    void updateUI([[maybe_unused]] Game *game, Engine *engine, Fixed<UIInstance> *instances,
                  Vec2i render_size, [[maybe_unused]] Arena *a) {

        // hud
        constexpr int hud_len = 3;
        for (int i = 0; i < hud_len; i++) {
            if (engine->isKeyPressed(Key(int(Key::key_1) + i))) select = i;
        }

        Vec2 hud_offset = {(render_size.x - slot_sprite.w * hud_len) / 2,
                           render_size.y - slot_sprite.h};
        for (size_t i = 0; i < hud_len; i++) {
            drawSlot(engine, instances, hud_offset, i);
            if (i == select) {
                instances->append({hud_offset, select_sprite.size(), select_sprite, WHITE});
            }
            hud_offset.x += slot_sprite.w;
        }

        // inventory
        if (engine->isKeyJustPressed(Key::e)) visible = !visible;
        const Vec2 slot_size = slot_sprite.size();
        if (visible) {
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    const Vec2 position =
                        Vec2(x, y) * slot_size + (render_size - (slot_size * SIZE.x));
                    drawSlot(engine, instances, position, y * SIZE.y + x);
                }
            }
        }
    }

    void add(const char *name, int amount) {
        assert(items.contains(name));
        for (auto &slot : slots) {
            if (slot.name == 0) continue;
            if (strcmp(slot.name, name) == 0) {
                slot.count += amount;
                assert(slot.count < 100);
                return;
            }
        }
        for (auto &slot : slots) {
            if (slot.count == 0) {
                slot.name = name;
                slot.count = amount;
                assert(slot.count < 100);
                return;
            }
        }
        TODO("What if there is no such item in the inventory nor empty inventory slots?");
    }

    const char *getCurrent() {
        assert(slots.len > select);
        return slots[select].name;
    }
} inventory;
