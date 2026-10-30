#pragma once

#include "map.hpp"
#include "engine.hpp"

#include <glm/gtx/rotate_vector.hpp>

const std::unordered_map<std::string_view, Cell::Type> Cell::types = {
    {"block", Cell::Type::block},
    {"slope", Cell::Type::slope},
    {"surface", Cell::Type::surface},
    {"frame", Cell::Type::frame},
};

const std::unordered_map<std::string_view, Object::Type> Object::types = {
    {"door", Object::Type::door},
    {"wall", Object::Type::wall},
    {"ore", Object::Type::ore},
    {"furnace", Object::Type::furnace},
};

// Locations
static struct Location {
    const char *name;
    std::vector<glm::ivec2> chunk_ids;

    void init(const char *name) { this->name = name; }
} locations[2] = {};

static void initLocations() {
    locations[0].init("Home");
    locations[0].chunk_ids.emplace_back(0, 0);
    locations[1].init("Town");
    locations[1].chunk_ids.emplace_back(1, 0);
}

glm::ivec2 Map::indexFromPos(glm::vec3 pos) {
    return {int(std::floor(pos.x / Chunk::SIZE.x)), int(std::floor(pos.y / Chunk::SIZE.y))};
}

glm::ivec3 Map::posFromIndex(glm::ivec2 index) { return glm::ivec3(index * Chunk::SIZE, 0); }

Cell *Map::getCellRelative(glm::ivec3 v, glm::ivec2 chunk_index) {
    glm::ivec3 world = posFromIndex(chunk_index) + v;
    glm::ivec2 index = indexFromPos(glm::vec3(world));
    glm::ivec3 local = {world.x - index.x * Chunk::SIZE.x, world.y - index.y * Chunk::SIZE.y,
                        world.z};
    if (local.z < 0 or local.z >= MAP_Z_LEN) return 0;
    for (auto &chunk : chunks) {
        if (chunk.index == index) return &chunk.data[local.x][local.y][local.z];
    }
    return 0;
}

Cell *Map::getCell(glm::vec3 pos) {
    glm::ivec3 world = {int(std::floor(pos.x)), int(std::floor(pos.y)), int(std::floor(pos.z))};
    glm::ivec2 index = indexFromPos(glm::vec3(world));
    glm::ivec3 local = {world.x - index.x * Chunk::SIZE.x, world.y - index.y * Chunk::SIZE.y,
                        world.z};
    if (local.z < 0 or local.z >= MAP_Z_LEN) return 0;
    for (auto &chunk : chunks) {
        if (chunk.index == index) return &chunk.data[local.x][local.y][local.z];
    }
    return 0;
}

bool Map::isVisible(glm::ivec3 v, glm::ivec2 chunk_index, Face face) {
    if (v.z < 0 or v.z >= MAP_Z_LEN) return true;
    Cell *cell = getCellRelative(v, chunk_index);
    if (!cell) return true;
    return !cell->obscuresFace(face);
}

void Map::updateChunksAndApplyStructures(glm::ivec2 new_center) {
    // calc list of chunks that needed for current player position
    center = new_center;
    size_t side = 2 * range + 1;
    auto indices = std::vector<glm::ivec2>(side * side);
    for (int y = center.y - range, i = 0; y <= center.y + range; y++) {
        for (int x = center.x - range; x <= center.x + range; x++, i++) {
            indices[i] = {x, y};
        }
    }
    // remove old chunk
    if (chunks.size() > 0) {
        for (int i = chunks.size() - 1; i >= 0; i--) {
            bool need = false;
            for (auto &index : indices) {
                if (index == chunks[i].index) {
                    need = true;
                    break;
                }
            }
            if (!need) {
                std::swap(chunks[i], chunks.back());
                chunks.pop_back();
            }
        }
    }
    // add new chunk
    for (auto &index : indices) {
        bool add = true;
        for (auto &chunk : chunks) {
            if (chunk.index == index) add = false;
        }
        if (add) {
            auto &chunk = chunks.emplace_back(Chunk{});
            chunk.init(material_ids, index);
            // apply struct
            for (size_t i = 0; i < structures.size(); i++) {
                auto &s = structures[i];
                glm::ivec3 origin = glm::ivec3(s.pos.x, s.pos.y, s.pos.z);
                for (auto &object : s.objects) {
                    glm::ivec3 world = origin + object.pos;
                    glm::ivec2 target_index = indexFromPos(glm::vec3(world));
                    if (target_index != chunk.index) continue;
                    glm::ivec3 local = world - posFromIndex(chunk.index);
                    Object o = object;
                    o.pos = local;
                    chunk.objects.emplace_back(o);
                }
                for (int z = 0; z < Structure::MAX_SIZE; z++) {
                    for (int y = 0; y < Structure::MAX_SIZE; y++) {
                        for (int x = 0; x < Structure::MAX_SIZE; x++) {
                            const Cell &cell = s.data[x][y][z];
                            if (cell.isEmpty()) continue;
                            glm::ivec3 world = origin + glm::ivec3(x, y, z);
                            glm::ivec2 target_index = indexFromPos(glm::vec3(world));
                            if (target_index != chunk.index) continue;
                            glm::ivec3 local = world - posFromIndex(chunk.index);
                            if (local.x < 0 or local.x >= Chunk::SIZE.x or local.y < 0 or
                                local.y >= Chunk::SIZE.y or local.z < 0 or local.z >= MAP_Z_LEN)
                                continue;
                            chunk.data[local.x][local.y][local.z] = cell;
                        }
                    }
                }
            }
        }
    }
}

