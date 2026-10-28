#pragma once

#include "math.hpp"
#include "skn.cpp"
#include "world_pipeline.hpp"

// Material
enum struct Material : uint8_t {
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

constexpr OldArray<const char *, 12> initBlockKey() {
    OldArray<const char *, 12> r;
    r[int(Material::air)] = "", r[int(Material::deep)] = "deep",
    r[int(Material::stone)] = "stone", r[int(Material::dirt)] = "dirt",
    r[int(Material::grass)] = "grass", r[int(Material::wood)] = "wood",
    r[int(Material::wall)] = "wall", r[int(Material::water)] = "water",
    r[int(Material::door)] = "door", r[int(Material::window)] = "window",
    r[int(Material::iron_ore)] = "iron_ore", r[int(Material::furnace)] = "furnace";
    return r;
}

static constexpr auto BlockKey = initBlockKey();

static FRect MaterialSprite[ARRAY_LEN(BlockKey)] = {};

// Cell
enum class Rotation : uint8_t { sn, we, ns, ew };

extern const std::unordered_map<std::string_view, Rotation> rotations;

struct Cell {
    enum class Type : uint8_t { block, slope, surface, frame };
    static const std::unordered_map<std::string_view, Type> types;
    Material a : 8 {};        // block, floor, slope
    Material b : 8 {};        // ceiling
    Type type : 2 {};         // 4
    Rotation rotation : 2 {}; // 4

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

    inline FRect getBlockSprite() const {
        SDL_assert(type == Type::block);
        return MaterialSprite[int(a)];
    }

    inline FRect getFrameSprite() const {
        SDL_assert(type == Type::frame);
        return MaterialSprite[int(a)];
    }

    inline FRect getSlopeSprite() const {
        SDL_assert(type == Type::slope);
        return MaterialSprite[int(a)];
    }

    inline FRect getFloorSprite() const {
        SDL_assert(type == Type::surface);
        return MaterialSprite[int(a)];
    }

    inline FRect getCeilingSprite() const {
        SDL_assert(type == Type::surface);
        return MaterialSprite[int(b)];
    }

    inline void setFloor(Material material = Material::air) {
        SDL_assert(type == Type::surface);
        a = material;
    }

    inline Material getFloor() {
        SDL_assert(type == Type::surface);
        return a;
    }
};

struct Object {
    enum class Type { door, wall, ore, furnace };
    static const std::unordered_map<std::string_view, Type> types;
    Type type;
    Vector3 pos;
    Material material;
    Rotation rotation;

    static Object door(Vector3 pos, Rotation rotation) {
        return {Type::door, pos, Material::door, rotation};
    }
    static Object wall(Vector3 pos, Material material, Rotation rotation) {
        return {Type::wall, pos, material, rotation};
    }

    static Object ore(Vector3 pos) { return {Type::ore, pos, Material::iron_ore, Rotation::sn}; }

    static Object furnace(Vector3 pos) {
        return {Type::furnace, pos, Material::furnace, Rotation::sn};
    }

    inline FRect getSprite() const { return MaterialSprite[int(material)]; }
};

constexpr int OBJECT_LEN = 32;
