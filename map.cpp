#pragma once

#include "shared.cpp"

#include <initializer_list>
#include <limits>

static constexpr int CHUNK_MAX_X = 16;
static constexpr int CHUNK_MAX_Y = 16;
static constexpr int MAP_MAX_Z = 255;

// Material
enum struct Material : u8 { air, deep, stone, dirt, grass, wood };

static constexpr const char *BlockKey[]{
    [int(Material::air)] = "",        [int(Material::deep)] = "deep",
    [int(Material::stone)] = "stone", [int(Material::dirt)] = "dirt",
    [int(Material::grass)] = "grass", [int(Material::wood)] = "wood",
};

static_assert(ARRAY_LEN(BlockKey) <= std::numeric_limits<u8>::max());

static Rect MaterialSprite[ARRAY_LEN(BlockKey)] = {};

void initMaterialSprite(Engine *engine) {
    // skip air
    for (size_t i = size_t(Material::air) + 1; i < ARRAY_LEN(BlockKey); i++) {
        MaterialSprite[i] = engine->sprites.get(BlockKey[i]);
    }
}

// Cell
struct Cell {
    enum class Type : u8 { block, slope, surface };
    enum class Rotation : u8 { sn, we, ns, ew };
    Material a : 8;
    Material b : 8;
    Type type : 3;         // 4
    Rotation rotation : 3; // 4

    static constexpr Cell block(Material material) {
        return {.a = material, .type = Type::block};
    }

    static constexpr Cell slope(Material material, Rotation rotation) {
        return {.a = material, .type = Type::slope, .rotation = rotation};
    }

    static constexpr Cell surface(Material ceiling, Material floor) {
        return {.a = ceiling, .b = floor, .type = Type::surface};
    }

    inline bool obscures_face(Face face) {
        switch (type) {
        case Type::block:
        case Type::slope:
            return a != Material::air;
        case Type::surface:
            if (face == Face::z_pos) return a != Material::air;
            return false;
        }
    }

    inline bool is_empty() {
        switch (type) {
        case Type::block:
        case Type::slope:
            return a == Material::air;
        case Type::surface:
            return a == Material::air or b == Material::air;
        }
    }

    inline bool has_floor() { return a != Material::air; }
    inline bool has_ceiling() { return b != Material::air; }

    inline Rect get_block_sprite() {
        assert(type == Type::block);
        return MaterialSprite[int(a)];
    }

    inline Rect get_slope_sprite() {
        assert(type == Type::slope);
        return MaterialSprite[int(a)];
    }

    inline Rect get_floor_sprite() {
        assert(type == Type::surface);
        return MaterialSprite[int(a)];
    }

    inline Rect get_ceiling_sprite() {
        assert(type == Type::surface);
        return MaterialSprite[int(b)];
    }
};

// Chunk
struct Chunk {
    Cell data[CHUNK_MAX_X][CHUNK_MAX_Y][MAP_MAX_Z] = {};
};

struct Map {
    Slice<Chunk> chunks;

    bool hide_above_level = false;

    inline bool is_visible(Vec3i v, Face face) {
        if (v.x < 0 or v.y < 0 or v.z < 0) return true;
        if (v.x >= CHUNK_MAX_X or v.y >= CHUNK_MAX_Y or v.z >= MAP_MAX_Z) return true;
        return !chunks[0].data[v.x][v.y][v.z].obscures_face(face);
    }

    void init(Engine *engine) {
        initMaterialSprite(engine);

        chunks = arena.alloc<Chunk>(1);

        for (int z = 0; z < MAP_MAX_Z; z++) {
            for (int y = 0; y < CHUNK_MAX_Y; y++) {
                for (int x = 0; x < CHUNK_MAX_X; x++) {
                    if (z == 0)
                        chunks[0].data[x][y][z] = Cell::block(Material::deep);
                    else if (z < 60)
                        chunks[0].data[x][y][z] = Cell::block(Material::stone);
                    else if (z < 64)
                        chunks[0].data[x][y][z] = Cell::block(Material::dirt);
                    else if (z == 64)
                        chunks[0].data[x][y][z] = Cell::surface(Material::grass, Material::air);
                }
            }
        }
        chunks[0].data[0][0][64] = Cell::slope(Material::wood, Cell::Rotation::sn);
    }

    void update(Engine *engine, WorldInstances *instances, Vec3 player_pos) {
        for (int z = 0; z < MAP_MAX_Z; z++) {
            for (int y = 0; y < CHUNK_MAX_Y; y++) {
                for (int x = 0; x < CHUNK_MAX_X; x++) {
                    Cell cell = chunks[0].data[x][y][z];
                    Vec3 p = Vec3(x, y, z) + Vec3(0.5F, 0.5F, 0);
                    Vec3i position = {x, y, z};
                    Vec3i positions[6] = {
                        {0, 1, 0}, {1, 0, 0}, {0, -1, 0}, {-1, 0, 0}, {0, 0, 1}};
                    switch (cell.type) {
                    case Cell::Type::block: {
                        if (cell.is_empty()) continue;
                        auto sprite = cell.get_block_sprite();

                        for (size_t i = 0; i < ARRAY_LEN(positions); i++) {
                            if (is_visible(position + positions[i], Face(i)))
                                instances->squares.append(
                                    {p + positions[i] * 0.5F, {1, 1}, sprite, WHITE, Face(i), 0});
                        }
                    } break;
                    case Cell::Type::slope: {
                        auto sprite = cell.get_slope_sprite();
                        if (is_visible(position + positions[int(cell.rotation)],
                                       Face(cell.rotation)))
                            instances->squares.append({p + positions[int(cell.rotation)] * 0.5,
                                                       {1, 1},
                                                       sprite,
                                                       WHITE,
                                                       Face(cell.rotation),
                                                       0});
                        instances->squares.append({p,
                                                   {1, 1},
                                                   sprite,
                                                   WHITE,
                                                   Face::slope,
                                                   -deg2rad(int(cell.rotation) * 90)});
                        for (int i = 0; i < 2; i++) {
                            const bool second = i == 1;
                            const int a = (cell.rotation == Cell::Rotation::sn ||
                                           cell.rotation == Cell::Rotation::ew)
                                              ? (second ? 180 : 0)
                                              : (second ? 0 : 180);
                            const int f = (cell.rotation == Cell::Rotation::sn ||
                                           cell.rotation == Cell::Rotation::ns)
                                              ? (second ? 3 : 1)
                                              : (second ? 2 : 0);
                            instances->triangles.append({p + positions[f] * 0.5,
                                                         {1, 1},
                                                         sprite,
                                                         WHITE,
                                                         Face(f),
                                                         deg2rad(a)});
                        }
                    } break;
                    case Cell::Type::surface: {
                        if (cell.has_ceiling() and is_visible({x, y, z + 1}, Face::z_pos))
                            instances->squares.append({p + Vec3(0, 0, 0.5F),
                                                       {1, 1},
                                                       cell.get_ceiling_sprite(),
                                                       WHITE,
                                                       Face::z_pos,
                                                       0});
                        if (cell.has_floor())
                            instances->squares.append({p - Vec3(0, 0, 0.5F),
                                                       {1, 1},
                                                       cell.get_floor_sprite(),
                                                       WHITE,
                                                       Face::z_pos,
                                                       0});
                    } break;
                    }
                }
            }
        }
    }
};
