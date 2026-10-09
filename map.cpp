#pragma once

#include "shared.cpp"

#include <skn.cpp>
#include <skn_math.cpp>

#include <limits>

// Textures
enum class TextureId : u8 { air, dirt, grass, water, wood, wall, deep, stone, door, window };

Rect textures[std::numeric_limits<u8>::max()] = {};

// Surface
struct Surface {
    enum class Id : u8 { none, grass, wood };
    TextureId texture;

    const Rect &getTexture() const { return textures[u8(texture)]; }
};

Surface surfaces[std::numeric_limits<u8>::max()] = {};

// Blocks
struct Block {
    enum class Id : u8 { deep, dirt, stone, wall, wood, wood_slope };
    enum class Type : u8 { block, slope };
    Type type;
    TextureId texture;

    const Rect &getTexture() const { return textures[u8(texture)]; }
};

Block blocks[std::numeric_limits<u8>::max()] = {};

// Cell
enum class Direction : u8 {
    sn, // South -> North
    ew, // East  -> West
    ns, // North -> South
    we, // West  -> East
};

struct Cell {
    enum class Type : u8 { empty, block };
    struct Metadata {
        Type type : 1;
        Direction direction : 2 = Direction::sn;
    };
    u8 first_id : 8;
    u8 second_id : 8;
    Metadata metadata;

    const Block &getBlock() const {
        assert(metadata.type == Type::block);
        return blocks[first_id];
    }

    const Surface &getFloor() const {
        assert(metadata.type == Type::empty);
        return surfaces[first_id];
    }

    const Surface &getCeiling() const {
        assert(metadata.type == Type::empty);
        return surfaces[second_id];
    }
};

struct OldBlock {
    enum class Shape : u8 { block, slope, door, window };

    TextureId other;            // 16 max
    TextureId top;              // 16 max
    TextureId front;            // 16 max
    Shape shape = Shape::block; // 4 max
    bool see_through : 1;       // 1 max

    static OldBlock door() {
        return {TextureId::door, TextureId::door, TextureId::door, Shape::door, true};
    }

    static OldBlock block(TextureId other, TextureId top = TextureId::air) {
        if (top == TextureId::air) top = other;
        return {other, top, other, Shape::block, false};
    }

    static OldBlock slope(TextureId other) {
        return {other, other, other, OldBlock::Shape::slope, false};
    }
};

OldBlock old_blocks[std::numeric_limits<u8>::max()] = {};

enum class BlockId { deep, dirt, grass, stone };

struct OldCell {
    enum class Direction : u8 {
        sn, // South -> North
        ew, // East  -> West
        ns, // North -> South
        we, // West  -> East
    };
    BlockId id;
    Direction direction = Direction::sn;
};

// Locations
static struct Location {
    const char *name;
    Rect rect;
    Surface::Id floor;
} locations[2];

struct Map {
    static constexpr Vec3i MAP_SIZE = {256, 256, 256};
    static constexpr Vec3i MAP_MIN = {-128, -128, 0};
    static constexpr Vec3i MAP_MAX = {127, 127, 255};

    Slice<Cell> data = {};
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

    inline Cell &get(int x, int y, int z) {
        assert(contains(x, y, z));
        return data[index(x, y, z)];
    }

    // inline OldBlock &getBlock(int x, int y, int z) { return blocks[int(getCell(x, y, z).id)]; }

    inline void setBlock(int x, int y, int z, Block::Id id) {
        assert(contains(x, y, z));
        data[index(x, y, z)] = {u8(id), 0, {Cell::Type::block}};
    }

    inline void setSurface(int x, int y, int z, Surface::Id floor, Surface::Id ceiling) {
        assert(contains(x, y, z));
        data[index(x, y, z)] = {u8(floor), u8(ceiling), {Cell::Type::empty}};
    }

    inline bool checkFaceVisible(Vec3i v, Face face) {
        if (!contains(v.x, v.y, v.z)) return true;
        const auto &cell = get(v.x, v.y, v.z);
        if (cell.metadata.type == Cell::Type::empty) {
            if (face == Face::z_pos) return Surface::Id(cell.first_id) == Surface::Id::none;
            return true;
        }
        return false;
    }

