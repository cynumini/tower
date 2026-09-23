#include "game.hpp"

#include "unagi.hpp"

// Init
static Arena arena;
static Rect solid;
static bool pause = false;

// Dialog
static bool dialog = false;
static const char *lines[2] = {"Hello!", "My name is Alice."};
static uint curr_line = 0;

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

// Inventory
Dynamic<Slice<const char>> items;

struct InventorySlot {
    u8 item_id;
    u8 count;
};
static InventorySlot inventory[8 * 8];
static bool inventory_visible;

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
    Rect interaction;
    Rect collision_rel;
    enum : u8 { NONE };
    enum Kind : u8 { PLAYER = 1, ENEMY, ATTACK, SPELL, NPC, BUILDING } kind;
    enum Body : u8 { MOVABLE = 1, STATIC } body;
    u8 frame;
    bool alive;
    bool invincible;
    Color tint;

    Rect interactionArea() const {
        const float PADDING = 8.0F;
        assert(kind == NPC);
        return {pos.x - PADDING, pos.y - PADDING, size.x + (PADDING * 2), size.y + (PADDING * 2)};
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

    const char *check() const {
        if (u8(kind) == NONE) return "object can't have Kind::NONE";
        if (isSolid() and u8(body) == NONE) return "solid objects can't have Body::NONE";
        return 0;
    }
};

struct Filter {
    Object *ptr;
    Object *end_ptr;
    bool (*predicate)(Object *);

    void skip() {
        while (ptr != end_ptr and !predicate(ptr)) ptr++;
    }

    Filter &operator++() {
        ptr++;
        skip();
        return *this;
    }

    Filter begin() {
        skip();
        return *this;
    }

    Filter end() { return {end_ptr, end_ptr, predicate}; }

    Object &operator*() const { return *ptr; }

    bool operator!=(const Filter &query) const { return ptr != query.ptr; }
};

static Filter makeFilter(Fixed<Object> object, bool (*predicate)(Object *)) {
    return Filter{object.begin(), object.end(), predicate};
}

const u8 OBJECTS_MAX = 255;
static Object objects_raw[OBJECTS_MAX];
static Fixed<Object> objects;

// ids
static size_t player_id;
static size_t attack_id;
static size_t spell_id;

// Locations
const float TITLE_SIZE = 24.0F;

static struct Location {
    const char *name;
    Rect rect;
    Rect ground;
} locations[2];

static struct ShowLocation {
    bool active;
    int curr_id;
    int prev_id;
    Timer timer;
} show_location = {false, -1, -1, Timer::init(2)};

