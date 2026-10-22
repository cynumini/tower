#pragma once

#include "shared.cpp"

#include <inplace_vector>
#include <limits>

static constexpr int MAP_Z_LEN = 3;
static constexpr int SEA_LEVEL = 1;
static constexpr int PIXELS_PER_UNIT = 32;
static constexpr float PIXEL = 1.0F / float(PIXELS_PER_UNIT);

// Material
enum struct Material : u8 {
    air,
    deep,
    stone,
    dirt,
    grass,
    wood,
    wall,
    water,
    door,
    window,
    iron_ore,
    furnace
};

constexpr std::array<const char *, 12> initBlockKey() {
    std::array<const char *, 12> r;
    r[int(Material::air)] = "", r[int(Material::deep)] = "deep",
    r[int(Material::stone)] = "stone", r[int(Material::dirt)] = "dirt",
    r[int(Material::grass)] = "grass", r[int(Material::wood)] = "wood",
    r[int(Material::wall)] = "wall", r[int(Material::water)] = "water",
    r[int(Material::door)] = "door", r[int(Material::window)] = "window",
    r[int(Material::iron_ore)] = "iron_ore", r[int(Material::furnace)] = "furnace";
    return r;
}

static constexpr auto BlockKey = initBlockKey();

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
    enum class Type : u8 { block, slope, surface, frame };
    Material a : 8;        // block, floor, slope
    Material b : 8;        // ceiling
    Type type : 2;         // 4
    Rotation rotation : 2; // 4

    static constexpr Cell air() { return {.a = Material::air, .type = Type::block}; }

    static constexpr Cell block(Material material) {
        return {.a = material, .type = Type::block};
    }

    static constexpr Cell slope(Material material, Rotation rotation) {
        return {.a = material, .type = Type::slope, .rotation = rotation};
    }

    static constexpr Cell surface(Material floor, Material ceiling = Material::air) {
        return {.a = floor, .b = ceiling, .type = Type::surface};
    }

    static constexpr Cell frame(Material floor, Rotation rotation) {
        return {.a = floor, .type = Type::frame, .rotation = rotation};
    }

    inline bool obscuresFace(Face face) const {
        switch (type) {
        case Type::block:
        case Type::slope:
            return a != Material::air;
        case Type::surface:
            if (face == Face::z_pos) return a != Material::air;
            return false;
        case Type::frame:
            if (a == Material::air) return false;
            switch (rotation) {
            case Rotation::sn:
            case Rotation::ns:
                if (face == Face::y_pos or face == Face::y_neg) return false;
            case Rotation::we:
            case Rotation::ew:
                if (face == Face::x_pos or face == Face::x_neg) return false;
            }
            return true;
        }
    }

    inline bool isEmpty(bool floor_ok = false) const {
        switch (type) {
        case Type::block:
        case Type::slope:
        case Type::frame:
            return a == Material::air;
        case Type::surface:
            if (!floor_ok) {
                return a == Material::air and b == Material::air;
            } else {
                return b == Material::air;
            }
        }
    }

    inline bool isSolid() const {
        return (type == Type::block or type == Type::frame or type == Type::slope) and
               a != Material::air;
    }

    inline bool hasFloor() const { return a != Material::air; }
    inline bool hasCeiling() const { return b != Material::air; }

    inline Rect getBlockSprite() const {
        assert(type == Type::block);
        return MaterialSprite[int(a)];
    }

    inline Rect getFrameSprite() const {
        assert(type == Type::frame);
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

    inline void setFloor(Material material = Material::air) {
        assert(type == Type::surface);
        a = material;
    }

    inline Material getFloor() {
        assert(type == Type::surface);
        return a;
    }
};

struct Object {
    enum class Type { door, wall, ore, furnace };
    Type type;
    Vec3i pos;
    Material material;
    Rotation rotation;

    static Object door(Vec3i pos, Material material, Rotation rotation) {
        return {Type::door, pos, material, rotation};
    }
    static Object wall(Vec3i pos, Material material, Rotation rotation) {
        return {Type::wall, pos, material, rotation};
    }

