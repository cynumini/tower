#pragma once

#include "ui_pipeline.hpp"

struct Slot {
    std::string_view name;
    uint8_t count;
};

static constexpr glm::ivec2 INVENTORY_SIZE = {8, 8};

struct Inventory {
    std::unordered_map<std::string_view, SDL_FRect> items;
    std::array<Slot, size_t(INVENTORY_SIZE.x *INVENTORY_SIZE.y)> slots{};
    bool visible = false;
    SDL_FRect slot_sprite;
    SDL_FRect select_sprite;
    size_t select = 0;
};

Inventory createInventory();
void updateInventoryUI(Inventory &self, FixedVector<UIInstance> &instances, glm::ivec2 render_size);
void addToInventory(Inventory &self, std::string_view name, uint8_t amount);
std::string_view getCurrentInventoryItem(const Inventory &self);
