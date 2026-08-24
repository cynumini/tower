#include "engine.hpp"

#include "csv.hpp"

// // struct Object {
// //     Vector2 position;
// // };

// // FixedArray<Object, 2> objects;

struct Atlas {
    Texture::Id texture_id;
    HashMap<Rectangle, 48> data;

    static Atlas init(Allocator &gpa, Renderer &renderer, String csv_name, String image_name) {
        Atlas self = {};
        self.texture_id = renderer.loadTexture(image_name);

        auto csv = CSV::init(gpa, csv_name, true);
        while (true) {
            auto line = csv.readline(gpa);
            if (!line.has_value) {
                break;
            }
            assert(line.payload.len == 5);
            auto key = line.payload[0].dupe(gpa);
            bool existed = self.data.put(key, Rectangle{
                                                  line.payload[1].parseNumber<f32>(),
                                                  line.payload[2].parseNumber<f32>(),
                                                  line.payload[3].parseNumber<f32>(),
                                                  line.payload[4].parseNumber<f32>(),
                                              });
            if (existed) {
                key.deinit(gpa);
            }
            line.payload.deinit(gpa);
        }
        csv.deinit(gpa);
        return self;
    }

    void deinit(Allocator &gpa) {
        for (auto &item : data) {
            item.key.deinit(gpa);
        }
    }

    const Sprite operator[](const String &key) const { return {texture_id, data[key]}; }
};

struct Zombie {
    Sprite sprite;
    glm::vec2 position;

    void init(Atlas &atlas) {
        position.x = SCREEN.x / 2;
        position.y = SCREEN.y / 2;

        sprite = atlas["zombie"];
    }

    void draw(Renderer &renderer) {
        renderer.drawSprite(sprite, position);
    }
};

struct Player {
    enum class Direction { right, down_right, down, down_left, left, up_left, up, up_right };

    static Direction directionFromVec2(glm::vec2 v) {
        f32 angle = atan2(v.y, v.x);

        if (angle < 0) angle += glm::two_pi<f32>();

        usize index = usize(floor(angle / (glm::pi<f32>() / 4.0f) + 0.5f)) % 8;

        return static_cast<Direction>(index);
    }

    glm::vec2 position;
    Sprite sprite;
    Sprite stop_sprite;
    Sprite attack_sprite;
    u8 frame = 0;
    f32 animation_timer = 0.0f;
    f32 animation_speed;
    bool attack = false;
    f32 attack_timer = 0.0f;
    f32 attack_duration = 0.25f;
    bool flip_x = false;
    Direction direction_kind = Direction::down;
    glm::vec2 direction = {0, 1};
    Array<Rectangle, 4> down;
    Array<Rectangle, 4> up;
    Array<Rectangle, 4> right;

    void init(Atlas &atlas) {
        position.x = SCREEN.x / 2;
        position.y = SCREEN.y / 2;

        animation_speed = 0.25;

        sprite = atlas["player_down0"];
        stop_sprite = atlas["player_down0"];
        attack_sprite = atlas["slash0"];

        down[0] = atlas["player_down0"].rectangle;
        down[1] = atlas["player_down1"].rectangle;
        down[2] = atlas["player_down0"].rectangle;
        down[3] = atlas["player_down2"].rectangle;

        up[0] = atlas["player_up0"].rectangle;
        up[1] = atlas["player_up1"].rectangle;
        up[2] = atlas["player_up0"].rectangle;
        up[3] = atlas["player_up2"].rectangle;

        right[0] = atlas["player_right0"].rectangle;
        right[1] = atlas["player_right1"].rectangle;
        right[2] = atlas["player_right0"].rectangle;
        right[3] = atlas["player_right2"].rectangle;
    }

    void update(Engine &engine) {
        glm::vec2 velocity = {
            engine.isKeyDown(Key::right) - engine.isKeyDown(Key::left),
            engine.isKeyDown(Key::down) - engine.isKeyDown(Key::up),
        };

        if (engine.isKeyPressed(Key::z)) {
            attack = true;
        }
        auto length = glm::length(velocity);
        if (length > 0) {
            velocity /= length;
            direction_kind = directionFromVec2(velocity);
            direction = velocity;
            flip_x = false;
            if (velocity.y > 0) {
                stop_sprite.rectangle = down[0];
                sprite.rectangle = down[frame];
            } else if (velocity.y < 0) {
                stop_sprite.rectangle = up[0];
                sprite.rectangle = up[frame];
            } else if (velocity.x > 0) {
                stop_sprite.rectangle = right[0];
                sprite.rectangle = right[frame];
            } else {
                stop_sprite.rectangle = right[0];
                sprite.rectangle = right[frame];
                flip_x = true;
            }
        } else {
            sprite = stop_sprite;
        }
        constexpr f32 player_speed = 100;

        position += velocity * (engine.dt * player_speed);

        animation_timer += engine.dt;
        if (animation_timer >= animation_speed) {
            animation_timer = 0.0f;
            frame++;
            frame %= 4;
        }

        if (attack) {
            attack_timer += engine.dt;
            if (attack_timer > attack_duration) {
                attack = false;
                attack_timer = 0.0;
            }
        }
    }

    void draw(Renderer &renderer) {
        if (attack) {
            auto attack_position = position;
            f32 angle = glm::radians(f32(direction_kind) * 45);
            auto offset = direction;
            offset.x *= 16;
            offset.y *= 24;
            attack_position += offset;
            renderer.drawSprite(attack_sprite, attack_position, false, false, angle);
        }
        renderer.drawSprite(sprite, position, flip_x);
    }
};

Atlas atlas;
Player player;
Zombie zombie;

GameResult gameInit(Allocator &gpa, [[maybe_unused]] Engine &engine, Renderer &renderer) {
    atlas = Atlas::init(gpa, renderer, "world.csv", "world.png");
    player.init(atlas);
    zombie.init(atlas);
    return GameResult::ongoing;
}

GameResult gameUpdate([[maybe_unused]] Allocator &gpa, Engine &engine,
                      [[maybe_unused]] Renderer &renderer) {
                          player.update(engine);

    return GameResult::ongoing;
}
void gameDraw(Renderer &renderer) {
    player.draw(renderer);
    zombie.draw(renderer);
    ImGui::Text("%.1f FPS", renderer.imgui_io->Framerate);
    ImGui::Text("x: %f", player.position.x);
    ImGui::Text("y: %f", player.position.y);
    ImGui::Text("frame: %hhu", player.frame);
    ImGui::Text("direction: %d", static_cast<int>(player.direction_kind));
};
void gameQuit(Allocator &gpa) { atlas.deinit(gpa); }