    static Object ore(Vec3i pos) { return {Type::ore, pos, Material::iron_ore, Rotation::sn}; }

    static Object furnace(Vec3i pos) {
        return {Type::furnace, pos, Material::furnace, Rotation::sn};
    }

    inline Rect getSprite() const { return MaterialSprite[int(material)]; }
};

constexpr int OBJECT_LEN = 32;

// Chunk
struct Chunk {
    static constexpr Vec2i SIZE = {16, 16};
    Vec2i index;
    Cell data[SIZE.x][SIZE.y][MAP_Z_LEN] = {};
    std::inplace_vector<Object, OBJECT_LEN> objects;

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
    static constexpr int MAX_SIZE = 32;
    Vec3i pos;
    Cell data[MAX_SIZE][MAX_SIZE][MAX_SIZE];
    std::inplace_vector<Object, OBJECT_LEN> objects;
} structures[2] = {};

static void initStructure(Engine *engine) {
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
    structures[0].data[0][1][0] = Cell::block(Material::wood);
    structures[0].data[2][1][0] = Cell::block(Material::wood);
    structures[0].data[4][1][0] = Cell::block(Material::wood);
    structures[0].data[6][1][0] = Cell::block(Material::wood);

    structures[0].data[0][3][0] = Cell::frame(Material::grass, Rotation::sn);
    structures[0].data[2][3][0] = Cell::frame(Material::stone, Rotation::we);
    structures[0].data[4][3][0] = Cell::frame(Material::wall, Rotation::ns);
    structures[0].data[6][3][0] = Cell::frame(Material::wood, Rotation::ew);
    structures[0].objects.emplace_back(Object::door({0, 0, 0}, Material::door, Rotation::sn));
    structures[0].objects.emplace_back(Object::door({2, 0, 0}, Material::door, Rotation::we));
    structures[0].objects.emplace_back(Object::door({4, 0, 0}, Material::door, Rotation::ns));
    structures[0].objects.emplace_back(Object::door({6, 0, 0}, Material::door, Rotation::ew));

    for (int i = 0; i < 16; i++) {
        while (true) {
            int x = engine->rand(16);
            int y = engine->rand(16);
            Vec3i pos = {x, y, 0};

            if (!structures[0].data[x][y][0].isEmpty(true)) continue;
            bool ok = true;
            for (auto &object : structures[0].objects) {
                if (object.pos == pos) ok = false;
            }

            if (!ok) continue;
            structures[0].objects.emplace_back(Object::ore(pos));
            break;
        }
    }

    // house
    structures[1].pos = {20, 5, 1};
    for (int x = 1; x < 10; x++) {
        const int y = x - 1;
        structures[1].data[0][y][1] = Cell::slope(Material::wood, Rotation::we);
        structures[1].data[10][y][1] = Cell::slope(Material::wood, Rotation::ew);
        for (int y = 0; y < 9; y++) {
            const bool inside = x > 1 and x < 9 and y > 0 and y < 8;
            if (inside) {
                structures[1].data[x][y][0] = Cell::surface(Material::wood, Material::wood);
                structures[1].data[x][y][1] = Cell::surface(Material::wood, Material::wood);
            } else {
                structures[1].data[x][y][0] = Cell::block(Material::wall);
                structures[1].data[x][y][1] = Cell::block(Material::wood);
            }
        }
    }
    structures[1].data[3][0][0] = Cell::frame(Material::wall, Rotation::sn);
    structures[1].data[5][0][0] = Cell::surface(Material::wood, Material::wood);
    structures[1].data[7][0][0] = Cell::frame(Material::wall, Rotation::sn);
    structures[1].objects.emplace_back(Object::door({5, 0, 0}, Material::door, Rotation::ns));
    structures[1].objects.emplace_back(Object::wall({3, 0, 0}, Material::window, Rotation::ns));
    structures[1].objects.emplace_back(Object::wall({7, 0, 0}, Material::window, Rotation::ns));
    structures[1].objects.emplace_back(Object::furnace({4, 7, 0}));
    structures[1].objects.emplace_back(Object::furnace({5, 7, 0}));
    structures[1].objects.emplace_back(Object::furnace({6, 7, 0}));
}

