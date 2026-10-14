#pragma once

#include "shared.cpp"

#include <limits>

static constexpr int MAP_Z_LEN = 3;
static constexpr int SEA_LEVEL = 1;

// Material
enum struct Material : u8 { air, deep, stone, dirt, grass, wood, wall, water };

static constexpr const char *BlockKey[]{
    [int(Material::air)] = "",        [int(Material::deep)] = "deep",
    [int(Material::stone)] = "stone", [int(Material::dirt)] = "dirt",
    [int(Material::grass)] = "grass", [int(Material::wood)] = "wood",
    [int(Material::wall)] = "wall",   [int(Material::water)] = "water",
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
enum class Rotation : u8 { sn, we, ns, ew };

struct Cell {
    enum class Type : u8 { block, slope, surface };
    Material a : 8;        // block, floor, slope
    Material b : 8;        // ceiling
    Type type : 3;         // 4
    Rotation rotation : 3; // 4

    static constexpr Cell block(Material material) {
        return {.a = material, .type = Type::block};
    }

    static constexpr Cell slope(Material material, Rotation rotation) {
        return {.a = material, .type = Type::slope, .rotation = rotation};
    }

    static constexpr Cell surface(Material floor, Material ceiling = Material::air) {
        return {.a = floor, .b = ceiling, .type = Type::surface};
    }

    inline bool obscuresFace(Face face) const {
        switch (type) {
        case Type::block:
        case Type::slope:
            return a != Material::air;
        case Type::surface:
            if (face == Face::z_pos) return a != Material::air;
            return false;
        }
    }

    inline bool isEmpty() const {
        switch (type) {
        case Type::block:
        case Type::slope:
            return a == Material::air;
        case Type::surface:
            return a == Material::air and b == Material::air;
        }
    }

    inline bool hasFloor() const { return a != Material::air; }
    inline bool hasCeiling() const { return b != Material::air; }

    inline Rect getBlockSprite() const {
        assert(type == Type::block);
        return MaterialSprite[int(a)];
    }

    inline Rect getSlopeSprite() const {
        assert(type == Type::slope);
        return MaterialSprite[int(a)];
    }

    inline Rect getFloorSprite() const {
        assert(type == Type::surface);
        return MaterialSprite[int(a)];
    }

    inline Rect getCeilingSprite() const {
        assert(type == Type::surface);
        return MaterialSprite[int(b)];
    }
};

// Chunk
struct Chunk {
    static constexpr Vec2i SIZE = {16, 16};
    Vec2i index;
    Cell data[SIZE.x][SIZE.y][MAP_Z_LEN] = {};

    void init(Vec2i index) {
        this->index = index;
        for (int z = 0; z < MAP_Z_LEN; z++) {
            for (int y = 0; y < SIZE.y; y++) {
                for (int x = 0; x < SIZE.x; x++) {
                    if (z < SEA_LEVEL - 4)
                        data[x][y][z] = Cell::block(Material::stone);
                    else if (z < SEA_LEVEL)
                        data[x][y][z] = Cell::block(Material::dirt);
                    else if (z == SEA_LEVEL)
                        data[x][y][z] = Cell::surface(Material::grass);
                }
            }
        }
    }
};

// Structure
static struct Structure {
    static constexpr int MAX_SIZE = 16;
    Vec3i pos;
    Cell data[MAX_SIZE][MAX_SIZE][MAX_SIZE];
} structures[2] = {};

static void initStructure() {
    // road
    structures[0].pos = {0, 6, 1};
    for (int x = 0; x < Structure::MAX_SIZE; x++) {
        for (int y = 0; y < 4; y++) {
            structures[0].data[x][y][0] = Cell::surface(Material::stone);
        }
    }
    structures[0].data[8][8][0] = Cell::slope(Material::stone, Rotation::sn);
    structures[0].data[10][8][0] = Cell::slope(Material::stone, Rotation::we);
    structures[0].data[12][8][0] = Cell::slope(Material::stone, Rotation::ns);
    structures[0].data[14][8][0] = Cell::slope(Material::stone, Rotation::ew);

    // house
    structures[1].pos = {20, 5, 1};
    for (int x = 1; x < 9; x++) {
        const int y = x - 1;
        structures[1].data[0][y][1] = Cell::slope(Material::wood, Rotation::we);
        structures[1].data[9][y][1] = Cell::slope(Material::wood, Rotation::ew);
        for (int y = 0; y < 8; y++) {
            const bool inside = x > 1 and x < 8 and y > 0 and y < 7;
            if (inside) {
                structures[1].data[x][y][0] = Cell::surface(Material::wood, Material::wood);
                structures[1].data[x][y][1] = Cell::surface(Material::wood, Material::wood);
            } else {
                structures[1].data[x][y][0] = Cell::block(Material::wall);
                structures[1].data[x][y][1] = Cell::block(Material::wood);
            }
        }
    }
}

// Locations
static struct Location {
    const char *name;
    Vec2i _chunk_ids[1];
    Fixed<Vec2i> chunk_ids;

    void init(const char *name) {
        this->name = name;
        chunk_ids.init({ARRAY_LEN(_chunk_ids), _chunk_ids});
    }
} locations[2] = {};

static void initLocations() {
    locations[0].init("Home");
    locations[0].chunk_ids.append({0, 0});
    locations[1].init("Town");
    locations[1].chunk_ids.append({1, 0});
}

struct Map {
    Fixed<Chunk> chunks;
    bool hide_above_level = false;
    Vec2i center;
    int range = 1;

    static Vec2i indexFromPos(Vec3 pos) {
        return {int(std::floor(pos.x / Chunk::SIZE.x)), int(std::floor(pos.y / Chunk::SIZE.y))};
    }

    static Vec2i indexFromPos(Vec3i pos) { return indexFromPos(Vec3(pos)); }

    static Vec3i posFromIndex(Vec2i index) { return {index * Chunk::SIZE, 0}; }

    Cell *getCell(Vec3i v, Vec2i chunk_index) {
        Vec3i world = posFromIndex(chunk_index) + v;
        Vec2i index = indexFromPos(world);
        Vec3i local = {world.x - index.x * Chunk::SIZE.x, world.y - index.y * Chunk::SIZE.y,
                       world.z};
        for (auto &chunk : chunks) {
            if (chunk.index == index) {
                if (local.z < 0 or local.z >= MAP_Z_LEN) return 0;
                return &chunk.data[local.x][local.y][local.z];
            }
        }
        return 0;
    }

    bool isVisible(Vec3i v, Vec2i chunk_index, Face face) {
        if (v.z < 0 or v.z >= MAP_Z_LEN) return true;
        Cell *cell = getCell(v, chunk_index);
        if (!cell) return true;
        return !cell->obscuresFace(face);
    }

    void updateChunksAndApplyStructures(Vec2i new_center) {
        auto scope = ScopeArena(&arena);
        // calc list of chunks that needed for current player position
        center = new_center;
        size_t side = 2 * range + 1;
        auto indices = scope.tmp.alloc<Vec2i>(side * side);
        for (int y = center.y - range, i = 0; y <= center.y + range; y++) {
            for (int x = center.x - range; x <= center.x + range; x++, i++) {
                indices[i] = {x, y};
            }
        }
        // remove old chunk
        if (chunks.len > 0) {
            for (int i = chunks.len - 1; i >= 0; i--) {
                bool need = false;
                for (auto &index : indices) {
                    if (index == chunks[i].index) {
                        need = true;
                        break;
                    }
                }
                if (!need) chunks.swapRemove(i);
            }
        }
        // add new chunk
        for (auto &index : indices) {
            bool add = true;
            for (auto &chunk : chunks) {
                if (chunk.index == index) add = false;
            }
            if (add) {
                auto chunk = chunks.append({});
                chunk->init(index);
                // apply struct
                for (size_t i = 0; i < ARRAY_LEN(structures); i++) {
                    auto &s = structures[i];
                    Vec3i origin = s.pos;
                    for (int z = 0; z < Structure::MAX_SIZE; z++) {
                        for (int y = 0; y < Structure::MAX_SIZE; y++) {
                            for (int x = 0; x < Structure::MAX_SIZE; x++) {
                                const Cell &cell = s.data[x][y][z];
                                if (cell.isEmpty()) continue;
                                Vec3i world = origin + Vec3i(x, y, z);
                                Vec2i target_index = indexFromPos(world);
                                if (target_index != chunk->index) continue;
                                Vec3i local = world - posFromIndex(chunk->index);
                                if (local.x < 0 or local.x >= Chunk::SIZE.x or local.y < 0 or
                                    local.y >= Chunk::SIZE.y or local.z < 0 or
                                    local.z >= MAP_Z_LEN)
                                    continue;
                                chunk->data[local.x][local.y][local.z] = cell;
                            }
                        }
                    }
                }
            }
        }
    }

    void init(Engine *engine, Vec3 player_pos) {
        initMaterialSprite(engine);
        initStructure();
        initLocations();

        size_t side = 2 * range + 1;
        size_t len = side * side; // 1 9 25 49
        chunks.init(arena.alloc<Chunk>(len));
        updateChunksAndApplyStructures(indexFromPos(player_pos));
    }

    void update(Engine *engine, WorldInstances *instances, Vec3 player_pos) {
        static constexpr Color tint[] = {0xDFDFDFFF, WHITE, 0xDFDFDFFF, 0xBFBFBFFF, WHITE};
        static constexpr Vec3i face_offsets[] = {
            {0, 1, 0},  // Y+
            {1, 0, 0},  // X+
            {0, -1, 0}, // Y-
            {-1, 0, 0}, // X-
            {0, 0, 1},  // Z+
            {0, 0, -1}, // Z-
        };
        static constexpr Vec2 size = {1, 1};

        Vec2i new_center = indexFromPos(player_pos);
        if (new_center != center) updateChunksAndApplyStructures(new_center);
        for (auto &chunk : chunks) {
            for (int z = 0; z < MAP_Z_LEN; z++) {
                for (int y = 0; y < Chunk::SIZE.y; y++) {
                    for (int x = 0; x < Chunk::SIZE.x; x++) {
                        bool is_current_level = z == int(player_pos.z);
                        if (hide_above_level and z > player_pos.z) continue;
                        Cell cell = chunk.data[x][y][z];
                        if (cell.isEmpty()) continue;
                        Vec3i cell_pos = {x, y, z};
                        Vec3 pos = chunk.index * Chunk::SIZE + cell_pos + Vec3(0.5F, 0.5F, 0);
                        switch (cell.type) {
                        case Cell::Type::block: {
                            const auto sprite = cell.getBlockSprite();
                            // don't draw floor = omit face_offsets[5]
                            constexpr size_t BLOCK_FACE_COUNT = 5;
                            for (size_t i = 0; i < BLOCK_FACE_COUNT; i++) {
                                const Face face = Face(i);
                                const bool force_visible =
                                    hide_above_level and face == Face::z_pos and is_current_level;
                                if (force_visible or
                                    isVisible(cell_pos + face_offsets[i], chunk.index, face)) {
                                    instances->squares.append({pos + face_offsets[i] * 0.5F, size,
                                                               sprite, tint[i], face, 0});
                                }
                            }
                        } break;
                        case Cell::Type::slope: {
                            const auto sprite = cell.getSlopeSprite();
                            int r = int(cell.rotation);
                            if (isVisible(cell_pos + face_offsets[r], chunk.index, Face(r)))
                                instances->squares.append({pos + face_offsets[r] * 0.5F, size,
                                                           sprite, tint[r], Face(r), 0});
                            instances->squares.append({pos, size, sprite, tint[(r + 2) % 4],
                                                       Face::slope, -r * M_PI_2f});
                            for (int i = 0; i < 2; i++) {
                                // dark magic - sn -> 1 3, we -> 2 0, ns -> 3 1, ew -> 0 2
                                int index = 2 * !((r >> 1) ^ ((r & 1) == i)) + !(r & 1);
                                instances->triangles.append({pos + face_offsets[index] * 0.5F,
                                                             size, sprite, tint[index],
                                                             Face(index), i * M_PIf});
                            }
                        } break;
                        case Cell::Type::surface: {
                            bool should_render_ceiling =
                                cell.hasCeiling() and !(hide_above_level and is_current_level);
                            if (should_render_ceiling and
                                isVisible({x, y, z + 1}, chunk.index, Face::z_pos)) {
                                instances->squares.append(
                                    {pos + face_offsets[int(Face::z_pos)] * 0.5F, size,
                                     cell.getCeilingSprite(), WHITE, Face::z_pos, 0});
                            }
                            if (cell.hasFloor())
                                // Face::z_pos is intentional, because from a standpoint of light
                                // floor look upward
                                instances->squares.append(
                                    {pos + face_offsets[int(Face::z_neg)] * 0.5F, size,
                                     cell.getFloorSprite(), WHITE, Face::z_pos, 0});
                        } break;
                        }
                    }
                }
            }
        }
    }
};
