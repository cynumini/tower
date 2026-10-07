#pragma once

#include "shared.cpp"

#include <skn.cpp>
#include <skn_math.cpp>

// Bloks
enum class Block : u8 { air, dirt, grass, water, wood_floor, wall, deep, stone };

Rect blocks[U8_MAX] = {};

// Locations
static struct Location {
    const char *name;
    Rect rect;
    Block block;
} locations[2];

struct Cell {
    Block block;
    Block top;
};

struct Map {
    static constexpr int MAP_SIZE_X = 256;
    static constexpr int MAP_SIZE_Y = 256;
    static constexpr int MAP_SIZE_Z = 256;
    static constexpr int MAP_MIN_X = -MAP_SIZE_X / 2;
    static constexpr int MAP_MIN_Y = -MAP_SIZE_Y / 2;
    static constexpr int MAP_MIN_Z = 0;
    static constexpr int MAP_MAX_X = MAP_SIZE_X / 2 - 1;
    static constexpr int MAP_MAX_Y = MAP_SIZE_Y / 2 - 1;
    static constexpr int MAP_MAX_Z = 255;
    Slice<Cell> data = {};
    uint current_level = 0;
    bool hide_above_level = false;

    inline Cell get(int x, int y, int z) {
        x += 128, y += 128, z += 128;
        return data[x * MAP_SIZE_Y * MAP_SIZE_Z + y * MAP_SIZE_Z + z];
    }

    inline void set(int x, int y, int z, Cell cell) {
        x += 128, y += 128, z += 128;
        data[x * MAP_SIZE_Y * MAP_SIZE_Z + y * MAP_SIZE_Z + z] = cell;
    }

    inline bool checkFaceVisible(int x, int y, int z) {
        if (x < MAP_MIN_X or y < MAP_MIN_Y or z < MAP_MIN_Z) return true;
        if (x > MAP_MAX_X or y > MAP_MAX_Y or z > MAP_MAX_Z) return true;
        return get(x, y, z).block == Block::air;
    }

    void init(Engine *engine) {
        data = arena.alloc<Cell>(MAP_SIZE_X * MAP_SIZE_Y * MAP_SIZE_Z);

        // location
        locations[0] = {"Home", {0, 0, 32, 32}, Block::dirt};
        locations[1] = {"Town", {32, 0, 32, 32}, Block::grass};

        // bloks
        blocks[int(Block::air)] = {};
        blocks[int(Block::dirt)] = engine->sprites.get("dirt");
        blocks[int(Block::grass)] = engine->sprites.get("grass");
        blocks[int(Block::water)] = engine->sprites.get("water");
        blocks[int(Block::wood_floor)] = engine->sprites.get("wood_floor");
        blocks[int(Block::wall)] = engine->sprites.get("wall");
        blocks[int(Block::deep)] = engine->sprites.get("deep");
        blocks[int(Block::stone)] = engine->sprites.get("stone");

        // map
        for (int x = MAP_MIN_X; x <= MAP_MAX_X; x++) {
            for (int y = MAP_MIN_Y; y <= MAP_MAX_Y; y++) {
                for (int z = MAP_MIN_Z; z <= MAP_MAX_Z; z++) {
                    if (z == 0)
                        set(x, y, z, {Block::deep, Block::deep});
                    else if (z < 60)
                        set(x, y, z, {Block::stone, Block::stone});
                    else if (z < 63)
                        set(x, y, z, {Block::dirt, Block::dirt});
                    else if (z < 64)
                        set(x, y, z, {Block::dirt, Block::grass});
                }
            }
        }
        for (u8 i = 0; i < u8(ARRAY_LEN(locations)); i++) {
            const auto *location = &locations[i];

            auto x_start = location->rect.x - location->rect.w / 2;
            auto x_end = x_start + location->rect.w;

            auto y_start = location->rect.y - location->rect.h / 2;
            auto y_end = y_start + location->rect.h;

            for (int x = x_start; x < x_end; x++) {
                for (int y = y_start; y < y_end; y++) {
                    set(x, y, 63, {location->block, location->block});
                }
            }
        }

        // house
        int x_offset = -4 + 32;
        int y_offset = -4;
        for (int x = x_offset; x < (9 + x_offset); x++) {
            for (int y = y_offset; y < (9 + y_offset); y++) {
                if ((x > x_offset and y > y_offset) and (x < x_offset + 8 and y < y_offset + 8)) {
                    set(x, y, 64, {Block::air, Block::wood_floor});
                    set(x, y, 65, {Block::air, Block::wood_floor});
                } else {
                    set(x, y, 64, {Block::wall, Block::wall});
                    set(x, y, 65, {Block::wood_floor, Block::wood_floor});
                }
                set(x, y, 63, {Block::dirt, Block::wood_floor});
            }
        }
    }

    void update(Engine *engine, Fixed<WorldInstance> *instances, Vec3 player_pos) {
        current_level = int(player_pos.z);

        const int distance = 20;

        int x_start = std::max(int(player_pos.x) - distance, MAP_MIN_X);
        int x_end = std::min(int(player_pos.x) + distance, MAP_MAX_X);

        int y_start = std::max(int(player_pos.y) - distance, MAP_MIN_Y);
        int y_end = std::min(int(player_pos.y) + distance, MAP_MAX_Y);

        int z_start = std::max(int(player_pos.z) - distance, MAP_MIN_Z);
        int z_end = std::min(int(player_pos.z) + distance, MAP_MAX_Z);
        if (hide_above_level) z_end = std::min(int(player_pos.z + 1), z_end);

        for (int x = x_start; x < x_end; x++) {
            for (int y = y_start; y < y_end; y++) {
                for (int z = z_start; z < z_end; z++) {
                    const Cell cell = get(x, y, z);
                    if (cell.block == Block::air and cell.top == Block::air) continue;

                    const Rect block = blocks[int(cell.block)];
                    const Rect top = blocks[int(cell.top)];

                    const bool draw_top = hide_above_level and int(player_pos.z) == z
                                              ? cell.block != Block::air
                                              : checkFaceVisible(x, y, z + 1);

                    if (draw_top) {
                        instances->append(
                            {Vec3(x, y, z + 0.5F), {1, 1}, top, WHITE, Face::z_pos, 0});
                    }

                    if (checkFaceVisible(x + 1, y, z))
                        instances->append(
                            {Vec3(x + 0.5F, y, z), {1, 1}, block, WHITE, Face::x_pos, 0});

                    if (checkFaceVisible(x - 1, y, z))
                        instances->append(
                            {Vec3(x - 0.5F, y, z), {1, 1}, block, 0xBFBFBFFF, Face::x_neg, 0});

                    if (checkFaceVisible(x, y + 1, z))
                        instances->append(
                            {Vec3(x, y + 0.5F, z), {1, 1}, block, 0xDFDFDFFF, Face::y_pos, 0});

                    if (checkFaceVisible(x, y - 1, z))
                        instances->append(
                            {Vec3(x, y - 0.5F, z), {1, 1}, block, 0xDFDFDFFF, Face::y_neg, 0});
                }
            }
        }
    }
};