// Locations
static struct Location {
    const char *name;
    std::inplace_vector<Vec2i, 1> chunk_ids;

    void init(const char *name) { this->name = name; }
} locations[2] = {};

static void initLocations() {
    locations[0].init("Home");
    locations[0].chunk_ids.emplace_back(0, 0);
    locations[1].init("Town");
    locations[1].chunk_ids.emplace_back(1, 0);
}

struct Map {
    Fixed<Chunk> chunks;
    bool hide_above_level = false;
    Vec2i center;
    int range = 1;

    Rect selection_sprite;
    Vec3 selection_pos;
    bool selection = true;

    static Vec2i indexFromPos(Vec3 pos) {
        return {int(std::floor(pos.x / Chunk::SIZE.x)), int(std::floor(pos.y / Chunk::SIZE.y))};
    }

    static Vec2i indexFromPos(Vec3i pos) { return indexFromPos(Vec3(pos)); }

    static Vec3i posFromIndex(Vec2i index) { return {index * Chunk::SIZE, 0}; }

    Cell *getCellRelative(Vec3i v, Vec2i chunk_index) {
        Vec3i world = posFromIndex(chunk_index) + v;
        Vec2i index = indexFromPos(world);
        Vec3i local = {world.x - index.x * Chunk::SIZE.x, world.y - index.y * Chunk::SIZE.y,
                       world.z};
        if (local.z < 0 or local.z >= MAP_Z_LEN) return 0;
        for (auto &chunk : chunks) {
            if (chunk.index == index) return &chunk.data[local.x][local.y][local.z];
        }
        return 0;
    }

    Cell *getCell(Vec3 pos) {
        Vec3i world = {int(std::floor(pos.x)), int(std::floor(pos.y)), int(std::floor(pos.z))};
        Vec2i index = indexFromPos(world);
        Vec3i local = {world.x - index.x * Chunk::SIZE.x, world.y - index.y * Chunk::SIZE.y,
                       world.z};
        if (local.z < 0 or local.z >= MAP_Z_LEN) return 0;
        for (auto &chunk : chunks) {
            if (chunk.index == index) return &chunk.data[local.x][local.y][local.z];
        }
        return 0;
    }

