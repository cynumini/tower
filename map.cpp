#pragma once

#include "shared.cpp"

#include <skn.cpp>
#include <skn_math.cpp>

// Bloks

Rect blocks[U8_MAX] = {};

struct Block {
    enum class Kind : u8 { air, dirt, grass, water, wood_floor, wall, deep, stone };
    enum class Shape : u8 {
        block,
        slope_sn, // South -> North
        slope_ns, // North -> South
        slope_ew, // East  -> West
        slope_we, // West  -> East
    };

    Kind other;
    Kind top;

    Shape shape = Shape::block;
};

// Locations
static struct Location {
    const char *name;
    Rect rect;
    Block::Kind block;
} locations[2];

struct Map {
    static constexpr Vec3i MAP_SIZE = {256, 256, 256};
    static constexpr Vec3i MAP_MIN = {-MAP_SIZE.x / 2, -MAP_SIZE.y / 2, 0};
    static constexpr Vec3i MAP_MAX = {MAP_SIZE.x / 2 - 1, MAP_SIZE.y / 2 - 1, 255};
    Slice<Block> data = {};
    uint current_level = 0;
    bool hide_above_level = false;

    inline Block &get(int x, int y, int z) {
        x += 128, y += 128, z += 128;
        return data[x * MAP_SIZE.y * MAP_SIZE.z + y * MAP_SIZE.z + z];
    }

    inline void set(int x, int y, int z, Block cell) {
        x += 128, y += 128, z += 128;
        data[x * MAP_SIZE.y * MAP_SIZE.z + y * MAP_SIZE.z + z] = cell;
    }

    inline bool checkFaceVisible(int x, int y, int z) {
        if (x < MAP_MIN.x or y < MAP_MIN.y or z < MAP_MIN.z) return true;
        if (x > MAP_MAX.x or y > MAP_MAX.y or z > MAP_MAX.z) return true;
        return get(x, y, z).other == Block::Kind::air;
    }

    void init(Engine *engine) {
        data = arena.alloc<Block>(MAP_SIZE.x * MAP_SIZE.y * MAP_SIZE.z);

        // location
        locations[0] = {"Home", {0, 0, 32, 32}, Block::Kind::dirt};
        locations[1] = {"Town", {32, 0, 32, 32}, Block::Kind::grass};

        // bloks
        blocks[int(Block::Kind::air)] = {};
        blocks[int(Block::Kind::dirt)] = engine->sprites.get("dirt");
        blocks[int(Block::Kind::grass)] = engine->sprites.get("grass");
        blocks[int(Block::Kind::water)] = engine->sprites.get("water");
        blocks[int(Block::Kind::wood_floor)] = engine->sprites.get("wood_floor");
        blocks[int(Block::Kind::wall)] = engine->sprites.get("wall");
        blocks[int(Block::Kind::deep)] = engine->sprites.get("deep");
        blocks[int(Block::Kind::stone)] = engine->sprites.get("stone");

        // map
        for (int x = MAP_MIN.x; x <= MAP_MAX.x; x++) {
            for (int y = MAP_MIN.y; y <= MAP_MAX.y; y++) {
                for (int z = MAP_MIN.z; z <= MAP_MAX.z; z++) {
                    if (z == 0)
                        set(x, y, z, {Block::Kind::deep, Block::Kind::deep});
                    else if (z < 60)
                        set(x, y, z, {Block::Kind::stone, Block::Kind::stone});
                    else if (z < 63)
                        set(x, y, z, {Block::Kind::dirt, Block::Kind::dirt});
                    else if (z < 64)
                        set(x, y, z, {Block::Kind::dirt, Block::Kind::grass});
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
                    set(x, y, 64, {Block::Kind::air, Block::Kind::wood_floor});
                    set(x, y, 65, {Block::Kind::air, Block::Kind::wood_floor});
                } else {
                    set(x, y, 64, {Block::Kind::wall, Block::Kind::wall});
                    set(x, y, 65, {Block::Kind::wood_floor, Block::Kind::wood_floor});
                }
                set(x, y, 63, {Block::Kind::dirt, Block::Kind::wood_floor});
            }
        }
        for (int y = y_offset; y < (9 + y_offset); y++) {
            set(x_offset - 1, y, 65,
                {Block::Kind::wood_floor, Block::Kind::wood_floor, Block::Shape::slope_we});
            set(x_offset + 9, y, 65,
                {Block::Kind::wood_floor, Block::Kind::wood_floor, Block::Shape::slope_ew});
        }
    }

