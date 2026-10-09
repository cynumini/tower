#pragma once

#include "shared.cpp"

#include <skn.cpp>
#include <skn_math.cpp>

#include <limits>

// Bloks

Rect blocks[std::numeric_limits<u8>::max()] = {};

struct Block {
    enum class Kind : u8 { air, dirt, grass, water, wood_floor, wall, deep, stone, door };
    enum class Shape : u8 { block, slope, door };
    enum class Direction : u8 {
        sn, // South -> North
        ew, // East  -> West
        ns, // North -> South
        we, // West  -> East
    };
    Kind other : 4;                 // 16 max
    Kind top : 4;                   // 16 max
    Shape shape : 2 = Shape::block; // 4 max
    Direction direction : 2;        // 4 max
    bool see_through : 1;           // 1 max

    static Block door(Direction direction) {
        return {Kind::door, Kind::door, Shape::door, direction, true};
    }

    static Block block(Kind other, Kind top = Kind::air) {
        if (top == Kind::air) top = other;
        return {other, top, Shape::block, Direction::sn, false};
    }

    static Block slope(Kind other, Direction direction) {
        return {other, other, Block::Shape::slope, direction, false};
    }
};

// Locations
static struct Location {
    const char *name;
    Rect rect;
    Block::Kind block;
} locations[2];

struct Map {
    static constexpr Vec3i MAP_SIZE = {256, 256, 256};
    static constexpr Vec3i MAP_MIN = {-128, -128, 0};
    static constexpr Vec3i MAP_MAX = {127, 127, 255};

    Slice<Block> data = {};
    uint current_level = 0;
    bool hide_above_level = false;

    static constexpr bool contains(int x, int y, int z) {
        return x >= MAP_MIN.x and x <= MAP_MAX.x and y >= MAP_MIN.y and y <= MAP_MAX.y and
               z >= MAP_MIN.z and z <= MAP_MAX.z;
    }

    inline size_t index(int x, int y, int z) {
        size_t xi = x - MAP_MIN.x;
        size_t yi = y - MAP_MIN.y;
        size_t zi = z - MAP_MIN.z;
        return xi * MAP_SIZE.y * MAP_SIZE.z + yi * MAP_SIZE.z + zi;
    }

    inline Block &get(int x, int y, int z) {
        assert(contains(x, y, z));
        return data[index(x, y, z)];
    }

    inline void set(int x, int y, int z, Block cell) {
        assert(contains(x, y, z));
        data[index(x, y, z)] = cell;
    }

    inline bool checkFaceVisible(int x, int y, int z) {
        if (!contains(x, y, z)) return true;
        auto &block = get(x, y, z);
        return block.other == Block::Kind::air or block.see_through;
    }