Map::Ray Map::screenToRay(glm::vec2 logical) {
    glm::vec2 ndc = {
        float(engine.mouse.x) / float(engine.screen.x) * 2.F - 1.F,
        1.F - float(engine.mouse.y) / float(engine.screen.y) * 2.F,
    };

    glm::mat4 inv_view_projection = glm::inverse(world.ubo.projection * world.ubo.view);

    glm::vec4 near_clip = {ndc.x, ndc.y, 0.F, 1.F};
    glm::vec4 far_clip = {ndc.x, ndc.y, 1.F, 1.F};

    glm::vec4 near_world = inv_view_projection * near_clip;
    glm::vec4 far_world = inv_view_projection * far_clip;

    near_world /= near_world.w;
    far_world /= far_world.w;

    Ray ray = {
        glm::vec3(near_world),
        glm::normalize(far_world - near_world),
    };

    return ray;
}

// glm::vec3 intersectGround(Ray ray, float z) {
//     if (std::abs(ray.direction.z) < 1e-6F) return {};
//     float t = (z - ray.origin.z) / ray.direction.z;
//     if (t < 0.F) return {};
//     return ray.origin + ray.direction * t;
// }

void Map::update(WorldInstances *instances, glm::vec3 player_pos, glm::vec2 logical) {
    static constexpr Color tint[] = {toColor(0xDFDFDFFF), WHITE, toColor(0xDFDFDFFF),
                                     toColor(0xBFBFBFFF), WHITE};
    static constexpr glm::ivec3 face_offsets[] = {
        {0, 1, 0},  // Y+
        {1, 0, 0},  // X+
        {0, -1, 0}, // Y-
        {-1, 0, 0}, // X-
        {0, 0, 1},  // Z+
        {0, 0, -1}, // Z-
    };
    static constexpr glm::vec2 size = {1, 1};

    glm::ivec2 new_center = indexFromPos(player_pos);
    if (new_center != center) updateChunksAndApplyStructures(new_center);
    for (auto &chunk : chunks) {
        for (int z = 0; z < MAP_Z_LEN; z++) {
            for (int y = 0; y < Chunk::SIZE.y; y++) {
                for (int x = 0; x < Chunk::SIZE.x; x++) {
                    bool is_current_level = z == int(player_pos.z);
                    if (hide_above_level and z > player_pos.z) continue;
                    Cell cell = chunk.data[x][y][z];
                    if (cell.isEmpty()) continue;
                    glm::ivec3 cell_pos = {x, y, z};
                    glm::vec3 pos = glm::vec3(chunk.index * Chunk::SIZE, 0) +
                                    glm::vec3(cell_pos) + glm::vec3(0.5F, 0.5F, 0.5F);
                    switch (cell.type) {
                    case Cell::Type::block: {
                        const auto sprite = cell.getBlockSprite(materials);
                        // don't draw floor = omit face_offsets[5]
                        constexpr size_t BLOCK_FACE_COUNT = 5;
                        for (size_t i = 0; i < BLOCK_FACE_COUNT; i++) {
                            const Face face = Face(i);
                            const bool force_visible =
                                hide_above_level and face == Face::z_pos and is_current_level;
                            if (force_visible or
                                isVisible(cell_pos + face_offsets[i], chunk.index, face)) {
                                append(instances->squares,
                                       pos + glm::vec3(face_offsets[i]) * 0.5F, size, sprite,
                                       tint[i], face, 0);
                            }
                        }
                    } break;
                    case Cell::Type::slope: {
                        const auto sprite = cell.getSlopeSprite(materials);
                        int r = int(cell.rotation);
                        if (isVisible(cell_pos + face_offsets[r], chunk.index, Face(r)))
                            append(instances->squares, pos + glm::vec3(face_offsets[r]) * 0.5F,
                                   size, sprite, tint[r], Face(r), 0);
                        append(instances->squares, pos, size, sprite, tint[(r + 2) % 4],
                               Face::slope, -r * M_PI_2f);
                        for (int i = 0; i < 2; i++) {
                            // dark magic - sn -> 1 3, we -> 2 0, ns -> 3 1, ew -> 0 2
                            int index = 2 * !((r >> 1) ^ ((r & 1) == i)) + !(r & 1);
                            if (isVisible(glm::ivec3{x, y, z} + face_offsets[index], chunk.index,
                                          Face(index)))
                                append(instances->triangles,
                                       pos + glm::vec3(face_offsets[index]) * 0.5F, size, sprite,
                                       tint[index], Face(index), i * M_PIf);
                        }
                    } break;
                    case Cell::Type::surface: {
                        bool should_render_ceiling =
                            cell.hasCeiling() and !(hide_above_level and is_current_level);
                        if (should_render_ceiling and
                            isVisible({x, y, z + 1}, chunk.index, Face::z_pos)) {
                            append(instances->squares,
                                   pos + glm::vec3(face_offsets[int(Face::z_pos)]) * 0.5F, size,
                                   cell.getCeilingSprite(materials), WHITE, Face::z_pos, 0);
                        }
                        if (cell.hasFloor())
                            // Face::z_pos is intentional, because from a standpoint of light
                            // floor look upward
                            append(instances->squares,
                                   pos + glm::vec3(face_offsets[int(Face::z_neg)]) * 0.5F, size,
                                   cell.getFloorSprite(materials), WHITE, Face::z_pos, 0);
                    } break;
                    case Cell::Type::frame: {
                        const auto sprite = cell.getFrameSprite(materials);
                        int face = int(cell.rotation);
                        int r_t = int(Face::z_pos);
                        // top
                        if (isVisible(glm::ivec3(pos + glm::vec3(face_offsets[r_t])), chunk.index,
                                      Face(r_t)))
                            append(instances->squares, pos + glm::vec3(face_offsets[r_t]) * 0.5F,
                                   size, sprite, tint[r_t], Face(r_t), 0);
                        // sides
                        for (int i = 0; i < 2; i++) {
                            auto r = (face + 1 + i * 2) % 4;
                            if (isVisible(glm::ivec3(pos + glm::vec3(face_offsets[r])),
                                          chunk.index, Face(r)))
                                append(instances->squares,
                                       pos + glm::vec3(face_offsets[r]) * 0.5F, size, sprite,
                                       tint[r], Face(r), 0);
                        }
                        // face & back
                        {
                            // left & right
                            int faces[] = {face, (face + 2) % 4};
                            for (int i = 0; i < 2; i++) {
                                const auto s = toFRect(glm::vec2(sprite.x, sprite.y) +
                                                           glm::vec2(30.0F * i, 0.0F),
                                                       glm::vec2(2.0F, sprite.h));
                                glm::vec3 offset =
                                    glm::vec3(15 * PIXEL - (30 * PIXEL * i), 0.0F, 0.0F);
                                if (faces[0] & 1) offset = glm::rotateZ(offset, -M_PI_2f);
                                if (faces[0] & 2) offset = -offset;
                                for (int j = 0; j < 2; j++) {
                                    auto sign = j == 0 ? 1 : -1;
                                    append(instances->squares,
                                           pos + glm::vec3(face_offsets[faces[j]]) * 0.5F +
                                               offset * sign,
                                           glm::vec2(2.0F * PIXEL, 1.0F), s, tint[faces[j]],
                                           Face(faces[j]), 0);
                                }
                            }
                            // top & bottom
                            for (int i = 0; i < 2; i++) {
                                const auto s = toFRect(glm::vec2(sprite.x, sprite.y) +
                                                           glm::vec2(2.F, 30.0F * i),
                                                       {sprite.w - 4.F, 2.F});
                                glm::vec3 offset =
                                    glm::vec3(0, 0, 15.F * PIXEL - (30.F * PIXEL * i));
                                if (faces[0] & 1) offset = rotateZ(offset, -M_PI_2f);
                                for (int j = 0; j < 2; j++) {
                                    append(instances->squares,
                                           pos + glm::vec3(face_offsets[faces[j]]) * 0.5F +
                                               offset,
                                           glm::vec2{28.F * PIXEL, 2.F * PIXEL}, s,
                                           tint[faces[j]], Face(faces[j]), 0);
                                }
                            }
                            // inner left & right parts
                            for (int i = 0; i < 2; i++) {
                                auto f = (face + 1 + (2 * i)) % 4;
                                append(instances->squares,
                                       pos - glm::vec3(face_offsets[f]) * (14 * PIXEL),
                                       glm::vec2{1.F, 28.F * PIXEL}, sprite,
                                       face & 1 ? tint[0] : tint[3], Face(f), 0);
                            }
                            // inner botom
                            auto f = int(Face::z_pos);
                            append(instances->squares,
                                   pos - glm::vec3(face_offsets[f]) * (14 * PIXEL),
                                   glm::vec2{1.F, 1.F}, sprite, face & 1 ? WHITE : tint[3],
                                   Face(f), 0);
                        }
                    } break;
                    }
                }
            }
        }
        for (auto object : chunk.objects) {
            glm::vec3 pos = glm::vec3(glm::ivec3(chunk.index * Chunk::SIZE, 0) + object.pos) +
                            glm::vec3(0.5F, 0.5F, 0.5F);
            switch (object.type) {
            case Object::Type::door: {
                auto r = int(object.rotation);

                auto sprite = object.getSprite(materials);
                SDL_FRect face_sprite =
                    toFRect(glm::vec2{sprite.x, sprite.y} + glm::vec2{0, 4}, {32, 32});
                // front
                if (isVisible(object.pos + face_offsets[r], chunk.index, Face(r)))
                    append(instances->squares, pos + glm::vec3(face_offsets[r]) * 0.5, size,
                           face_sprite, tint[r], Face(r), 0);
                // back
                auto r_b = (r + 2) % 4;
                append(instances->squares, pos + glm::vec3(face_offsets[r]) * (0.5 - 4 * PIXEL),
                       size, face_sprite, tint[r_b], Face(r_b), 0);
                // top
                const bool force_visible = hide_above_level and object.pos.z == int(player_pos.z);
                auto top_sprite = toFRect(glm::vec2(sprite.x, sprite.y), {32, 4});
                if (force_visible or
                    isVisible(glm::ivec3(pos + glm::vec3(face_offsets[int(Face::z_pos)])),
                              chunk.index, Face::z_pos))
                    append(instances->squares,
                           pos + glm::vec3(face_offsets[r]) * (0.5 - 2 * PIXEL) +
                               glm::vec3(face_offsets[int(Face::z_pos)]) * 0.5,
                           glm::vec2(1.0F, PIXEL * 4.0F), top_sprite, tint[int(Face::z_pos)],
                           Face::z_pos, M_PIf + (M_PI_2f * r));
                // right
                const auto side_sprite =
                    toFRect(glm::vec2(sprite.x, sprite.y) + glm::vec2(32, 4), {4, 32});
                for (int i = 0; i < 2; i++) {
                    auto r_s = (r + 1 + (i * 2)) % 4;
                    if (isVisible(glm::ivec3(pos + glm::vec3(face_offsets[r_s])), chunk.index,
                                  Face(r_s)))
                        append(instances->squares,
                               pos + glm::vec3(face_offsets[r_s]) * (0.5) +
                                   glm::vec3(face_offsets[r]) * (0.5 - 2 * PIXEL),
                               glm::vec2(PIXEL * 4.0F, 1.0F), side_sprite, tint[r_s], Face(r_s),
                               0);
                }
            } break;
            case Object::Type::wall: {
                auto r = int(object.rotation);
                auto sprite = object.getSprite(materials);
                // front
                if (isVisible(object.pos + face_offsets[r], chunk.index, Face(r)))
                    append(instances->squares, pos + glm::vec3(face_offsets[r]) * 0.5, size,
                           sprite, tint[r], Face(r), 0);
            } break;
            case Object::Type::furnace:
            case Object::Type::ore: {
                auto sprite = object.getSprite(materials);
                append(instances->squares, pos, glm::vec2{1, 1}, sprite, WHITE, Face::billboard,
                       0);

            } break;
            }
        }
    }

    if (selection) {
        Ray ray = screenToRay(logical);
        glm::vec3 goal = floor(ray.origin);
        glm::vec3 voxel = goal;
        goal.x += ray.direction.x > 0 ? 1 : 0; // why not -1? Because floor imply "-"
        goal.y += ray.direction.y > 0 ? 1 : 0;
        goal.z += ray.direction.z > 0 ? 1 : 0;
        bool found = false;
        bool floor = false;
        bool from_top = false;
        while (goal.z >= 0 and length(goal - ray.origin) < 1000) {
            Cell *cell = getCell(voxel);
            Cell *above = getCell(voxel + glm::vec3(0, 0, 1));

            bool floor_check =
                from_top and above != nullptr and above->type == Cell::Type::surface and
                (!above->hasCeiling() or (hide_above_level and (voxel.z + 1) == player_pos.z)) and
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
            auto t = (goal - ray.origin);
            t.x = ray.direction.x == 0 ? inf : t.x / ray.direction.x;
            t.y = ray.direction.y == 0 ? inf : t.y / ray.direction.y;
            t.z = ray.direction.z == 0 ? inf : t.z / ray.direction.z;
            glm::vec3 step = {};
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
            append(instances->squares, selection_pos + glm::vec3(0.5, 0.5, 1), glm::vec2{1, 1},
                   selection_sprite, WHITE, Face::z_pos, 0);
            if (floor) selection_pos.z += 1;
        }
    }
}

