#pragma once

#include "world_pipeline.hpp"

// Material
using MaterialId = uint8_t;

constexpr uint8_t AIR = 0;

struct MaterialInfo {
    std::string key;
    SDL_FRect sprite;
};

using MaterialMap = std::unordered_map<std::string, MaterialId>;

// Cell
enum class Rotation : uint8_t { sn, we, ns, ew };

static const std::unordered_map<std::string_view, Rotation> rotations = {
    {"sn", Rotation::sn},
    {"we", Rotation::we},
    {"ns", Rotation::ns},
    {"ew", Rotation::ew},
};

struct Cell {
    enum class Type : uint8_t { block, slope, surface, frame };
    static const std::unordered_map<std::string_view, Type> types;
    MaterialId a{};           // block, floor, slope
    MaterialId b{};           // ceiling
    Type type : 2 {};         // 4
    Rotation rotation : 2 {}; // 4

    static constexpr Cell air(const MaterialMap &map) {
        return {.a = map.at("air"), .type = Type::block};
    }

    static constexpr Cell block(MaterialId material) {
        return {.a = material, .type = Type::block};
    }

    static constexpr Cell slope(MaterialId material, Rotation rotation) {
        return {.a = material, .type = Type::slope, .rotation = rotation};
    }

    static constexpr Cell surface(MaterialId floor, MaterialId ceiling = AIR) {
        return {.a = floor, .b = ceiling, .type = Type::surface};
    }

    static constexpr Cell frame(MaterialId floor, Rotation rotation) {
        return {.a = floor, .type = Type::frame, .rotation = rotation};
    }

    [[nodiscard]] bool obscuresFace(Face face) const {
        switch (type) {
        case Type::block:
        case Type::slope:
            return a != AIR;
        case Type::surface:
            if (face == Face::z_pos) return a != AIR;
            return false;
        case Type::frame:
            if (a == AIR) return false;
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

    bool isEmpty(bool floor_ok = false) const {
        switch (type) {
        case Type::block:
        case Type::slope:
        case Type::frame:
            return a == AIR;
        case Type::surface:
            if (!floor_ok) {
                return a == AIR and b == AIR;
            } else {
                return b == AIR;
            }
        }
    }

    inline bool isSolid() const {
        return (type == Type::block or type == Type::frame or type == Type::slope) and a != AIR;
    }

    inline bool hasFloor() const { return a != AIR; }
    inline bool hasCeiling() const { return b != AIR; }

    inline SDL_FRect getBlockSprite(const std::vector<MaterialInfo> &materials) const {
        SDL_assert(type == Type::block);
        return materials[a].sprite;
    }

    inline SDL_FRect getFrameSprite(const std::vector<MaterialInfo> &materials) const {
        SDL_assert(type == Type::frame);
        return materials[a].sprite;
    }

    inline SDL_FRect getSlopeSprite(const std::vector<MaterialInfo> &materials) const {
        SDL_assert(type == Type::slope);
        return materials[a].sprite;
    }

    inline SDL_FRect getFloorSprite(const std::vector<MaterialInfo> &materials) const {
        SDL_assert(type == Type::surface);
        return materials[a].sprite;
    }

    inline SDL_FRect getCeilingSprite(const std::vector<MaterialInfo> &materials) const {
        SDL_assert(type == Type::surface);
        return materials[b].sprite;
    }

    inline void setFloor(MaterialId material = AIR) {
        SDL_assert(type == Type::surface);
        a = material;
    }

    inline MaterialId getFloor() {
        SDL_assert(type == Type::surface);
        return a;
    }
};

struct Object {
    enum class Type { door, wall, ore, furnace };
    static const std::unordered_map<std::string_view, Type> types;
    Type type;
    glm::ivec3 pos;
    MaterialId material;
    Rotation rotation;

    static Object door(const MaterialMap &map, glm::ivec3 pos, Rotation rotation) {
        return {Type::door, pos, map.at("door"), rotation};
    }
    static Object wall(glm::ivec3 pos, MaterialId material, Rotation rotation) {
        return {Type::wall, pos, material, rotation};
    }

    static Object ore(const MaterialMap &map, glm::ivec3 pos) {
        return {Type::ore, pos, map.at("iron_ore"), Rotation::sn};
    }

    static Object furnace(const MaterialMap &map, glm::ivec3 pos) {
        return {Type::furnace, pos, map.at("furnace"), Rotation::sn};
    }

    inline SDL_FRect getSprite(const std::vector<MaterialInfo> &materials) const {
        return materials[material].sprite;
    }
};
