#pragma once

#include "shared.cpp"

#include <skn.cpp>
#include <skn_math.cpp>

// Bloks
enum class Block : u8 { air, dirt, grass, water, wood_floor, wall, deep };

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
    static constexpr int MAP_MAX_X = 256;
    static constexpr int MAP_MAX_Y = 256;
    static constexpr int MAP_MAX_Z = 256;
    Slice<Cell> data = {};
    uint current_level = 0;

    inline Cell get(u32 x, u32 y, u32 z) {
        return data[x * MAP_MAX_Y * MAP_MAX_Z + y * MAP_MAX_Z + z];
    }

    inline void set(u32 x, u32 y, u32 z, Cell cell) {
        data[x * MAP_MAX_Y * MAP_MAX_Z + y * MAP_MAX_Z + z] = cell;
    }

    inline bool checkFaceVisible(int x, int y, int z) {
        if (x < 0 or y < 0 or z < 0) return true;
        if (x >= MAP_MAX_X or y >= MAP_MAX_Y or z >= MAP_MAX_Z) return true;
        return get(x, y, z).block == Block::air;
    }

    void init(Engine *engine) {
        data = arena.alloc<Cell>(MAP_MAX_X * MAP_MAX_Y * MAP_MAX_Z);

        // location
        locations[0] = {"Home", {32, 32, 64, 64}, Block::dirt};
        locations[1] = {"Town", {96, 32, 64, 64}, Block::grass};

        // bloks
        blocks[int(Block::air)] = {};
        blocks[int(Block::dirt)] = engine->sprites.get("dirt");
        blocks[int(Block::grass)] = engine->sprites.get("grass");
        blocks[int(Block::water)] = engine->sprites.get("water");
        blocks[int(Block::wood_floor)] = engine->sprites.get("wood_floor");
        blocks[int(Block::wall)] = engine->sprites.get("wall");
        blocks[int(Block::deep)] = engine->sprites.get("deep");

        // map
        for (uint x = 0; x < MAP_MAX_X; x++) {
            for (uint y = 0; y < MAP_MAX_X; y++) {
                set(x, y, 0, {Block::deep, Block::deep});
            }
        }
        for (uint x = 0; x < MAP_MAX_X; x++) {
            for (uint y = 0; y < MAP_MAX_X; y++) {
                if ((x + y) % 1 != 0) continue;
                Cell cell = {Block(engine->rand(7)), Block(engine->rand(7))};
                set(x, y, 1, cell);
            }
        }
        // for (u8 i = 0; i < u8(ARRAY_LEN(locations)); i++) {
        //     const auto *location = &locations[i];

        //     auto x_start = location->rect.x - location->rect.w / 2;
        //     auto x_end = x_start + location->rect.w;

        //     auto y_start = location->rect.y - location->rect.h / 2;
        //     auto y_end = y_start + location->rect.h;

        //     for (int x = x_start; x < x_end; x++) {
        //         for (int y = y_start; y < y_end; y++) {
        //             set(x, y, 0, u8(location->block));
        //         }
        //     }
    }

    void update(Engine *engine, Fixed<WorldInstance> *instances, Vec3 player_pos) {
        int distance = 32;
        int x_start = std::max(int(player_pos.x) - distance, 0);
        int x_end = std::min(int(player_pos.x) + distance, MAP_MAX_X);

        int y_start = std::max(int(player_pos.y) - distance, 0);
        int y_end = std::min(int(player_pos.y) + distance, MAP_MAX_Y);

        for (int x = x_start; x < x_end; x++) {
            for (int y = y_start; y < y_end; y++) {
                Cell floor = get(x, y, current_level);

                Cell cell = get(x, y, current_level + 1);
                Rect top[2] = {blocks[u8(floor.top)], blocks[u8(cell.top)]};
                Rect block[2] = {blocks[u8(floor.block)], blocks[u8(cell.block)]};

                // cell top
                if (cell.block != Block::air) {
                    Rect texture = top[1];
                    if (cell.top == Block::air) texture = block[1];
                    instances->append({Vec3(x, y, current_level + 1 + 0.5F),
                                       {1, 1},
                                       texture,
                                       WHITE,
                                       Face::z_pos,
                                       0});
                } else {
                    // floor top
                    instances->append({Vec3(x, y, current_level + 0.5F),
                                       {1, 1},
                                       top[0],
                                       WHITE,
                                       Face::z_pos,
                                       0});
                }

                for (u8 i = 0; i < 2; i++) {
                    if (checkFaceVisible(x + 1, y, current_level + i))
                        instances->append({Vec3(x + 0.5, y, current_level + i),
                                           {1, 1},
                                           block[i],
                                           WHITE,
                                           Face::x_pos,
                                           0});

                    if (checkFaceVisible(x - 1, y, current_level + i))
                        instances->append({Vec3(x - 0.5, y, current_level + i),
                                           {1, 1},
                                           block[i],
                                           WHITE,
                                           Face::x_neg,
                                           0});

                    if (checkFaceVisible(x, y + 1, current_level + i))
                        instances->append({Vec3(x, y + 0.5, current_level + i),
                                           {1, 1},
                                           block[i],
                                           WHITE,
                                           Face::y_pos,
                                           0});

                    if (checkFaceVisible(x, y - 1, current_level + i))
                        instances->append({Vec3(x, y - 0.5, current_level + i),
                                           {1, 1},
                                           block[i],
                                           WHITE,
                                           Face::y_neg,
                                           0});
                }
            }
        }
        // for (int x = 0; x < MAP_MAX_X; x++) {
        //     for (int y = 0; y < MAP_MAX_Y; y++) {
        //         for (int z = 0; z < MAP_MAX_Z; z++) {
        //             Block block = Block(get(x, y, z));
        //             if (block == Block::air) continue;
        //             Rect texture = blocks[int(block)];
        //             const Vec3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        //             for (int axis = 0; axis < 3; axis++)
        //                 for (int i = 0; i < 2; i++) {
        //                     const auto pos = Vec3(x, y, z);
        //                     const auto offset = axes[axis] * (1 - i * 2);
        //                     if (!checkFaceVisible(pos + offset)) continue;
        //                     instances->append({Vec3(offset) * 0.25F + pos * 0.5,
        //                                        {0.5, 0.5},
        //                                        texture,
        //                                        WHITE,
        //                                        Face(axis * 2 + i),
        //                                        0});
        //                 }
        //         }
        //     }
        // }
    }
};