std::vector<SDL_FRect> Map::getCollision(glm::vec3 pos) {
    std::vector<SDL_FRect> result = {};
    result.reserve(256);
    for (auto &chunk : chunks) {
        for (int y = 0; y < Chunk::SIZE.y; y++) {
            for (int x = 0; x < Chunk::SIZE.x; x++) {
                auto z = int(std::floor(pos.z));
                auto cell_pos = glm::ivec3(x, y, z);
                auto cell = getCellRelative(cell_pos, chunk.index);
                if (!cell->isSolid()) continue;
                pos = glm::vec3(posFromIndex(chunk.index)) + glm::vec3(cell_pos);
                result.emplace_back(toFRect(glm::vec2(pos), glm::vec2{1, 1}));
            }
        }
        for (auto &object : chunk.objects) {
            if (object.type == Object::Type::ore) {
                pos = glm::vec3(posFromIndex(chunk.index) + object.pos);
                result.emplace_back(toFRect(glm::vec2(pos), {0.25F, 0.25F}));
            } else if (object.type == Object::Type::furnace) {
                pos = glm::vec3(posFromIndex(chunk.index) + object.pos);
                result.emplace_back(toFRect(glm::vec2(pos), {1.0F, 1.0F}));
            }
        }
    }
    return result;
}

