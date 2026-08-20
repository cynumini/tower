#include "engine.hpp"

// struct Object {
//     Vector2 position;
// };

// FixedArray<Object, 2> objects;

struct Atlas {
    Texture::Id texture;
    HashMap<Rectangle, 48> data;

    Atlas(Allocator &gpa, Renderer &renderer, String csv_name, String image_name) {
        texture = renderer.loadTexture(image_name);

        auto csv = CSV(gpa, csv_name, true);
        while (true) {
            auto line = csv.readline(gpa);
            if (!line.has_value) {
                break;
            }
            assert(line.payload.len == 5);
            auto key = line.payload[0].dupe(gpa);
            bool existed = data.put(key, Rectangle{
                                                         line.payload[1].parseNumber<f32>(),
                                                         line.payload[2].parseNumber<f32>(),
                                                         line.payload[3].parseNumber<f32>(),
                                                         line.payload[4].parseNumber<f32>(),
                                                     });

            if (existed) {
                key.deinit(gpa);
            }

            for (auto cell : line.payload) {
                printf("%.*s,", int(cell.c_str.len), cell.c_str.rawptr);
            }

            printf("\n");
            line.payload.deinit(gpa);
        }
        csv.deinit(gpa);
    }

    void deinit(Allocator &gpa) {
        // I don't like this code
        for (usize i = 0; i < 48; i++) {
            if (data.used[i]) {
                data.data[i].key.deinit(gpa);
            }
        }
    }
};

GameResult gameInit(Allocator &gpa, [[maybe_unused]] Engine &engine,
                    [[maybe_unused]] Renderer &renderer) {
                        Atlas atlas(gpa, *engine.renderer, "world.csv", "world.png");
                        atlas.deinit(gpa);
    return GameResult::success;
}
GameResult gameUpdate([[maybe_unused]] Engine &engine, [[maybe_unused]] Renderer &renderer) {
    using enum GameResult;

    return ongoing;
}
void gameDraw([[maybe_unused]] Renderer &renderer) {};
void gameQuit() {}
