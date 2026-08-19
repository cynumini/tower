#include "engine.hpp"

// struct Object {
//     Vector2 position;
// };

// FixedArray<Object, 2> objects;

struct Atlas {
    Texture::Id texture;

    Atlas(Allocator &gpa, Renderer &renderer, String csv_name, String image_name) {
        texture = renderer.loadTexture(image_name);

        auto csv = CSV(gpa, csv_name);
        while (true) {
            auto line = csv.readline(gpa);
            if (!line.has_value) {
                break;
            }
            for (auto cell : line.payload) {
                printf("%.*s,", int(cell.len), cell.c_str);
            }
            printf("\n");
            line.payload.deinit(gpa);
        }
        csv.deinit(gpa);
    }
};

GameResult gameInit(Allocator &gpa, [[maybe_unused]] Engine &engine) {
    Atlas atlas(gpa, *engine.renderer, "world.csv", "world.png");

    String a("Good?");
    return GameResult::success;
}
GameResult gameUpdate(Engine &engine) {
    using enum GameResult;

    return ongoing;
}
void gameDraw(Renderer &renderer) {};
void gameQuit() {}