    void init(Engine *engine) {
        // map
        data = arena.alloc<Block>(MAP_SIZE.x * MAP_SIZE.y * MAP_SIZE.z);

        // generate terrain
        for (int x = MAP_MIN.x; x <= MAP_MAX.x; x++) {
            for (int y = MAP_MIN.y; y <= MAP_MAX.y; y++) {
                for (int z = MAP_MIN.z; z <= MAP_MAX.z; z++) {
                    if (z == 0)
                        set(x, y, z, Block::block(Block::Kind::deep));
                    else if (z < 60)
                        set(x, y, z, Block::block(Block::Kind::stone));
                    else if (z < 63)
                        set(x, y, z, Block::block(Block::Kind::dirt));
                    else if (z < 64)
                        set(x, y, z, Block::block(Block::Kind::dirt, Block::Kind::grass));
                }
            }
        }

        // init location
        locations[0] = {"Home", {0, 0, 32, 32}, Block::Kind::dirt};
        locations[1] = {"Town", {32, 0, 32, 32}, Block::Kind::grass};

        for (u8 i = 0; i < u8(ARRAY_LEN(locations)); i++) {
            const auto *location = &locations[i];
            auto x_start = location->rect.x - location->rect.w / 2;
            auto x_end = x_start + location->rect.w;
            auto y_start = location->rect.y - location->rect.h / 2;
            auto y_end = y_start + location->rect.h;
            for (int x = x_start; x < x_end; x++) {
                for (int y = y_start; y < y_end; y++) {
                    set(x, y, 63, Block::block(location->block, location->block));
                }
            }
        }

        // init blocks
        blocks[int(Block::Kind::air)] = {};
        blocks[int(Block::Kind::dirt)] = engine->sprites.get("dirt");
        blocks[int(Block::Kind::grass)] = engine->sprites.get("grass");
        blocks[int(Block::Kind::water)] = engine->sprites.get("water");
        blocks[int(Block::Kind::wood_floor)] = engine->sprites.get("wood_floor");
        blocks[int(Block::Kind::wall)] = engine->sprites.get("wall");
        blocks[int(Block::Kind::deep)] = engine->sprites.get("deep");
        blocks[int(Block::Kind::stone)] = engine->sprites.get("stone");
        blocks[int(Block::Kind::door)] = engine->sprites.get("door");

        // generate house
        int x_offset = -4 + 32;
        int y_offset = -4;

        for (int x = x_offset; x < (9 + x_offset); x++) {
            for (int y = y_offset; y < (9 + y_offset); y++) {
                if ((x > x_offset and y > y_offset) and (x < x_offset + 8 and y < y_offset + 8)) {
                    set(x, y, 64, Block::block(Block::Kind::air, Block::Kind::wood_floor));
                    set(x, y, 65, Block::block(Block::Kind::air, Block::Kind::wood_floor));
                } else {
                    set(x, y, 64, Block::block(Block::Kind::wall));
                    set(x, y, 65, Block::block(Block::Kind::wood_floor));
                }
                set(x, y, 63, Block::block(Block::Kind::dirt, Block::Kind::wood_floor));
            }
        }

        for (int y = y_offset; y < (9 + y_offset); y++) {
            set(x_offset - 1, y, 65,
                Block::slope(Block::Kind::wood_floor, Block::Direction::we));
            set(x_offset + 9, y, 65,
                Block::slope(Block::Kind::wood_floor, Block::Direction::ew));
        }

        set(x_offset + 4, y_offset, 64, Block::door(Block::Direction::sn));
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
                    case Block::Shape::slope: {
                        float top_angle = deg2rad(90 * float(cell.direction));
                        if (draw_top) {
                            instances->squares.append(
                                {Vec3(x, y, z), size, top, top_tint, Face::slope, top_angle});
                        }
                        switch (cell.direction) {
                        case Block::Direction::sn: {
                            if (checkFaceVisible(x + 1, y, z))
                                instances->triangles.append({Vec3(x + 0.5F, y, z), size, other,
                                                             east_tint, Face::x_pos, deg2rad(0)});
                            if (checkFaceVisible(x - 1, y, z))
                                instances->triangles.append({Vec3(x - 0.5F, y, z), size, other,
                                                             west_tint, Face::x_neg,
                                                             deg2rad(180)});
                            if (checkFaceVisible(x, y + 1, z))
                                instances->squares.append({Vec3(x, y + 0.5F, z), size, other,
                                                           other_tint, Face::y_pos, deg2rad(0)});
                            break;
                        }
                        case Block::Direction::ew: {
                            if (checkFaceVisible(x - 1, y, z))
                                instances->squares.append({Vec3(x - 0.5F, y, z), size, other,
                                                           west_tint, Face::x_neg, deg2rad(0)});
                            if (checkFaceVisible(x, y + 1, z))
                                instances->triangles.append({Vec3(x, y + 0.5F, z), size, other,
                                                             other_tint, Face::y_pos,
                                                             deg2rad(0)});
                            if (checkFaceVisible(x, y - 1, z))
                                instances->triangles.append({Vec3(x, y - 0.5F, z), size, other,
                                                             other_tint, Face::y_neg,
                                                             deg2rad(180)});
                            break;
                        }
                        case Block::Direction::ns: {
                            if (checkFaceVisible(x + 1, y, z))
                                instances->triangles.append({Vec3(x + 0.5F, y, z), size, other,
                                                             east_tint, Face::x_pos,
                                                             deg2rad(180)});
                            if (checkFaceVisible(x - 1, y, z))
                                instances->triangles.append({Vec3(x - 0.5F, y, z), size, other,
                                                             west_tint, Face::x_neg, deg2rad(0)});
                            if (checkFaceVisible(x, y + 1, z))
                                instances->squares.append({Vec3(x, y - 0.5F, z), size, other,
                                                           other_tint, Face::y_neg, deg2rad(0)});
                            break;
                        }
                        case Block::Direction::we: {
                            if (checkFaceVisible(x + 1, y, z))
                                instances->squares.append({Vec3(x + 0.5F, y, z), size, other,
                                                           east_tint, Face::x_pos, deg2rad(0)});
                            if (checkFaceVisible(x, y + 1, z))
                                instances->triangles.append({Vec3(x, y + 0.5F, z), size, other,
                                                             other_tint, Face::y_pos,
                                                             deg2rad(180)});
                            if (checkFaceVisible(x, y - 1, z))
                                instances->triangles.append({Vec3(x, y - 0.5F, z), size, other,
                                                             other_tint, Face::y_neg,
                                                             deg2rad(0)});
                            break;
                        }
                        }

                        break;
                    }
                    case Block::Shape::door: {
                        Rect door_front_back = {other.position() + Vec2{0, 4}, {32, 32}};
                        Rect door_top = {top.position(), {32, 4}};
                        Rect door_side = {top.position() + Vec2(32, 4), {4, 32}};
                        constexpr float unit = 1.0F / 32.0F;

                        Vec2 offset = {};
                        switch (cell.direction) {
                        case Block::Direction::sn:
                            offset = {0.0F, -0.5F + 2 * unit};
                            break;
                        case Block::Direction::ew:
                            offset = {0.5F - 2 * unit, 0.0F};
                            break;
                        case Block::Direction::ns:
                            offset = {0.0F, 0.5F - 2 * unit};
                            break;
                        case Block::Direction::we:
                            offset = {-0.5F + 2 * unit, 0.0F};
                            break;
                        }

                        float top_angle = deg2rad(90 * float(cell.direction));
                        if (draw_top) {
                            instances->squares.append({Vec3(x, y, z + 0.5f) + offset,
                                                       {size.x, 4 * unit},
                                                       door_top,
                                                       top_tint,
                                                       Face::z_pos,
                                                       top_angle});
                        }
                        switch (cell.direction) {
                        case Block::Direction::sn:
                        case Block::Direction::ns: {
                            instances->squares.append({Vec3(x, y + 2 * unit, z) + offset, size,
                                                       door_front_back, other_tint, Face::y_pos,
                                                       0});
                            instances->squares.append({Vec3(x, y - 2 * unit, z) + offset, size,
                                                       door_front_back, other_tint, Face::y_neg,
                                                       0});
                            if (checkFaceVisible(x + 1, y, z)) {
                                instances->squares.append({Vec3(x + 0.5, y, z) + offset,
                                                           {4 * unit, size.y},
                                                           door_side,
                                                           east_tint,
                                                           Face::x_pos,
                                                           0});
                            }
                            if (checkFaceVisible(x - 1, y, z)) {
                                instances->squares.append({Vec3(x - 0.5, y, z) + offset,
                                                           {4 * unit, size.y},
                                                           door_side,
                                                           west_tint,
                                                           Face::x_neg,
                                                           0});
                            }
                            break;
                        }
                        case Block::Direction::ew:
                        case Block::Direction::we: {
                            instances->squares.append({Vec3(x + 2 * unit, y, z) + offset, size,
                                                       door_front_back, east_tint, Face::x_pos,
                                                       0});
                            instances->squares.append({Vec3(x - 2 * unit, y, z) + offset, size,
                                                       door_front_back, west_tint, Face::x_neg,
                                                       0});
                            if (checkFaceVisible(x, y + 1, z)) {
                                instances->squares.append({Vec3(x, y + 0.5, z) + offset,
                                                           {4 * unit, size.y},
                                                           door_side,
                                                           other_tint,
                                                           Face::y_pos,
                                                           0});
                            }
                            if (checkFaceVisible(x, y - 1, z)) {
                                instances->squares.append({Vec3(x, y - 0.5, z) + offset,
                                                           {4 * unit, size.y},
                                                           door_side,
                                                           other_tint,
                                                           Face::y_neg,
                                                           0});
                            }
                            break;
                        }
                        }

                        break;
                    }
                    }
                }
            }
        }
    }
};