void Game::init(Unagi *unagi) {
    // globals
    arena.init(512);
    solid = unagi->sprites.get("solid");

    // engine
    unagi->clear_color = colorFromHex(0x8bbbffff);

    // invertory
    uint items_len = 1;
    for (auto &mod : unagi->mods) {
        for (auto &_ : mod.items) items_len++;
    }
    items.init(&arena, items_len);
    items.append(&arena, sliceFromStrZ("wheat_seeds"));
    for (auto &mod : unagi->mods) {
        for (auto &item : mod.items) {
            auto key = arena.allocPrint("%*s/%*s", int(mod.name.len), mod.name.ptr, int(item.len),
                                        item.ptr);
            items.append(&arena, {key.len, key.ptr});
        }
    }
    for (u8 i = 0; i < u8(items.len); i++) {
        inventory[i] = InventorySlot{i, 1};
    }

    // objects
    objects.items = {.len = OBJECTS_MAX, .ptr = objects_raw};

    player_down.init(unagi->sprites.get("player_down"), 3);
    player_up.init(unagi->sprites.get("player_up"), 3);
    player_right.init(unagi->sprites.get("player_right"), 3);
    player_left.init(unagi->sprites.get("player_right"), 3, true);

    player_id = objects.append({
        .direction = {0.0F, 1.0F},
        .timer = Timer::init(0.2F),
        .pos = {TITLE_SIZE * 16.0F, TITLE_SIZE * 16.0F},
        .size = player_down.get(0).size(),
        .sprite = player_down.get(0),
        .collision_rel = {7.0F, 45.0F, 10.0F, 3.0F},
        .kind = Object::PLAYER,
        .body = Object::MOVABLE,
        .alive = true,
        .tint = WHITE,
    });

    attack_id = objects.append({
        .timer = Timer::init(0.1),
        .size = unagi->sprites.get("attack_trail1").size(),
        .sprite = unagi->sprites.get("attack_trail1"),
        .kind = Object::ATTACK,
        .tint = WHITE,
    });

    spell_id = objects.append({
        .size = unagi->sprites.get("spell0").size(),
        .sprite = unagi->sprites.get("spell0"),
        .kind = Object::SPELL,
        .tint = WHITE,
    });

    objects.append({
        .pos = {TITLE_SIZE * 48.0F, TITLE_SIZE * 16.0F},
        .size = unagi->sprites.get("character").size(),
        .sprite = unagi->sprites.get("character"),
        .collision_rel = {7.0F, 45.0F, 10.0F, 3.0F},
        .kind = Object::NPC,
        .body = Object::STATIC,
        .alive = true,
        .tint = WHITE,
    });

    {
        auto house = unagi->sprites.get("house");
        objects.append({
            .pos = {(TITLE_SIZE * 48.0F) - (house.w / 2.0F), 0},
            .size = house.size(),
            .sprite = house,
            .collision_rel = {1.0F, 47.0F, 192.0F, 162.0F},
            .kind = Object::BUILDING,
            .body = Object::STATIC,
            .alive = true,
            .tint = WHITE,
        });
    }

    const i32 MAX_X = 64;
    const i32 MAX_Y = 32;

    const size_t ENEMY_COUNT = 50;
    for (size_t i = 0; i < ENEMY_COUNT; i++) {
        objects.append({
            .hp = 5,
            .timer = Timer::init(0.2F),
            .pos = {float(unagi->rand(MAX_X)) * TITLE_SIZE,
                    (float(unagi->rand(MAX_Y)) * TITLE_SIZE) - 16},
            .size = unagi->sprites.get("zombie").size(),
            .sprite = unagi->sprites.get("zombie"),
            .collision_rel = {7.0F, 45.0F, 10.0F, 3.0F},
            .kind = Object::ENEMY,
            .body = Object::MOVABLE,

            .alive = true,
            .tint = WHITE,
        });
    }
    const Rect grass = unagi->sprites.get("grass");
    const Rect dirt = unagi->sprites.get("dirt");

    // Location
    locations[0] = {"Home", {0, 0, TITLE_SIZE * 32, TITLE_SIZE * 32}, dirt};
    locations[1] = {"Town", {TITLE_SIZE * 32, 0, TITLE_SIZE * 32, TITLE_SIZE * 32}, grass};

    // Check objects
    auto id = 0;
    for (auto &object : objects) {
        const char *message = object.check();
        if (message != 0) {
            unagi->log("id = %d, %s", id, message);
            assert(message == 0);
        }
        id++;
    }
}