    void init(Engine *engine) {
        // map
        data = arena.alloc<Cell>(MAP_SIZE.x * MAP_SIZE.y * MAP_SIZE.z);

        // init texture
        textures[int(TextureId::air)] = {};
        textures[int(TextureId::dirt)] = engine->sprites.get("dirt");
        textures[int(TextureId::grass)] = engine->sprites.get("grass");
        textures[int(TextureId::water)] = engine->sprites.get("water");
        textures[int(TextureId::wood)] = engine->sprites.get("wood_floor");
        textures[int(TextureId::wall)] = engine->sprites.get("wall");
        textures[int(TextureId::deep)] = engine->sprites.get("deep");
        textures[int(TextureId::stone)] = engine->sprites.get("stone");
        textures[int(TextureId::door)] = engine->sprites.get("door");

        // init block
        blocks[int(Block::Id::deep)] = {Block::Type::block, TextureId::deep};
        blocks[int(Block::Id::dirt)] = {Block::Type::block, TextureId::dirt};
        blocks[int(Block::Id::stone)] = {Block::Type::block, TextureId::stone};
        blocks[int(Block::Id::wall)] = {Block::Type::block, TextureId::wall};
        blocks[int(Block::Id::wood)] = {Block::Type::block, TextureId::wood};
        blocks[int(Block::Id::wood_slope)] = {Block::Type::slope, TextureId::wood};

        surfaces[int(Surface::Id::grass)] = {TextureId::grass};
        surfaces[int(Surface::Id::wood)] = {TextureId::wood};

        // generate terrain
        for (int x = MAP_MIN.x; x <= MAP_MAX.x; x++) {
            for (int y = MAP_MIN.y; y <= MAP_MAX.y; y++) {
                for (int z = MAP_MIN.z; z <= MAP_MAX.z; z++) {
                    if (z == 0)
                        setBlock(x, y, z, Block::Id::deep);
                    else if (z < 60)
                        setBlock(x, y, z, Block::Id::stone);
                    else if (z < 64)
                        setBlock(x, y, z, Block::Id::dirt);
                    else if (z < 65)
                        setSurface(x, y, z, Surface::Id::grass, Surface::Id::none);
                }
            }
        }

        // init location
        locations[0] = {"Home", {0, 0, 32, 32}, Surface::Id::none};
        locations[1] = {"Town", {32, 0, 32, 32}, Surface::Id::grass};

        for (u8 i = 0; i < u8(ARRAY_LEN(locations)); i++) {
            const auto *location = &locations[i];
            auto x_start = location->rect.x - location->rect.w / 2;
            auto x_end = x_start + location->rect.w;
            auto y_start = location->rect.y - location->rect.h / 2;
            auto y_end = y_start + location->rect.h;
            for (int x = x_start; x < x_end; x++) {
                for (int y = y_start; y < y_end; y++) {
                    setSurface(x, y, 64, location->floor, Surface::Id::none);
                }
            }
        }

        // generate house
        int x_offset = -4 + 32;
        int y_offset = -4;

        for (int x = x_offset; x < (9 + x_offset); x++) {
            for (int y = y_offset; y < (9 + y_offset); y++) {
                if ((x > x_offset and y > y_offset) and (x < x_offset + 8 and y < y_offset + 8)) {
                    setSurface(x, y, 64, Surface::Id::wood, Surface::Id::wood);
                    setSurface(x, y, 65, Surface::Id::wood, Surface::Id::wood);
                } else {
                    setBlock(x, y, 64, Block::Id::wall);
                    setBlock(x, y, 65, Block::Id::wood);
                }
            }
        }

        for (int y = y_offset; y < (9 + y_offset); y++) {
            // setBlock(x, y, 64, Block::Id::wood_slope);
            //  set(x_offset - 1, y, 65,
            //      OldBlock::slope(Block::Kind::wood_floor, Block::OldCell::we));
            //  set(x_offset + 9, y, 65,
            //      OldBlock::slope(Block::Kind::wood_floor, Block::OldCell::ew));
        }

        // set(x_offset + 4, y_offset, 64, OldBlock::door(Block::OldCell::sn));
    }