    void update(Engine *engine, WorldInstances *instances, Vec3 player_pos) {
        current_level = int(player_pos.z);

        constexpr Color top_tint = WHITE;
        constexpr Color east_tint = WHITE;
        constexpr Color west_tint = 0xBFBFBFFF;
        constexpr Color other_tint = 0xDFDFDFFF;
        constexpr Vec2 size = {1, 1};

        constexpr int distance = 19;
        Vec3i start = player_pos - distance, end = player_pos + distance;
        for (int i = 0; i < 3; i++) {
            start[i] = std::max(start[i], MAP_MIN[i]);
            end[i] = std::min(end[i], MAP_MAX[i]);
        }
        if (hide_above_level) end.z = std::min(int(player_pos.z + 1), end.z);

        for (int x = start.x; x < end.x; x++) {
            for (int y = start.y; y < end.y; y++) {
                for (int z = start.z; z < end.z; z++) {
                    const Block &cell = get(x, y, z);
                    if (cell.other == Block::Kind::air and cell.top == Block::Kind::air) continue;
                    const bool draw_top = hide_above_level and int(player_pos.z) == z
                                              ? cell.other != Block::Kind::air
                                              : checkFaceVisible(x, y, z + 1);
                    if (!draw_top and cell.other == Block::Kind::air) continue;

                    const Rect other = blocks[int(cell.other)];
                    const Rect top = blocks[int(cell.top)];

                    switch (cell.shape) {
                    case Block::Shape::block: {
                        if (draw_top) {
                            instances->squares.append(
                                {Vec3(x, y, z + 0.5F), size, top, top_tint, Face::z_pos, 0});
                        }
                        if (checkFaceVisible(x + 1, y, z))
                            instances->squares.append(
                                {Vec3(x + 0.5F, y, z), size, other, east_tint, Face::x_pos, 0});
                        if (checkFaceVisible(x - 1, y, z))
                            instances->squares.append(
                                {Vec3(x - 0.5F, y, z), size, other, west_tint, Face::x_neg, 0});
                        if (checkFaceVisible(x, y + 1, z))
                            instances->squares.append(
                                {Vec3(x, y + 0.5F, z), size, other, other_tint, Face::y_pos, 0});
                        if (checkFaceVisible(x, y - 1, z))
                            instances->squares.append(
                                {Vec3(x, y - 0.5F, z), size, other, other_tint, Face::y_neg, 0});
                        break;
                    }
                    case Block::Shape::slope_sn: {
                        if (draw_top) {
                            instances->squares.append(
                                {Vec3(x, y, z), size, top, top_tint, Face::slope, deg2rad(0)});
                        }
                        if (checkFaceVisible(x + 1, y, z))
                            instances->triangles.append({Vec3(x + 0.5F, y, z), size, other,
                                                         east_tint, Face::x_pos, deg2rad(0)});
                        if (checkFaceVisible(x - 1, y, z))
                            instances->triangles.append({Vec3(x - 0.5F, y, z), size, other,
                                                         west_tint, Face::x_neg, deg2rad(180)});
                        if (checkFaceVisible(x, y + 1, z))
                            instances->squares.append({Vec3(x, y + 0.5F, z), size, other,
                                                       other_tint, Face::y_pos, deg2rad(0)});
                        break;
                    }
                    case Block::Shape::slope_ns: {
                        if (draw_top) {
                            instances->squares.append(
                                {Vec3(x, y, z), size, top, top_tint, Face::slope, deg2rad(180)});
                        }
                        if (checkFaceVisible(x + 1, y, z))
                            instances->triangles.append({Vec3(x + 0.5F, y, z), size, other,
                                                         east_tint, Face::x_pos, deg2rad(180)});
                        if (checkFaceVisible(x - 1, y, z))
                            instances->triangles.append({Vec3(x - 0.5F, y, z), size, other,
                                                         west_tint, Face::x_neg, deg2rad(0)});
                        if (checkFaceVisible(x, y + 1, z))
                            instances->squares.append({Vec3(x, y - 0.5F, z), size, other,
                                                       other_tint, Face::y_neg, deg2rad(0)});
                        break;
                    }
                    case Block::Shape::slope_ew: {
                        if (draw_top) {
                            instances->squares.append(
                                {Vec3(x, y, z), size, top, top_tint, Face::slope, deg2rad(90)});
                        }
                        if (checkFaceVisible(x - 1, y, z))
                            instances->squares.append({Vec3(x - 0.5F, y, z), size, other,
                                                       west_tint, Face::x_neg, deg2rad(0)});
                        if (checkFaceVisible(x, y + 1, z))
                            instances->triangles.append({Vec3(x, y + 0.5F, z), size, other,
                                                         other_tint, Face::y_pos, deg2rad(0)});
                        if (checkFaceVisible(x, y - 1, z))
                            instances->triangles.append({Vec3(x, y - 0.5F, z), size, other,
                                                         other_tint, Face::y_neg, deg2rad(180)});
                        break;
                    }
                    case Block::Shape::slope_we: {
                        if (draw_top) {
                            instances->squares.append(
                                {Vec3(x, y, z), size, top, top_tint, Face::slope, deg2rad(-90)});
                        }
                        if (checkFaceVisible(x + 1, y, z))
                            instances->squares.append({Vec3(x + 0.5F, y, z), size, other,
                                                       east_tint, Face::x_pos, deg2rad(0)});
                        if (checkFaceVisible(x, y + 1, z))
                            instances->triangles.append({Vec3(x, y + 0.5F, z), size, other,
                                                         other_tint, Face::y_pos, deg2rad(180)});
                        if (checkFaceVisible(x, y - 1, z))
                            instances->triangles.append({Vec3(x, y - 0.5F, z), size, other,
                                                         other_tint, Face::y_neg, deg2rad(0)});
                        break;
                    }
                    }
                }
            }
        }
    }
};
