#include "game.hpp"

#include "unagi.hpp"

static Arena arena;

// Timer
struct Timer {
    float elapsed;
    float duration;

    static Timer init(float duration) { return {.duration = duration}; };

    void reset() { elapsed = 0; };

    bool advanceAndCheck(float dt) {
        elapsed += dt;
        if (elapsed >= duration) {
            reset();
            return true;
        }
        return false;
    };
};

// Inventory
struct InventorySlot {
    u8 item_id;
    u8 count;
};
static InventorySlot inventory[8 * 8];
static bool invertory_visible;

// Animation
static struct Animation {
    u8 frames;
    Rect origin;
    bool flip_x;

    void init(Rect texture, u8 frames, bool flip_x = false) {
        this->flip_x = flip_x;
        this->frames = frames;
        this->origin = {
            .x = texture.x,
            .y = texture.y,
            .w = texture.w / float(frames),
            .h = texture.h,
        };
    }

    Rect get(u8 index) const {
        assert(index < frames);
        Rect result = {.x = origin.x + (origin.w * float(index)),
                       .y = origin.y,
                       .w = origin.w,
                       .h = origin.h};
        if (flip_x) result.x += result.w, result.w *= -1;
        return result;
    }
} player_down, player_right, player_left, player_up;

// Object
struct Object {
    float speed;
    float angle;
    int hp;
    vec2 direction;
    Timer timer;
    vec2 pos;
    vec2 size;
    Rect sprite;
    enum Kind : u8 { NONE, PLAYER, ENEMY, ATTACK, SPELL } kind;
    u8 frame;
    bool alive;
    bool invincible;
    Color tint;

    void takeDamage(vec2 direction, InventorySlot *inventory) {
        this->direction = direction;
        const float KNOCKBACK_SPEED = 100;
        speed = KNOCKBACK_SPEED;
        hp -= 1;
        invincible = true;
        if (hp == 0) {
            alive = false;
            inventory[0].count += 1;
        }
    }

    Rect rect() { return {pos.x, pos.y, size.x, size.y}; }
};

const u8 OBJECTS_MAX = 255;
static Object objects_raw[OBJECTS_MAX];
static Fixed<Object> objects;

// ids
static size_t player_id;
static size_t attack_id;
static size_t spell_id;

void Game::init(Engine *engine) {
    // globals
    arena.init(64);

    // engine
    engine->clear_color = colorFromHex(0x8bbbffff);

    // invertory
    inventory[0] = InventorySlot{1, 1};

    // objects
    objects.items = {.len = OBJECTS_MAX, .ptr = objects_raw};

    player_down.init(engine->sprites.get("player_down"), 3);
    player_up.init(engine->sprites.get("player_up"), 3);
    player_right.init(engine->sprites.get("player_right"), 3);
    player_left.init(engine->sprites.get("player_right"), 3, true);

    player_id =objects.append({
        .direction = {0.0F, 1.0F},
        .timer = Timer::init(0.2F),
        .size = player_down.get(0).size(),
        .sprite = player_down.get(0),
        .kind = Object::PLAYER,
        .alive = true,
        .tint = WHITE,
    });

    attack_id = objects.append({
        .timer = Timer::init(0.1),
        .size = engine->sprites.get("attack_trail1").size(),
        .sprite = engine->sprites.get("attack_trail1"),
        .kind = Object::ATTACK,
        .tint = WHITE,
    });

    spell_id = objects.append({
        .size = engine->sprites.get("spell0").size(),
        .sprite = engine->sprites.get("spell0"),
        .kind = Object::SPELL,
        .tint = WHITE,
    });

    const i32 MAX_X = 1000;
    const i32 MAX_Y = 1000;

    const size_t ENEMY_COUNT = 100;
    for (size_t i = 0; i < ENEMY_COUNT; i++) {
        objects.append({
            .hp = 5,
            .timer = Timer::init(0.2F),
            .pos = {float(engine->rand(MAX_X)) - (MAX_X / 2.0F),
                    float(engine->rand(MAX_Y)) - (MAX_Y / 2.0F)},
            .size = engine->sprites.get("zombie").size(),
            .sprite = engine->sprites.get("zombie"),
            .kind = Object::ENEMY,
            .alive = true,
            .tint = WHITE,
        });
    }
}