    void drawBlock(WorldInstances *instances, const Block &block, Vec3i pos, Vec3 size,
                   const Color tint[5], bool draw_top) {
        Vec3i axes[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};

        for (size_t i = 0; i < ARRAY_LEN(axes); i++) {
            if (checkFaceVisible(pos + axes[i], Face(i)))
                instances->squares.append(
                    {pos + axes[i] * 0.5F, size, block.getTexture(), tint[i], Face(i), 0});
        }
        if (draw_top)
            instances->squares.append(
                {pos + Vec3(0, 0, 0.5F), size, block.getTexture(), tint[4], Face(4), 0});
    }

    void update(Engine *engine, WorldInstances *instances, Vec3 player_pos) {
        current_level = int(player_pos.z);
        constexpr int distance = 19;

        constexpr Color tint[] = {WHITE, 0xBFBFBFFF, 0xDFDFDFFF, 0xDFDFDFFF, WHITE};

        constexpr Vec2 size = {1, 1};

        Vec3i start = player_pos - distance, end = player_pos + distance;
        for (int i = 0; i < 3; i++) {
            start[i] = std::max(start[i], MAP_MIN[i]);
            end[i] = std::min(end[i], MAP_MAX[i]);
        }
        if (hide_above_level) end.z = std::min(int(player_pos.z + 1), end.z);

        for (int x = start.x; x < end.x; x++) {
            for (int y = start.y; y < end.y; y++) {
                for (int z = start.z; z < end.z; z++) {
                    // if (x == 35 and y == 3 and z == 65) __builtin_debugtrap();
                    const Cell &cell = get(x, y, z);
                    const bool draw_top = hide_above_level and int(player_pos.z) == z
                                              ? cell.metadata.type == Cell::Type::block
                                              : checkFaceVisible({x, y, z + 1}, Face::z_pos);
                    Vec3i pos = Vec3i(x, y, z);
                    switch (cell.metadata.type) {
                    case Cell::Type::empty: {
                        const Surface &ceiling = cell.getCeiling();
                        if (draw_top) {
                            if (Surface::Id(cell.second_id) != Surface::Id::none)
                                instances->squares.append({pos + Vec3(0, 0, 0.5F), size,
                                                           ceiling.getTexture(), tint[4], Face(4),
                                                           0});
                        }
                        if (Surface::Id(cell.first_id) != Surface::Id::none) {
                            const Surface &floor = cell.getFloor();
                            instances->squares.append({pos - Vec3(0, 0, 0.5F), size,
                                                       floor.getTexture(), tint[4], Face(4), 0});
                        }
                        break;
                    }
                    case Cell::Type::block: {
                        const Block &block = cell.getBlock();
                        switch (block.type) {
                        case Block::Type::block:
                            drawBlock(instances, block, pos, size, tint, draw_top);
                            break;
                        case Block::Type::slope:
                            break;
                        }

                        break;
                    }
                    }
                    //                 const OldBlock &cell = get(x, y, z);
                    //                 if (cell.other == Block::Kind::air and cell.top ==
                    //                 Block::Kind::air) continue;
                    //                 if (!draw_top and cell.other == Block::Kind::air) continue;

                    //                 const Rect other = textures[int(cell.other)];
                    //                 const Rect top = textures[int(cell.top)];

                    //                 switch (cell.shape) {
                    //                 case OldBlock::Shape::block: {
                    //                     if (draw_top) {
                    //                         instances->squares.append(
                    //                             {Vec3(x, y, z + 0.5F), size, top, top_tint,
                    //                             Face::z_pos, 0});
                    //                     }
                    //                     if (checkFaceVisible(x + 1, y, z))
                    //                         instances->squares.append(
                    //                             {Vec3(x + 0.5F, y, z), size, other, east_tint,
                    //                             Face::x_pos, 0});
                    //                     if (checkFaceVisible(x - 1, y, z))
                    //                         instances->squares.append(
                    //                             {Vec3(x - 0.5F, y, z), size, other, west_tint,
                    //                             Face::x_neg, 0});
                    //                     if (checkFaceVisible(x, y + 1, z))
                    //                         instances->squares.append(
                    //                             {Vec3(x, y + 0.5F, z), size, other, other_tint,
                    //                             Face::y_pos, 0});
                    //                     if (checkFaceVisible(x, y - 1, z))
                    //                         instances->squares.append(
                    //                             {Vec3(x, y - 0.5F, z), size, other, other_tint,
                    //                             Face::y_neg, 0});
                    //                     break;
                    //                 }
                    //                 case OldBlock::Shape::slope: {
                    //                     float top_angle = deg2rad(90 * float(cell.direction));
                    //                     if (draw_top) {
                    //                         instances->squares.append(
                    //                             {Vec3(x, y, z), size, top, top_tint,
                    //                             Face::slope, top_angle});
                    //                     }
                    //                     switch (cell.direction) {
                    //                     case Block::Direction::sn: {
                    //                         if (checkFaceVisible(x + 1, y, z))
                    //                             instances->triangles.append({Vec3(x + 0.5F, y,
                    //                             z), size, other,
                    //                                                          east_tint,
                    //                                                          Face::x_pos,
                    //                                                          deg2rad(0)});
                    //                         if (checkFaceVisible(x - 1, y, z))
                    //                             instances->triangles.append({Vec3(x - 0.5F, y,
                    //                             z), size, other,
                    //                                                          west_tint,
                    //                                                          Face::x_neg,
                    //                                                          deg2rad(180)});
                    //                         if (checkFaceVisible(x, y + 1, z))
                    //                             instances->squares.append({Vec3(x, y + 0.5F,
                    //                             z), size, other,
                    //                                                        other_tint,
                    //                                                        Face::y_pos,
                    //                                                        deg2rad(0)});
                    //                         break;
                    //                     }
                    //                     case Block::Direction::ew: {
                    //                         if (checkFaceVisible(x - 1, y, z))
                    //                             instances->squares.append({Vec3(x - 0.5F, y,
                    //                             z), size, other,
                    //                                                        west_tint,
                    //                                                        Face::x_neg,
                    //                                                        deg2rad(0)});
                    //                         if (checkFaceVisible(x, y + 1, z))
                    //                             instances->triangles.append({Vec3(x, y + 0.5F,
                    //                             z), size, other,
                    //                                                          other_tint,
                    //                                                          Face::y_pos,
                    //                                                          deg2rad(0)});
                    //                         if (checkFaceVisible(x, y - 1, z))
                    //                             instances->triangles.append({Vec3(x, y - 0.5F,
                    //                             z), size, other,
                    //                                                          other_tint,
                    //                                                          Face::y_neg,
                    //                                                          deg2rad(180)});
                    //                         break;
                    //                     }
                    //                     case Block::Direction::ns: {
                    //                         if (checkFaceVisible(x + 1, y, z))
                    //                             instances->triangles.append({Vec3(x + 0.5F, y,
                    //                             z), size, other,
                    //                                                          east_tint,
                    //                                                          Face::x_pos,
                    //                                                          deg2rad(180)});
                    //                         if (checkFaceVisible(x - 1, y, z))
                    //                             instances->triangles.append({Vec3(x - 0.5F, y,
                    //                             z), size, other,
                    //                                                          west_tint,
                    //                                                          Face::x_neg,
                    //                                                          deg2rad(0)});
                    //                         if (checkFaceVisible(x, y + 1, z))
                    //                             instances->squares.append({Vec3(x, y - 0.5F,
                    //                             z), size, other,
                    //                                                        other_tint,
                    //                                                        Face::y_neg,
                    //                                                        deg2rad(0)});
                    //                         break;
                    //                     }
                    //                     case Block::Direction::we: {
                    //                         if (checkFaceVisible(x + 1, y, z))
                    //                             instances->squares.append({Vec3(x + 0.5F, y,
                    //                             z), size, other,
                    //                                                        east_tint,
                    //                                                        Face::x_pos,
                    //                                                        deg2rad(0)});
                    //                         if (checkFaceVisible(x, y + 1, z))
                    //                             instances->triangles.append({Vec3(x, y + 0.5F,
                    //                             z), size, other,
                    //                                                          other_tint,
                    //                                                          Face::y_pos,
                    //                                                          deg2rad(180)});
                    //                         if (checkFaceVisible(x, y - 1, z))
                    //                             instances->triangles.append({Vec3(x, y - 0.5F,
                    //                             z), size, other,
                    //                                                          other_tint,
                    //                                                          Face::y_neg,
                    //                                                          deg2rad(0)});
                    //                         break;
                    //                     }
                    //                     }

                    //                     break;
                    //                 }
                    //                 case OldBlock::Shape::door: {
                    //                     Rect door_front_back = {other.position() + Vec2{0, 4},
                    //                     {32, 32}}; Rect door_top = {top.position(), {32, 4}};
                    //                     Rect door_side = {top.position() + Vec2(32, 4), {4,
                    //                     32}}; constexpr float unit = 1.0F / 32.0F;

                    //                     Vec2 offset = {};
                    //                     switch (cell.direction) {
                    //                     case Block::Direction::sn:
                    //                         offset = {0.0F, -0.5F + 2 * unit};
                    //                         break;
                    //                     case Block::Direction::ew:
                    //                         offset = {0.5F - 2 * unit, 0.0F};
                    //                         break;
                    //                     case Block::Direction::ns:
                    //                         offset = {0.0F, 0.5F - 2 * unit};
                    //                         break;
                    //                     case Block::Direction::we:
                    //                         offset = {-0.5F + 2 * unit, 0.0F};
                    //                         break;
                    //                     }

                    //                     float top_angle = deg2rad(90 * float(cell.direction));
                    //                     if (draw_top) {
                    //                         instances->squares.append({Vec3(x, y, z + 0.5f) +
                    //                         offset,
                    //                                                    {size.x, 4 * unit},
                    //                                                    door_top,
                    //                                                    top_tint,
                    //                                                    Face::z_pos,
                    //                                                    top_angle});
                    //                     }
                    //                     switch (cell.direction) {
                    //                     case Block::Direction::sn:
                    //                     case Block::Direction::ns: {
                    //                         instances->squares.append({Vec3(x, y + 2 * unit, z)
                    //                         + offset, size,
                    //                                                    door_front_back,
                    //                                                    other_tint, Face::y_pos,
                    //                                                    0});
                    //                         instances->squares.append({Vec3(x, y - 2 * unit, z)
                    //                         + offset, size,
                    //                                                    door_front_back,
                    //                                                    other_tint, Face::y_neg,
                    //                                                    0});
                    //                         if (checkFaceVisible(x + 1, y, z)) {
                    //                             instances->squares.append({Vec3(x + 0.5, y, z)
                    //                             + offset,
                    //                                                        {4 * unit, size.y},
                    //                                                        door_side,
                    //                                                        east_tint,
                    //                                                        Face::x_pos,
                    //                                                        0});
                    //                         }
                    //                         if (checkFaceVisible(x - 1, y, z)) {
                    //                             instances->squares.append({Vec3(x - 0.5, y, z)
                    //                             + offset,
                    //                                                        {4 * unit, size.y},
                    //                                                        door_side,
                    //                                                        west_tint,
                    //                                                        Face::x_neg,
                    //                                                        0});
                    //                         }
                    //                         break;
                    //                     }
                    //                     case Block::Direction::ew:
                    //                     case Block::Direction::we: {
                    //                         instances->squares.append({Vec3(x + 2 * unit, y, z)
                    //                         + offset, size,
                    //                                                    door_front_back,
                    //                                                    east_tint, Face::x_pos,
                    //                                                    0});
                    //                         instances->squares.append({Vec3(x - 2 * unit, y, z)
                    //                         + offset, size,
                    //                                                    door_front_back,
                    //                                                    west_tint, Face::x_neg,
                    //                                                    0});
                    //                         if (checkFaceVisible(x, y + 1, z)) {
                    //                             instances->squares.append({Vec3(x, y + 0.5, z)
                    //                             + offset,
                    //                                                        {4 * unit, size.y},
                    //                                                        door_side,
                    //                                                        other_tint,
                    //                                                        Face::y_pos,
                    //                                                        0});
                    //                         }
                    //                         if (checkFaceVisible(x, y - 1, z)) {
                    //                             instances->squares.append({Vec3(x, y - 0.5, z)
                    //                             + offset,
                    //                                                        {4 * unit, size.y},
                    //                                                        door_side,
                    //                                                        other_tint,
                    //                                                        Face::y_neg,
                    //                                                        0});
                    //                         }
                    //                         break;
                    //                     }
                    //                     }

                    //                     break;
                    //                 }
                    //                 }
                }
            }
        }
    }
};
