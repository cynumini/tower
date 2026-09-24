#pragma once

#include "game.hpp"

static struct Quest {
    enum : u8 { AVAILABLE, ACTIVE, COMPLETED, REWARDED } status;
    vec2 pos;
    const char *line[Quest::REWARDED];
    uint counter;
} quest = {
    Quest::AVAILABLE,
    {},
    {
        "Please, kill 10 zombies, and I'll give you 10 gold coins!",
        "Have you killed them yet?",
        "Thank you very much! Here, take these 10 gold coins.",
    },
    0,
};

// Itesm
static Dynamic<Slice<const char>> items;

// Invertory
struct InventorySlot {
    u8 item_id;
    u8 count;
};
static InventorySlot inventory[8 * 8];
static bool inventory_visible;

// Timer
struct Timer {
    float elapsed;
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

static struct Level {
    enum class Id : u8 { world, house };
    Color clear_color;
} levels[] = {{colorFromHex(0x8bbbffff)}, {BLACK}};

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
    Rect interaction_rel;
    Rect collision_rel;
    enum class Kind : u8 { player, enemy, attack, spell, npc, building } kind;
    enum class Body : u8 { none, movable, immovable } body;
    u8 frame;
    bool alive;
    bool invincible;
    Color tint;
    Level::Id level;

    static Object create(Kind kind, Rect sprite, bool alive = false, vec2 pos = {}) {
        return {.size = sprite.size(), .sprite = sprite, .kind = kind, .tint = WHITE};
    }

    void addCollision(Body body, Rect rect) {
        this->body = body;
        collision_rel = rect;
    }

    bool isInteractable() const { return interaction_rel.w != 0 and interaction_rel.h != 0; }

    Rect getInteraction() const {
        return {
            pos.x + interaction_rel.x,
            pos.y + interaction_rel.y,
            interaction_rel.w,
            interaction_rel.h,
        };
    }

    bool isSolid() const { return collision_rel.w != 0 and collision_rel.h != 0; }

    Rect getCollision() {
        return {
            pos.x + collision_rel.x,
            pos.y + collision_rel.y,
            collision_rel.w,
            collision_rel.h,
        };
    }

    void takeDamage(vec2 direction, InventorySlot *inventory) {
        this->direction = direction;
        const float KNOCKBACK_SPEED = 100;
        speed = KNOCKBACK_SPEED;
        hp -= 1;
        invincible = true;
        if (hp == 0) {
            alive = false;
            if (quest.status == Quest::ACTIVE) {
                quest.counter += 1;
                if (quest.counter >= 10) {
                    quest.status = Quest::COMPLETED;
                }
            }
            inventory[0].count += 1;
        }
    }

    Rect rect() { return {pos.x, pos.y, size.x, size.y}; }

    // const char *check() const {
    //     if (u8(kind) == NONE) return "object can't have Kind::NONE";
    //     if (isSolid() and u8(body) == NONE) return "solid objects can't have Body::NONE";
    //     return 0;
    // }
};
