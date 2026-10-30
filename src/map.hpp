#pragma once

#include "rect.hpp"

#include "world_pipeline.hpp"

#include "cell.hpp"



//     air,
//     deep,
//     stone,
//     dirt,
//     grass,
//     wood,
//     wall,
//     water,
//     door,
//     window,
//     iron_ore,
//     furnace
// }

// constexpr std::array<std::string_view, 12> initBlockKey() {
//     std::array<std::string_view, 12> r;
//     r[int(Material::air)] = "", r[int(Material::deep)] = "deep",
//     r[int(Material::stone)] = "stone", r[int(Material::dirt)] = "dirt",
//     r[int(Material::grass)] = "grass", r[int(Material::wood)] = "wood",
//     r[int(Material::wall)] = "wall", r[int(Material::water)] = "water",
//     r[int(Material::door)] = "door", r[int(Material::window)] = "window",
//     r[int(Material::iron_ore)] = "iron_ore", r[int(Material::furnace)] = "furnace";
//     return r;
// }

// static FRect MaterialSprite[SDL_arraysize(block_key)] = {};

// Cell






constexpr int OBJECT_LEN = 32;

static constexpr int MAP_Z_LEN = 3;
static constexpr int SEA_LEVEL = 1;
static constexpr int PIXELS_PER_UNIT = 32;
static constexpr float PIXEL = 1.0F / float(PIXELS_PER_UNIT);

// Chunk
struct Chunk {
    static constexpr glm::ivec2 SIZE = {16, 16};
    glm::ivec2 index;
    Cell data[SIZE.x][SIZE.y][MAP_Z_LEN] = {};
    std::vector<Object> objects;

    void init(MaterialMap map, glm::ivec2 index) {
        objects.reserve(256);
        this->index = index;
        for (int z = 0; z < MAP_Z_LEN; z++) {
            for (int y = 0; y < SIZE.y; y++) {
                for (int x = 0; x < SIZE.x; x++) {
                    if (z < SEA_LEVEL - 4)
                        data[x][y][z] = Cell::block(map.at("stone"));
                    else if (z < SEA_LEVEL)
                        data[x][y][z] = Cell::block(map.at("dirt"));
                    else if (z == SEA_LEVEL)
                        data[x][y][z] = Cell::surface(map.at("grass"));
                }
            }
        }
    }
};

#include "structure.hpp"

struct Map {
    std::vector<Chunk> chunks;
    std::vector<MaterialInfo> materials;
    MaterialMap material_ids;
    bool hide_above_level = false;
    glm::ivec2 center;
    int range = 1;

    SDL_FRect selection_sprite;
    glm::vec3 selection_pos;
    bool selection = true;

    std::vector<Structure> structures;

    static glm::ivec2 indexFromPos(glm::vec3 pos);
    static glm::ivec2 indexFromPos(glm::ivec3 pos);
    static glm::ivec3 posFromIndex(glm::ivec2 index);
    Cell *getCellRelative(glm::ivec3 v, glm::ivec2 chunk_index);
    Cell *getCell(glm::vec3 pos);
    bool isVisible(glm::ivec3 v, glm::ivec2 chunk_index, Face face);
    void updateChunksAndApplyStructures(glm::ivec2 new_center);
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };
    Ray screenToRay(glm::vec2 logical);
    void update(WorldInstances *instances, glm::vec3 player_pos, glm::vec2 logical);
    std::vector<SDL_FRect> getCollision(glm::vec3 pos);
    Cell *getSelected();
    Object *getObject(glm::ivec3 pos);
    Object *getSelectedObject();
    void deleteObject(glm::vec3 pos);
    void deleteFloor();
};
