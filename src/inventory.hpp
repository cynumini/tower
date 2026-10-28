#pragma once

#include "shared.cpp"

struct Slot {
    std::string_view name;
    uint8_t count;
};

static constexpr Vector2 INVENTORY_SIZE = {.x = 8, .y = 8};

struct Inventory {
    std::unordered_map<std::string_view, FRect> items;
    std::array<Slot, size_t(INVENTORY_SIZE.x *INVENTORY_SIZE.y)> slots{};
    bool visible = false;
    FRect slot_sprite;
    FRect select_sprite;
    size_t select = 0;
};

Inventory createInventory(Engine &engine);
void updateInventoryUI(Inventory &self, Engine *engine, FixedVector<UIInstance> &instances,
                       Vector2 render_size);
void addToInventory(Inventory &self, std::string_view name, uint8_t amount);
std::string_view getCurrentInventoryItem(const Inventory &self);