    bool isVisible(Vec3i v, Vec2i chunk_index, Face face) {
        if (v.z < 0 or v.z >= MAP_Z_LEN) return true;
        Cell *cell = getCellRelative(v, chunk_index);
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
                auto *chunk = chunks.append({});
                chunk->init(index);
                // apply struct
                for (size_t i = 0; i < ARRAY_LEN(structures); i++) {
                    auto &s = structures[i];
                    Vec3i origin = s.pos;
                    for (auto &object : s.objects) {
                        Vec3i world = origin + object.pos;
                        Vec2i target_index = indexFromPos(world);
                        if (target_index != chunk->index) continue;
                        Vec3i local = world - posFromIndex(chunk->index);
                        Object o = object;
                        o.pos = local;
                        chunk->objects.emplace_back(o);
                    }
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
        initStructure(engine);
        initLocations();

        size_t side = 2 * range + 1;
        size_t len = side * side; // 1 9 25 49
        chunks.init(arena.alloc<Chunk>(len));
        updateChunksAndApplyStructures(indexFromPos(player_pos));

        selection_sprite = engine->sprites.get("selection");
    }

    struct Ray {
        Vec3 origin;
        Vec3 direction;
    };

    Ray screenToRay(Engine *engine, Vec2 logical) {
        Vec3 ndc = {float(engine->mouse.x) / float(engine->screen.x) * 2.F - 1.F,
                    1.F - float(engine->mouse.y) / float(engine->screen.y) * 2.F, 0.F};
        Mat4 inv_view = Mat4::translation(engine->camera.pos) *
                        Mat4::rotationY(-deg2rad(engine->camera.roll)) *
                        Mat4::rotationZ(-deg2rad(engine->camera.yaw)) * Mat4::inverseIso45();
        float left = -logical.x / 2.0F;
        float right = logical.x / 2.0F;
        float bottom = -logical.y / 2.0F;
        float top = logical.y / 2.0F;
        float near = 100.F;
        float far = -100.F;
        Ray ray = {inv_view * Mat4::unprojectOrtho({ndc, 0}, left, right, bottom, top, near, far),
                   {}};
        Vec3 direction =
            inv_view * Mat4::unprojectOrtho({ndc, 1}, left, right, bottom, top, near, far);
        ray.direction = (direction - ray.origin).normalize();
        return ray;
    }

    // Vec3 intersectGround(Ray ray, float z) {
    //     if (std::abs(ray.direction.z) < 1e-6F) return {};
    //     float t = (z - ray.origin.z) / ray.direction.z;
    //     if (t < 0.F) return {};
    //     return ray.origin + ray.direction * t;
    // }

    void update(Engine *engine, WorldInstances *instances, Vec3 player_pos, Vec2 logical) {
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
                        Vec3 pos = chunk.index * Chunk::SIZE + cell_pos + Vec3(0.5F, 0.5F, 0.5F);
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
                                if (isVisible(Vec3i{x, y, z} + face_offsets[index], chunk.index,
                                              Face(index)))
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
                        case Cell::Type::frame: {
                            const auto sprite = cell.getFrameSprite();
                            int face = int(cell.rotation);
                            int r_t = int(Face::z_pos);
                            // top
                            if (isVisible(pos + face_offsets[r_t], chunk.index, Face(r_t)))
                                instances->squares.append({pos + face_offsets[r_t] * 0.5F, size,
                                                           sprite, tint[r_t], Face(r_t), 0});
                            // sides
                            for (int i = 0; i < 2; i++) {
                                auto r = (face + 1 + i * 2) % 4;
                                if (isVisible(pos + face_offsets[r], chunk.index, Face(r)))
                                    instances->squares.append({pos + face_offsets[r] * 0.5F, size,
                                                               sprite, tint[r], Face(r), 0});
                            }
                            // face & back
                            {
                                // left & right
                                int faces[] = {face, (face + 2) % 4};
                                for (int i = 0; i < 2; i++) {
                                    const Rect s = {sprite.position() + Vec2(30.0F * i, 0.0F),
                                                    {2.0F, sprite.h}};
                                    Vec3 offset = Vec3(15 * PIXEL - (30 * PIXEL * i), 0.0F, 0.0F);
                                    if (faces[0] & 1) offset = offset.rotateZ(-M_PI_2f);
                                    if (faces[0] & 2) offset = -offset;
                                    for (int j = 0; j < 2; j++) {
                                        auto sign = j == 0 ? 1 : -1;
                                        instances->squares.append(
                                            {pos + face_offsets[faces[j]] * 0.5F + offset * sign,
                                             {2.0F * PIXEL, 1.0F},
                                             s,
                                             tint[faces[j]],
                                             Face(faces[j]),
                                             0});
                                    }
                                }
                                // top & bottom
                                for (int i = 0; i < 2; i++) {
                                    const Rect s = {sprite.position() + Vec2(2.F, 30.0F * i),
                                                    {sprite.w - 4.F, 2.F}};
                                    Vec3 offset = Vec3(0, 0, 15.F * PIXEL - (30.F * PIXEL * i));
                                    if (faces[0] & 1) offset = offset.rotateZ(-M_PI_2f);
                                    for (int j = 0; j < 2; j++) {
                                        instances->squares.append(
                                            {pos + face_offsets[faces[j]] * 0.5F + offset,
                                             {28.F * PIXEL, 2.F * PIXEL},
                                             s,
                                             tint[faces[j]],
                                             Face(faces[j]),
                                             0});
                                    }
                                }
                                // inner left & right parts
                                for (int i = 0; i < 2; i++) {
                                    auto f = (face + 1 + (2 * i)) % 4;
                                    instances->squares.append(
                                        {pos - face_offsets[f] * (14 * PIXEL),
                                         {1.F, 28.F * PIXEL},
                                         sprite,
                                         face & 1 ? tint[0] : tint[3],
                                         Face(f),
                                         0});
                                }
                                // inner botom
                                auto f = int(Face::z_pos);
                                instances->squares.append({pos - face_offsets[f] * (14 * PIXEL),
                                                           {1.F, 1.F},
                                                           sprite,
                                                           face & 1 ? WHITE : tint[3],
                                                           Face(f),
                                                           0});
                            }
                        } break;
                        }
                    }
                }
            }
            for (auto object : chunk.objects) {
                Vec3 pos = chunk.index * Chunk::SIZE + object.pos + Vec3(0.5F, 0.5F, 0.5F);
                switch (object.type) {
                case Object::Type::door: {
                    auto r = int(object.rotation);

                    auto sprite = object.getSprite();
                    Rect face_sprite = Rect(sprite.position() + Vec2{0, 4}, {32, 32});
                    // front
                    if (isVisible(object.pos + face_offsets[r], chunk.index, Face(r)))
                        instances->squares.append({pos + face_offsets[r] * 0.5, size, face_sprite,
                                                   tint[r], Face(r), 0});
                    // back
                    auto r_b = (r + 2) % 4;
                    instances->squares.append({pos + face_offsets[r] * (0.5 - 4 * PIXEL), size,
                                               face_sprite, tint[r_b], Face(r_b), 0});
                    // top
                    const bool force_visible =
                        hide_above_level and object.pos.z == int(player_pos.z);
                    auto top_sprite = Rect(sprite.position(), {32, 4});
                    if (force_visible or
                        isVisible(pos + face_offsets[int(Face::z_pos)], chunk.index, Face::z_pos))
                        instances->squares.append({pos + face_offsets[r] * (0.5 - 2 * PIXEL) +
                                                       face_offsets[int(Face::z_pos)] * 0.5,
                                                   Vec2(1.0F, PIXEL * 4.0F), top_sprite,
                                                   tint[int(Face::z_pos)], Face::z_pos,
                                                   M_PIf + (M_PI_2f * r)});
                    // right
                    const auto side_sprite = Rect(sprite.position() + Vec2(32, 4), {4, 32});
                    for (int i = 0; i < 2; i++) {
                        auto r_s = (r + 1 + (i * 2)) % 4;
                        if (isVisible(pos + face_offsets[r_s], chunk.index, Face(r_s)))
                            instances->squares.append({pos + face_offsets[r_s] * (0.5) +
                                                           face_offsets[r] * (0.5 - 2 * PIXEL),
                                                       Vec2(PIXEL * 4.0F, 1.0F), side_sprite,
                                                       tint[r_s], Face(r_s), 0});
                    }
                } break;
                case Object::Type::wall: {
                    auto r = int(object.rotation);
                    auto sprite = object.getSprite();
                    // front
                    if (isVisible(object.pos + face_offsets[r], chunk.index, Face(r)))
                        instances->squares.append(
                            {pos + face_offsets[r] * 0.5, size, sprite, tint[r], Face(r), 0});
                } break;
                case Object::Type::furnace:
                case Object::Type::ore: {
                    auto sprite = object.getSprite();
                    instances->squares.append({pos, {1, 1}, sprite, WHITE, Face::billboard, 0});

                } break;
                }
            }
        }

        if (selection) {
            Ray ray = screenToRay(engine, logical);
            Vec3 goal = ray.origin.floor();
            Vec3 voxel = goal;
            goal.x += ray.direction.x > 0 ? 1 : 0; // why not -1? Because floor imply "-"
            goal.y += ray.direction.y > 0 ? 1 : 0;
            goal.z += ray.direction.z > 0 ? 1 : 0;
            bool found = false;
            bool floor = false;
            bool from_top = false;
            while (goal.z >= 0 and (goal - ray.origin).length() < 1000) {
                Cell *cell = getCell(voxel);
                Cell *above = getCell(voxel + Vec3(0, 0, 1));

                bool floor_check = from_top and above != nullptr and
                                   above->type == Cell::Type::surface and
                                   (!above->hasCeiling() or
                                    (hide_above_level and (voxel.z + 1) == player_pos.z)) and
                                   above->hasFloor();

                bool skip_ceiling = cell != nullptr and hide_above_level and
                                    cell->type == Cell::Type::surface and voxel.z == player_pos.z;

                if (cell != nullptr and (!cell->isEmpty() or floor_check) and
                    (cell->type != Cell::Type::surface or cell->hasCeiling()) and
                    !(hide_above_level and goal.z > player_pos.z) and !skip_ceiling) {
                    if (floor_check) floor = true;
                    found = true;
                    break;
                }
                constexpr auto inf = std::numeric_limits<float>::infinity();
                Vec3 t = (goal - ray.origin);
                t.x = ray.direction.x == 0 ? inf : t.x / ray.direction.x;
                t.y = ray.direction.y == 0 ? inf : t.y / ray.direction.y;
                t.z = ray.direction.z == 0 ? inf : t.z / ray.direction.z;
                Vec3 step = {};
                from_top = false;
                if (t.x < t.y and t.x < t.z) {
                    step.x += ray.direction.x > 0 ? 1 : -1;
                } else if (t.y < t.z) {
                    step.y += ray.direction.y > 0 ? 1 : -1;
                } else {
                    step.z += ray.direction.z > 0 ? 1 : -1;
                    if (ray.direction.z < 0) {
                        from_top = true;
                    }
                }
                goal += step;
                voxel += step;
            }
            if (found) {
                selection_pos = voxel;
                instances->squares.append({selection_pos + Vec3(0.5, 0.5, 1),
                                           {1, 1},
                                           selection_sprite,
                                           WHITE,
                                           Face::z_pos,
                                           0});
                if (floor) selection_pos.z += 1;
            }
        }
    }

    std::inplace_vector<Rect, 256> getCollision(Vec3 pos) {
        std::inplace_vector<Rect, 256> result = {};
        for (auto &chunk : chunks) {
            for (int y = 0; y < Chunk::SIZE.y; y++) {
                for (int x = 0; x < Chunk::SIZE.x; x++) {
                    auto z = int(std::floor(pos.z));
                    auto cell_pos = Vec3i(x, y, z);
                    auto cell = getCellRelative(cell_pos, chunk.index);
                    if (!cell->isSolid()) continue;
                    pos = posFromIndex(chunk.index) + cell_pos;
                    result.emplace_back(Rect(pos, {1, 1}));
                }
            }
            for (auto &object : chunk.objects) {
                if (object.type == Object::Type::ore) {
                    pos = posFromIndex(chunk.index) + object.pos;
                    result.emplace_back(Rect(pos, {0.25F, 0.25F}));
                } else if (object.type == Object::Type::furnace) {
                    pos = posFromIndex(chunk.index) + object.pos;
                    result.emplace_back(Rect(pos, {1.0F, 1.0F}));
                }
            }
        }
        return result;
    }

    Cell *getSelected() { return getCell(selection_pos); }

    Object *getObject(Vec3i pos) {
        for (auto &chunk : chunks) {
            auto index = indexFromPos(pos);
            auto local =
                Vec3i(pos.x - index.x * Chunk::SIZE.x, pos.y - index.y * Chunk::SIZE.y, pos.z);
            if (index != chunk.index) continue;
            for (auto &object : chunk.objects) {
                if (object.pos == local) return &object;
            }
        }
        return 0;
    }

    Object *getSelectedObject() { return getObject(selection_pos); }

    void deleteObject(Vec3 pos) {
        int del = -1;
        for (auto &chunk : chunks) {
            auto index = indexFromPos(pos);
            if (index != chunk.index) continue;
            auto local =
                Vec3i(pos.x - index.x * Chunk::SIZE.x, pos.y - index.y * Chunk::SIZE.y, pos.z);
            for (size_t i = 0; i < chunk.objects.size(); i++) {
                if (chunk.objects[i].pos == local) del = i;
            }
            chunk.objects.erase(chunk.objects.begin() + del);
        }
    }

    void deleteFloor() {
        auto cell = getSelected();
        if (cell->type == Cell::Type::surface and cell->hasFloor()) {
            cell->setFloor();
        }
    }
};
