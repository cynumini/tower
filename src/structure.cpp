#include "structure.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void structureInit(std::vector<Structure> &structures, MaterialMap map) {
    // road
    auto &road = structures.emplace_back();
    road.pos = {0, 6, 1}; // NOLINT
    for (int x = 0; x < Structure::MAX_SIZE; x++) {
        for (int y = 0; y < 4; y++) {
            road.data[x][y][0] = Cell::surface(map.at("stone"));
        }
    }
    road.data[8][8][0] = Cell::slope(map.at("stone"), Rotation::sn);
    road.data[10][8][0] = Cell::slope(map.at("stone"), Rotation::we);
    road.data[12][8][0] = Cell::slope(map.at("stone"), Rotation::ns);
    road.data[14][8][0] = Cell::slope(map.at("stone"), Rotation::ew);
    road.data[0][1][0] = Cell::block(map.at("wood"));
    road.data[2][1][0] = Cell::block(map.at("wood"));
    road.data[4][1][0] = Cell::block(map.at("wood"));
    road.data[6][1][0] = Cell::block(map.at("wood"));

    // road.data[0][3][0] = Cell::frame(Material::grass, Rotation::sn);
    road.data[2][3][0] = Cell::frame(map.at("stone"), Rotation::we);
    road.data[4][3][0] = Cell::frame(map.at("wall"), Rotation::ns);
    road.data[6][3][0] = Cell::frame(map.at("wood"), Rotation::ew);

    for (int i = 0; i < 16; i++) {
        while (true) {
            int x = SDL_rand(16);
            int y = SDL_rand(16);
            glm::ivec3 pos = {x, y, 0};

            if (!road.data[x][y][0].isEmpty(true)) continue;
            bool ok = true;
            for (auto &object : road.objects) {
                if (object.pos == pos) ok = false;
            }

            if (!ok) continue;
            road.objects.emplace_back(Object::ore(map, pos));
            break;
        }
    }

    // house
    auto &house = structures.emplace_back();
    house.pos = {20, 5, 1};
    for (int x = 1; x < 10; x++) {
        const int y = x - 1;
        house.data[0][y][1] = Cell::slope(map.at("wood"), Rotation::we);
        house.data[10][y][1] = Cell::slope(map.at("wood"), Rotation::ew);
        for (int y = 0; y < 9; y++) {
            const bool inside = x > 1 and x < 9 and y > 0 and y < 8;
            if (inside) {
                house.data[x][y][0] = Cell::surface(map.at("wood"), map.at("wood"));
                house.data[x][y][1] = Cell::surface(map.at("wood"), map.at("wood"));
            } else {
                house.data[x][y][0] = Cell::block(map.at("wall"));
                house.data[x][y][1] = Cell::block(map.at("wood"));
            }
        }
    }
    house.data[3][0][0] = Cell::frame(map.at("wall"), Rotation::sn);
    house.data[5][0][0] = Cell::surface(map.at("wood"), map.at("wood"));
    house.data[7][0][0] = Cell::frame(map.at("wall"), Rotation::sn);
    house.objects.emplace_back(Object::door(map, {5, 0, 0}, Rotation::ns));
    house.objects.emplace_back(Object::wall({3, 0, 0}, map.at("window"), Rotation::ns));
    house.objects.emplace_back(Object::wall({7, 0, 0}, map.at("window"), Rotation::ns));
    house.objects.emplace_back(Object::furnace(map, {4, 7, 0}));
    house.objects.emplace_back(Object::furnace(map, {5, 7, 0}));
    house.objects.emplace_back(Object::furnace(map, {6, 7, 0}));

    for (const auto &entry : std::filesystem::directory_iterator("resources/structures")) {
        std::ifstream file(entry.path());
        json data = json::parse(file);

        auto &structure = structures.emplace_back();
        structure.pos.x = data["pos"]["x"];
        structure.pos.y = data["pos"]["y"];
        structure.pos.z = data["pos"]["z"];

        std::unordered_map<std::string_view, Cell> palette;
        for (const auto &[key, value] : data["palette"].items()) {
            // std::cout << key << "\n";
            Cell cell = {};
            // for (BlockKey[0] == 0)
            // std::cout << value["type"] << "\n";
            // std::cout << value["material"] << "\n";
            // std::cout << value["rotation"] << "\n";
        }

        for (const auto &object : data["objects"]) {
            auto type = Object::types.at(std::string(object["type"]));
            glm::ivec3 pos = {
                object["pos"]["x"],
                object["pos"]["y"],
                object["pos"]["z"],
            };
            switch (type) {
            case Object::Type::door:
                structure.objects.emplace_back(Object::door(
                    map, {pos.x, pos.y, pos.z}, rotations.at(std::string(object["rotation"]))));
                break;
            case Object::Type::wall:
            case Object::Type::ore:
            case Object::Type::furnace:
                SDL_assert(false);
                break;
            }

            // structure.objects.emplace_back();
        }

        // house.objects.emplace_back();
        //  std::cout << data << "\n";
    }
}