Cell *Map::getSelected() { return getCell(selection_pos); }

Object *Map::getObject(glm::ivec3 pos) {
    for (auto &chunk : chunks) {
        auto index = indexFromPos(glm::vec3(pos));
        auto local =
            glm::ivec3(pos.x - index.x * Chunk::SIZE.x, pos.y - index.y * Chunk::SIZE.y, pos.z);
        if (index != chunk.index) continue;
        for (auto &object : chunk.objects) {
            if (object.pos == local) return &object;
        }
    }
    return 0;
}

Object *Map::getSelectedObject() { return getObject(glm::ivec3(selection_pos)); }

void Map::deleteObject(glm::vec3 pos) {
    int del = -1;
    for (auto &chunk : chunks) {
        auto index = indexFromPos(pos);
        if (index != chunk.index) continue;
        auto local =
            glm::ivec3(pos.x - index.x * Chunk::SIZE.x, pos.y - index.y * Chunk::SIZE.y, pos.z);
        for (size_t i = 0; i < chunk.objects.size(); i++) {
            if (chunk.objects[i].pos == local) del = i;
        }
        chunk.objects.erase(chunk.objects.begin() + del);
    }
}

void Map::deleteFloor() {
    auto cell = getSelected();
    if (cell->type == Cell::Type::surface and cell->hasFloor()) {
        cell->setFloor();
    }
}

void registerMaterial(Map &self, std::string_view name) {
    if (name == "air") {
        self.materials.emplace_back(std::string(name), SDL_FRect{});
    } else {
        self.materials.emplace_back(std::string(name), engine.sprites.at(std::string(name)));
    }
    self.material_ids.emplace(name, self.materials.size() - 1);
}

void initMap(Map &self, glm::vec3 player_pos) {
    registerMaterial(self, "air");
    registerMaterial(self, "deep");
    registerMaterial(self, "stone");
    registerMaterial(self, "dirt");
    registerMaterial(self, "grass");
    registerMaterial(self, "wood");
    registerMaterial(self, "wall");
    registerMaterial(self, "water");
    registerMaterial(self, "door");
    registerMaterial(self, "window");
    registerMaterial(self, "iron_ore");
    registerMaterial(self, "furnace");
    structureInit(self.structures, self.material_ids);
    initLocations();

    size_t side = 2 * self.range + 1;
    size_t len = side * side; // 1 9 25 49
    self.chunks.reserve(len);
    self.updateChunksAndApplyStructures(Map::indexFromPos(player_pos));

    self.selection_sprite = engine.sprites.at("selection");
}