vec2 Game::update(Unagi *unagi, Fixed<Instance> *instances) {
    // cheat && system
    if (unagi->is_key_just_pressed(Key::key_1)) {
        quest.status = Quest::COMPLETED;
    }
    if (unagi->is_key_just_pressed(Key::escape)) unagi->running = true;
    if (unagi->is_key_just_pressed(Key::f3)) unagi->debug_mode = !unagi->debug_mode;

    // map
    for (u8 i = 0; i < u8(ARRAY_LEN(locations)); ++i) {
        const auto *location = &locations[i];

        const int cols = int(location->rect.w / TITLE_SIZE);
        const int rows = int(location->rect.h / TITLE_SIZE);

        for (int tx = 0; tx < cols; ++tx) {
            for (int ty = 0; ty < rows; ++ty) {
                const float x = location->rect.x + (float(tx) * TITLE_SIZE);
                const float y = location->rect.y + (float(ty) * TITLE_SIZE);
                instances->append({{x, y}, {TITLE_SIZE, TITLE_SIZE}, location->ground, WHITE, 0});
            }
        }
    }

    auto offset = instances->len;

    // objects
    Object *player = &objects[player_id];
    Object *attack = &objects[attack_id];
    Object *spell = &objects[spell_id];

    vec2 camera = {};

    for (auto &object : objects) {
        assert(object.kind != Object::NONE);
        if (!object.alive) continue;
        if (!pause) {
            switch (object.kind) {
            case Object::PLAYER: {
                if (unagi->is_key_just_pressed(Key::space, false)) {
                    attack->alive = true;
                }
                if (unagi->is_key_just_pressed(Key::f)) {
                    spell->alive = true;
                    spell->pos = object.pos + (object.size / 2) - (spell->size / 2);
                    spell->direction = player->direction;
                    spell->speed = 100;
                }

                const vec2 velocity = vec2(float(unagi->is_key_pressed(Key::d)) -
                                               float(unagi->is_key_pressed(Key::a)),
                                           float(unagi->is_key_pressed(Key::s)) -
                                               float(unagi->is_key_pressed(Key::w)))
                                          .normalize();

                u8 frame = 0;
                if (velocity.length() > 0.0F) {
                    if (object.timer.advanceAndCheck(unagi->dt)) {
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

                break;
            }
            case Object::ENEMY: {
                if (object.speed > 0.0F) {
                    const float KNOCKBACK_FRICTION = 250;
                    object.speed -= KNOCKBACK_FRICTION * unagi->dt;
                } else {
                    object.speed = 0.0F;
                }

                if (object.invincible) {
                    object.tint = RED;
                    if (object.timer.advanceAndCheck(unagi->dt)) {
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
                object.pos = player->pos + ((player->size / 2) - (object.size / 2));
                object.pos += object.direction * vec2(24, 32);
                if (object.timer.advanceAndCheck(unagi->dt)) object.alive = false;
                break;
            }
            case Object::NPC: {
                quest.pos = {object.pos};
                quest.pos.x += object.size.x / 2;
                if (!dialog) {
                    if (checkCollisionAABB(object.interactionArea(), player->rect())) {
                        if (unagi->is_key_just_pressed(Key::space)) {
                            dialog = true;
                            pause = true;
                        }
                    }
                }
                break;
            }
            case Object::NONE:
                break;
            }

            if (object.isSolid() and object.body == Object::MOVABLE) {
                Filter q = makeFilter(
                    objects, [](Object *other) { return other->isSolid() and other->alive; });
                vec2 velocity = object.direction * unagi->dt * object.speed;
                object.pos.x += velocity.x;
                for (Object &other : q) {
                    if (&object != &other and
                        checkCollisionAABB(object.getCollision(), other.getCollision())) {
                        object.pos.x -= velocity.x;
                        break;
                    }
                }
                object.pos.y += velocity.y;
                for (Object &other : q) {
                    if (&object != &other and
                        checkCollisionAABB(object.getCollision(), other.getCollision())) {
                        object.pos.y -= velocity.y;
                        break;
                    }
                }
            } else {
                object.pos += object.direction * unagi->dt * object.speed;
            }
        }
        camera = -player->pos + unagi->screen / 2.0F - player->size / 2.0F;
        if (object.kind == Object::ATTACK or object.kind == Object::SPELL) continue;
        instances->append({object.pos, object.size, object.sprite, object.tint, object.angle});
    }

    // current location
    {
        bool outside = true;
        for (u8 i = 0; i < u8(ARRAY_LEN(locations)); i++) {
            auto *location = &locations[i];
            if (checkCollisionAABB(player->rect(), location->rect)) {
                outside = false;
                show_location.curr_id = i;
            }
        }
        if (outside) {
            show_location.curr_id = -1;
        }
        if (show_location.curr_id != show_location.prev_id) {
            show_location.active = true;
            show_location.timer.reset();
            show_location.prev_id = show_location.curr_id;
        } else {
        }
    }

    instances->sort(
        [](const void *a, const void *b) -> int {
            const auto *a_instance = (const Instance *)a;
            const auto *b_instance = (const Instance *)b;
            auto a_y = a_instance->position.y + a_instance->size.y;
            auto b_y = b_instance->position.y + b_instance->size.y;
            if (a_y < b_y) return -1;
            if (b_y < a_y) return 1;
            return 0;
        },
        offset);

    // draw attack and spell
    if (attack->alive) {
        instances->append(
            {attack->pos, attack->size, attack->sprite, attack->tint, attack->angle});
    }
    if (spell->alive) {
        instances->append({spell->pos, spell->size, spell->sprite, spell->tint, spell->angle});
    }

    // quest marker
    if (quest.status != Quest::REWARDED) {
        const uint SIZE = 20;
        Slice<const char> text = sliceFromStrZ("!");
        auto color = YELLOW;
        if (quest.status != Quest::AVAILABLE) {
            text = sliceFromStrZ("?");
            if (quest.status == Quest::ACTIVE) color = WHITE;
        }
        auto pos =
            vec2(quest.pos.x - (unagi->measureText(text, SIZE) / 2.0F), quest.pos.y - SIZE);
        unagi->drawText(instances, text, pos, SIZE, color);
    }

    // debug (show collision)
    if (unagi->debug_mode) {
        for (auto &object : objects) {
            if (!object.alive or object.kind == Object::NONE) continue;

            Color color = {191, 0, 255, 127};
            vec2 pos = object.pos;
            vec2 size = object.size;

            if (object.kind == Object::NPC) {
                color = {255, 0, 0, 127};
                pos = object.interactionArea().position();
                size = object.interactionArea().size();
            }

            instances->append({
                .position = pos,
                .size = size,
                .uv = solid,
                .color = color,
                .rotation = object.angle,
            });

            if (object.isSolid()) {
                auto collision = object.getCollision();
                instances->append({
                    .position = collision.position(),
                    .size = collision.size(),
                    .uv = solid,
                    .color = {0, 0, 255, 127},
                    .rotation = object.angle,
                });
            }
        }
    }

    return camera;
}

void Game::updateUI(Unagi *unagi, Fixed<Instance> *instances) {
    ScopeArena scope(&arena);

    if (unagi->is_key_just_pressed(Key::e)) inventory_visible = !inventory_visible;

    while (dialog) {
        Slice<const char> text = {};
        if (quest.status == Quest::REWARDED) {
            if (curr_line == ARRAY_LEN(lines)) {
                dialog = pause = false;
                curr_line = 0;
                break;
            }
            text = sliceFromStrZ(lines[curr_line]);
            curr_line += unagi->is_key_just_pressed(Key::space);
        } else {
            text = sliceFromStrZ(quest.line[quest.status]);
            if (unagi->is_key_just_pressed(Key::space)) {
                dialog = pause = false;
                if (quest.status == Quest::COMPLETED) quest.status = Quest::REWARDED;
                if (quest.status == Quest::AVAILABLE) quest.status = Quest::ACTIVE;
                break;
            }
        }
        vec2 pos = {0.0F, float(unagi->screen.y) * 2.0F / 3.0F};
        instances->append({
            .position = pos,
            .size = {float(unagi->screen.x), float(unagi->screen.y) / 3.0F},
            .uv = solid,
            .color = BLACK,
            .rotation = 0,
        });
        unagi->drawText(instances, text, pos + vec2(4, 4));
        break;
    }

    if (show_location.active) {
        if (show_location.timer.advanceAndCheck(unagi->dt)) {
            show_location.active = false;
        } else {
            auto height = 20.0F;
            const char *name;

            if (show_location.curr_id == -1) {
                name = "Outside";
            } else {
                name = locations[show_location.curr_id].name;
            }

            auto width = unagi->measureText(sliceFromStrZ(name), height);

            u8 alpha = 255;
            if (show_location.timer.elapsed > 1.0F) {
                alpha = u8((1.0F - (show_location.timer.elapsed - 1.0F)) * 255);
            }

            unagi->drawText(instances, sliceFromStrZ(name),
                            {(float(unagi->screen.x) / 2.0F) - (width / 2.0F),
                             (float(unagi->screen.y) / 2.0F) - (height / 2.0F)},
                            height, {WHITE.r, WHITE.g, WHITE.b, alpha});
        }
    }

    if (inventory_visible) {
        for (size_t x_i = 0; x_i < 8; x_i++) {
            for (size_t y_i = 0; y_i < 8; y_i++) {
                const vec2 cell_size = unagi->sprites.get("inventory_slot").size();
                const vec2 position = vec2{float(x_i), float(y_i)} * cell_size +
                                      (unagi->screen - (cell_size * 8.0F));
                instances->append(
                    {position, cell_size, unagi->sprites.get("inventory_slot"), WHITE, 0});
                const auto *invertory_slot = &inventory[(y_i * 8) + x_i];
                if (invertory_slot->count != 0) {
                    assert(invertory_slot->count);
                    assert(invertory_slot->count < 100);
                    auto text = scope.tmp.allocPrint("%d", invertory_slot->count);
                    instances->append({position, cell_size,
                                       unagi->sprites.get(items[invertory_slot->item_id]), WHITE,
                                       0});
                    const float FONT_SIZE = 10;
                    const vec2 text_offset =
                        position +
                        (cell_size - vec2(unagi->measureText({text.len, text.ptr}), FONT_SIZE));
                    unagi->drawText(instances, {text.len, text.ptr}, text_offset);
                }
            }
        }
    }
}

void Game::deinit(Unagi *engine) { arena.deinit(); }