vec2 Game::update(Engine *engine, Fixed<Instance> *instances) {
    if (engine->is_key_just_pressed(Key::escape)) engine->running = true;
    if (engine->is_key_just_pressed(Key::f3)) engine->show_fps = !engine->show_fps;

    const Object *player = &objects[player_id];
    Object *attack = &objects[attack_id];
    Object *spell = &objects[spell_id];

    vec2 camera = {};

    for (auto &object : objects) {
        if (!object.alive) continue;
        switch (object.kind) {
        case Object::PLAYER: {
            if (engine->is_key_just_pressed(Key::space)) {
                attack->alive = true;
            }
            if (engine->is_key_just_pressed(Key::f)) {
                spell->alive = true;
                spell->pos = object.pos + (object.size / 2) - (spell->size / 2);
                spell->direction = player->direction;
                spell->speed = 100;
            }

            vec2 velocity =
                vec2(
                    float(engine->is_key_pressed(Key::d)) - float(engine->is_key_pressed(Key::a)),
                    float(engine->is_key_pressed(Key::s)) - float(engine->is_key_pressed(Key::w)))
                    .normalize();

            u8 frame = 0;
            if (velocity.length() > 0.0F) {
                if (object.timer.advanceAndCheck(engine->dt)) {
                    object.frame += 1;
                    object.frame %= 4;
                };
                frame = object.frame;
                if (frame == 2) {
                    frame = 0;
                } else if (frame == 3) {
                    frame = 2;
                }

                object.direction = velocity;
                object.speed = 100;
            } else {
                object.speed = 0;
                object.frame = 0;
                object.timer.reset();
            }

            if (object.direction.y < 0) {
                object.sprite = player_up.get(frame);
            } else if (object.direction.y > 0) {
                object.sprite = player_down.get(frame);
            } else if (object.direction.x > 0) {
                object.sprite = player_right.get(frame);
            } else if (object.direction.x < 0) {
                object.sprite = player_left.get(frame);
            }

            camera = -player->pos + engine->screen / 2.0F - player->size / 2.0F;
            break;
        }
        case Object::ENEMY: {
            if (object.speed > 0.0F) {
                const float KNOCKBACK_FRICTION = 250;
                object.speed -= KNOCKBACK_FRICTION * engine->dt;
            } else {
                object.speed = 0.0F;
            }

            if (object.invincible) {
                object.tint = RED;
                if (object.timer.advanceAndCheck(engine->dt)) {
                    object.invincible = false;
                    object.tint = WHITE;
                }
            }

            if (attack->alive and
                checkCollisionSAT(attack->rect(), attack->angle, object.rect(), 0) and
                !object.invincible) {
                object.takeDamage(attack->direction, inventory);
            }

            if (spell->alive and checkCollisionAABB(spell->rect(), object.rect()) and
                !object.invincible) {
                object.takeDamage(spell->direction, inventory);
                spell->alive = false;
            }
            break;
        }
        case Object::ATTACK: {
            object.direction = player->direction;
            object.angle = atan2f(player->direction.y, player->direction.x);
            object.pos = player->pos + (player->direction * vec2{16.0F, 24.0F});
            if (object.timer.advanceAndCheck(engine->dt)) object.alive = false;
            break;
        }
        case Object::SPELL: {
            break;
        }
        case Object::NONE:
            assert(object.kind != Object::NONE);
            break;
        }
        object.pos += object.direction * engine->dt * object.speed;
        instances->append({object.pos, object.size, object.sprite, object.tint, object.angle});
    }

    instances->sort([](const void *a, const void *b) -> int {
        const auto *A = (const Instance *)a;
        const auto *B = (const Instance *)b;
        if (A->position.y < B->position.y) return -1;
        if (B->position.y < A->position.y) return 1;
        return 0;
    });

    return camera;
}

void Game::updateUI(Engine *engine, Fixed<Instance> *instances) {
    ScopeArena scope(&arena);

    if (engine->is_key_just_pressed(Key::e)) invertory_visible = !invertory_visible;

    if (invertory_visible) {
        for (size_t x_i = 0; x_i < 8; x_i++) {
            for (size_t y_i = 0; y_i < 8; y_i++) {
                const vec2 cell_size = {24.0F, 24.0F};
                const vec2 position = vec2{float(x_i), float(y_i)} * cell_size +
                                      (engine->screen - (cell_size * 8.0F));
                instances->append(
                    {position, cell_size, engine->sprites.get("inventory_slot"), WHITE, 0});
                const auto *invertory_slot = &inventory[(y_i * 8) + x_i];
                if (invertory_slot->item_id != 0) {
                    assert(invertory_slot->count);
                    assert(invertory_slot->count < 100);
                    auto text = scope.tmp.allocPrint("%d", invertory_slot->count);
                    instances->append(
                        {position, cell_size, engine->sprites.get("wheat_seeds"), WHITE, 0});
                    const float FONT_SIZE = 10;
                    const vec2 text_offset =
                        position +
                        (cell_size - vec2(engine->measureText({text.len, text.ptr}), FONT_SIZE));
                    engine->drawText(instances, {text.len, text.ptr}, text_offset);
                }
            }
        }
    }
}

void Game::deinit(Engine *engine) { arena.deinit(); }
