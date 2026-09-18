// clear line
#include "game.hpp"

#include "unagi.hpp"
// clear line

struct Timer {
    float elapsed = 0;
    float duration;

    constexpr static Timer init(float duration) noexcept { return {.duration = duration}; };
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

struct InvertorySlot {
    u8 item_id;
    u8 count;
};

const float ENEMY_KNOCKBACK_MAX_SPEED = 100;
const float ENEMY_KNOCKBACK_FRICTION = 250;
const vec2 ENEMY_SIZE = {16.0F, 32.0F};

struct Enemy {
    vec2 position;
    Color tint = WHITE;
    int hp = 5;
    bool invincible = false;
    Timer invincibility_timer = Timer::init(0.2F);
    vec2 knockback_direction = {};
    float knockback_speed = 0.0F;

    void takeDamage(vec2 direction, InvertorySlot *invertory) {
        knockback_direction = direction;
        knockback_speed = ENEMY_KNOCKBACK_MAX_SPEED;
        hp -= 1;
        invincible = true;
        if (hp == 0) {
            invertory[0].count += 1;
        }
    }
};

static vec2 player_size;
static vec2 player_position;
static bool attack;
static bool invertory_visible;
static bool cast_spell;
static u8 walking_frame;
static Timer walking_timer = Timer::init(0.2F);
static Timer attack_timer = Timer::init(0.1);
static vec2 direction = {0, 1};
static vec2 atlas_offset = {0.0F, 32.0F};
static bool flip_x;
static const size_t ENEMY_COUNT = 100;
static Enemy enemies[ENEMY_COUNT] = {};
static vec2 spell_position = {};
static vec2 spell_direction = {};
static const float SPELL_SPEED = 100;
static const vec2 SPELL_SIZE = vec2{10.0F, 10.0F};
static bool spell_alive = false;
static const vec2 PLAYER_SIZE = {16.0F, 32.0F};
static Arena *arena;

static InvertorySlot invertory[8 * 8];

const float FONT_SIZE = 10;

void Game::init(Engine *engine, Arena *a) {
    arena = a;

    player_size = {16.0F, 32.0F};
    engine->clear_color = colorFromHex(0x8bbbffff);
    invertory[0] = InvertorySlot{1, 1};

    atlas_offset = engine->sprites.get("player_down").position();

    const i32 MAX_X = 1000;
    const i32 MAX_Y = 1000;
    for (size_t i = 0; i < ENEMY_COUNT; i++) {
        enemies[i].position = {float(engine->rand(MAX_X)) - (MAX_X / 2.0F),
                               float(engine->rand(MAX_Y)) - (MAX_Y / 2.0F)};
    }
}

vec2 Game::update(Engine *engine, Fixed<Instance> *instances) {
    if (engine->is_key_just_pressed(Key::escape)) engine->running = true;
    if (engine->is_key_just_pressed(Key::f3)) engine->show_fps = !engine->show_fps;
    if (engine->is_key_just_pressed(Key::space)) attack = true;
    if (engine->is_key_just_pressed(Key::e)) invertory_visible = !invertory_visible;
    if (engine->is_key_just_pressed(Key::f)) cast_spell = true;

    vec2 velocity{float(engine->is_key_pressed(Key::d)) - float(engine->is_key_pressed(Key::a)),
                  float(engine->is_key_pressed(Key::s)) - float(engine->is_key_pressed(Key::w))};

    velocity = velocity.normalize();

    if (velocity.length() > 0.0F) {
        if (walking_timer.advanceAndCheck(engine->dt)) {
            walking_frame += 1;
            walking_frame %= 4;
        };
        direction = velocity;
    } else {
        walking_frame = 0;
        walking_timer.reset();
    }

    Rect uv = {0, 0, 16, 32};

    if (velocity.y < 0) {
        atlas_offset = engine->sprites.get("player_up").position();
    } else if (velocity.y > 0) {
        atlas_offset = engine->sprites.get("player_down").position();
    } else if (velocity.x > 0) {
        atlas_offset = engine->sprites.get("player_right").position();
        flip_x = false;
    } else if (velocity.x < 0) {
        atlas_offset = engine->sprites.get("player_right").position();
        flip_x = true;
    }

    if (walking_frame == 1) {
        uv.x = atlas_offset.x + 16.0F;
    } else if (walking_frame == 3) {
        uv.x = atlas_offset.x + 32.0F;
    } else {
        uv.x = atlas_offset.x + 0.0F;
    }

    uv.y = atlas_offset.y;

    const auto player_speed = 100.0F;
    player_position += velocity * engine->dt * player_speed;

    vec2 attack_position = player_position;
    const vec2 attack_size = {16.0F, 32.0F};
    attack_position += direction * vec2{16.0F, 24.0F};
    auto attack_angle = atan2f(direction.y, direction.x);

    if (attack) {
        if (attack_timer.advanceAndCheck(engine->dt)) attack = false;

        for (size_t i = 0; i < ENEMY_COUNT; i++) {
            if (enemies[i].hp <= 0) continue;
            if (checkCollisionSAT(Rect::fromVec(attack_position, attack_size), attack_angle,
                                  Rect::fromVec(enemies[i].position, ENEMY_SIZE), 0) and
                !enemies[i].invincible) {
                enemies[i].takeDamage(direction, invertory);
            }
        }
    }

    if (cast_spell) {
        spell_alive = true;
        spell_position = player_position;
        spell_position += PLAYER_SIZE / 2.0F - SPELL_SIZE / 2.0F;
        spell_direction = direction;
        cast_spell = false;
    }

    if (spell_alive) {
        spell_position += spell_direction * SPELL_SPEED * engine->dt;
        for (size_t i = 0; i < ENEMY_COUNT; i++) {
            if (enemies[i].hp <= 0) continue;
            if (checkCollisionAABB(Rect::fromVec(spell_position, SPELL_SIZE),
                                   Rect::fromVec(enemies[i].position, ENEMY_SIZE)) and
                !enemies[i].invincible) {
                enemies[i].takeDamage(spell_direction, invertory);
                spell_alive = false;
            }
        }
    }

    for (size_t i = 0; i < ENEMY_COUNT; i++) {
        if (enemies[i].knockback_speed > 0.0F) {
            enemies[i].position +=
                enemies[i].knockback_direction * enemies[i].knockback_speed * engine->dt;
            enemies[i].knockback_speed -= ENEMY_KNOCKBACK_FRICTION * engine->dt;
        } else {
            enemies[i].knockback_speed = 0.0F;
        }

        if (enemies[i].invincible) {
            enemies[i].tint = RED;
            if (enemies[i].invincibility_timer.advanceAndCheck(engine->dt)) {
                enemies[i].invincible = false;
                enemies[i].tint = WHITE;
            }
        }
    }

    // draw

    if (flip_x) {
        uv.x += uv.w;
        uv.w *= -1;
    }

    // engine->log("uv {%f, %f, %f, %f}", uv.x, uv.y, uv.w, uv.h);

    instances->append({player_position, PLAYER_SIZE, uv, WHITE, 0});
    for (size_t i = 0; i < ENEMY_COUNT; i++) {
        if (enemies[i].hp > 0) {
            instances->append({enemies[i].position, ENEMY_SIZE, engine->sprites.get("zombie"),
                               enemies[i].tint, 0});
        }
    }

    instances->sort([](const void *a, const void *b) -> int {
        const auto *A = (const Instance *)a;
        const auto *B = (const Instance *)b;
        if (A->position.y < B->position.y) return -1;
        if (B->position.y < A->position.y) return 1;
        return 0;
    });

    if (attack) {
        instances->append({attack_position, attack_size, engine->sprites.get("attack_trail1"),
                           WHITE, attack_angle});
    }

    if (spell_alive) {
        instances->append({spell_position, SPELL_SIZE, engine->sprites.get("spell0"), WHITE, 0});
    }

    return -player_position + engine->screen / 2.0F - player_size / 2.0F;
}

void Game::updateUI(Engine *engine, Fixed<Instance> *instances) {
    ScopeArena scope(arena);

    if (invertory_visible) {
        for (size_t x_i = 0; x_i < 8; x_i++) {
            for (size_t y_i = 0; y_i < 8; y_i++) {
                const vec2 cell_size = {24.0F, 24.0F};
                const vec2 position = vec2{float(x_i), float(y_i)} * cell_size +
                                      (engine->screen - (cell_size * 8.0F));
                instances->append(
                    {position, cell_size, engine->sprites.get("inventory_slot"), WHITE, 0});
                const auto *invertory_slot = &invertory[(y_i * 8) + x_i];
                if (invertory_slot->item_id != 0) {
                    assert(invertory_slot->count);
                    assert(invertory_slot->count < 100);
                    auto text = scope.tmp.allocPrint("%d", invertory_slot->count);
                    instances->append(
                        {position, cell_size, engine->sprites.get("wheat_seeds"), WHITE, 0});
                    const vec2 text_offset =
                        position +
                        (cell_size - vec2(engine->measureText({text.len, text.ptr}), FONT_SIZE));
                    engine->drawText(instances, {text.len, text.ptr}, text_offset);
                }
            }
        }
    }
}
