#include "inventory.hpp"

Inventory createInventory(Engine &engine) {
    Inventory inventory{
        .items =
            {
                {"gold", engine.sprites.at("gold")},
                {"iron_ore", engine.sprites.at("iron_ore")},
                {"sword", engine.sprites.at("sword")},
                {"hoe", engine.sprites.at("hoe")},
                {"pickaxe", engine.sprites.at("pickaxe")},
                {"iron_ingot", engine.sprites.at("iron_ingot")},
            },
        .slot_sprite = engine.sprites.at("inventory_slot"),
        .select_sprite = engine.sprites.at("inventory_select"),
    };
    addToInventory(inventory, "sword", 1);
    addToInventory(inventory, "hoe", 1);
    addToInventory(inventory, "pickaxe", 1);
    addToInventory(inventory, "gold", 10);
    return inventory;
}

static void drawSlot(Inventory &self, FixedVector<UIInstance> &instances,
                     FVector2 position, size_t index) {
    SDL_assert(index < self.slots.size());
    Slot &slot = self.slots[index];
    append(instances, position, fRectSize(self.slot_sprite), self.slot_sprite, WHITE);
    if (slot.count > 0) {
        append(instances, position, fRectSize(self.slot_sprite), self.items.at(slot.name), WHITE);
        auto buffer = std::to_string(slot.count);
        constexpr float FONT_SIZE = 10.F;
        auto padding = FVector2(measureText(buffer, FONT_SIZE), FONT_SIZE) + 2.F;
        drawText(instances, buffer, toVector2(position + fRectSize(self.slot_sprite) - padding),
                 FONT_SIZE);
    }
}

void updateInventoryUI(Inventory &self, Engine *engine, FixedVector<UIInstance> &instances,
                       Vector2 render_size) {
    const FVector2 slot_size = fRectSize(self.slot_sprite);

    // hud
    constexpr int hud_len = 3;
    FVector2 hud_offset = {.x = (float(render_size.x) - self.slot_sprite.w * hud_len) / 2,
                           .y = float(render_size.y) - self.slot_sprite.h};
    constexpr auto sc_1 = size_t(Scancode::sc_1);
    for (size_t i = 0; i < hud_len; i++) {
        if (isKeyPressed(Scancode(sc_1 + i))) self.select = i;
        drawSlot(self, instances, hud_offset, i);
        if (i == self.select) {
            append(instances, hud_offset, slot_size, self.select_sprite, WHITE);
        }
        hud_offset.x += self.slot_sprite.w;
    }

    // inventory
    if (isKeyJustPressed(Scancode::e)) self.visible = !self.visible;
    if (self.visible) {
        const auto inventory_size = slot_size * INVENTORY_SIZE;
        const auto inventory_offset = render_size - inventory_size;
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                const FVector2 position = Vector2{.x = x, .y = y} * slot_size + inventory_offset;
                auto slot_index = y * INVENTORY_SIZE.y + x;
                drawSlot(self, instances, position, slot_index);
            }
        }
    }
}

void addToInventory(Inventory &self, std::string_view name, uint8_t amount) {
    SDL_assert(self.items.contains(name));
    for (auto &slot : self.slots) {
        if (slot.name.empty()) continue;
        if (slot.name == name) {
            slot.count += amount;
            SDL_assert(slot.count < 100);
            return;
        }
    }
    for (auto &slot : self.slots) {
        if (slot.count == 0) {
            slot.name = name;
            slot.count = amount;
            SDL_assert(slot.count < 100);
            return;
        }
    }
    std::cerr << "What if there is no such item in the inventory nor empty inventory slots?";
    std::abort();
}

std::string_view getCurrentInventoryItem(const Inventory &self) {
    SDL_assert(self.slots.size() > self.select);
    return self.slots[self.select].name;
}
