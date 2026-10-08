#pragma once

#include <algorithm>

#include <skn.cpp>
#include <skn_math.cpp>

#include "shared.cpp"

#include "map.cpp"

static struct Quest {
    enum : u8 { AVAILABLE, ACTIVE, COMPLETED, REWARDED } status;
    Vec3 pos;
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

// Items
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

// Object
struct Object {
    float speed;
    float angle;
    int hp;
    Vec2 direction;
    Timer timer;
    Vec3 pos;
    Vec2 size;
    Rect sprite;
    Rect interaction_rel;
    Rect collision_rel;
    enum class Kind : u8 { player, enemy, attack, spell, npc, building } kind;
    enum class Body : u8 { none, movable, immovable } body;
    u8 frame;
    bool alive;
    bool invincible;
    Color tint;

    static Object create(Kind kind, Rect sprite, bool alive = false, Vec3 pos = {}) {
        return {.pos = pos,
                .size = sprite.size() / 64,
                .sprite = sprite,
                .kind = kind,
                .alive = alive,
                .tint = WHITE};
    }

    void addCollision(Body body, Rect rect) {
        this->body = body;
        collision_rel = rect;
    }

    bool isInteractable() const { return interaction_rel.w != 0 and interaction_rel.h != 0; }

    Rect getInteraction() const {
        return {
            {pos.x + interaction_rel.x, pos.y + interaction_rel.y},
            {interaction_rel.w, interaction_rel.h},
        };
    }

    bool isSolid() const { return collision_rel.w != 0 and collision_rel.h != 0; }

    Rect getCollision() {
        return {
            {pos.x + collision_rel.x, pos.y + collision_rel.y},
            {collision_rel.w, collision_rel.h},
        };
    }

    void takeDamage(Vec2 direction, InventorySlot *inventory) {
        this->direction = direction;
        const float KNOCKBACK_SPEED = 10;
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

    Rect rect() { return {Vec2(pos), {size.x, size.y}}; }
};

// Init
static Rect solid;
static bool pause = false;
static bool interaction_frame = false;

// Dialog
static bool dialog = false;
static const char *lines[2] = {"Hello!", "My name is Alice."};
static uint curr_line = 0;

// Animation
static struct Animation {
    u8 frames;
    Rect origin;
    bool flip_x;

    void init(Rect texture, u8 frames, bool flip_x = false) {
        this->flip_x = flip_x;
        this->frames = frames;
        this->origin = {
            {
                texture.x,
                texture.y,
            },
            {texture.w / frames, texture.h},
        };
    }

    Rect get(u8 index) const {
        assert(index < frames);
        Rect result = {
            {origin.x + (origin.w * index), origin.y},
            {origin.w, origin.h},
        };
        if (flip_x) result.x += result.w, result.w *= -1;
        return result;
    }
} player_down, player_right, player_left, player_up;

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

// Mpas
static Map map;

// ids
static Object *player;
static Object *attack;
static Object *spell;

static struct ShowLocation {
    bool active;
    int curr_id;
    int prev_id;
    Timer timer;
} show_location = {false, -1, -1, Timer::init(2)};

static struct Game {
    void init(Engine *engine) {

        // globals
        solid = engine->sprites.get("solid");
        engine->clear_color = 0x8bbbffff;

        // invertory
        uint items_len = 1;
        for (auto &mod : engine->mods) {
            for (auto &_ : mod.items) items_len++;
        }
        items.init(&arena, items_len);
        items.append(&arena, sliceFromStrZ("wheat_seeds"));
        for (auto &mod : engine->mods) {
            for (auto &item : mod.items) {
                auto key = arena.allocPrint("%*s/%*s", int(mod.name.len), mod.name.ptr,
                                            int(item.len), item.ptr);
                items.append(&arena, {key.len, key.ptr});
            }
        }
        for (u8 i = 0; i < u8(items.len); i++) {
            inventory[i] = InventorySlot{i, 1};
        }

        // animations
        player_down.init(engine->sprites.get("player_down"), 3);
        player_up.init(engine->sprites.get("player_up"), 3);
        player_right.init(engine->sprites.get("player_right"), 3);
        player_left.init(engine->sprites.get("player_right"), 3, true);

        // objects
        objects = {{0, objects_raw}, OBJECTS_MAX};

        using Kind = Object::Kind;
        using Body = Object::Body;

        player =
            objects.append(Object::create(Kind::player, player_down.get(0), true, {0, 0, 64}));
        player->timer = Timer::init(0.2F);
        player->direction = {0.0F, 1.0F};
        player->addCollision(Body::movable, {{0.0F, 0.0F}, {0.5F, 0.5F}});

        attack =
            objects.append(Object::create(Kind::attack, engine->sprites.get("attack_trail1")));
        attack->timer = Timer::init(0.1F);

        spell = objects.append(Object::create(Kind::spell, engine->sprites.get("spell0")));

        {
            auto *object = objects.append(
                Object::create(Kind::npc, engine->sprites.get("character"), true, {32, 0, 64}));
            object->addCollision(Body::immovable, {{0.0F, 0.0F}, {0.5F, 0.5F}});
            object->interaction_rel = Rect(0, 0, 1, 1);
        }

        // {
        //     auto house = engine->sprites.get("house");
        //     auto *object = objects.append(Object::create(Kind::building, house, true));
        //     object->addCollision(Body::immovable, {{1.0F, 47.0F}, {192.0F, 162.0F}});
        //     object->pos = {(TITLE_SIZE * 48.0F) - (house.w / 2.0F), 0};
        //     object->interaction_rel = {{80.0F, 208.0F}, {30.0F, 2.0F}};
        // }

        const i32 MAX_X = 64;
        const i32 MAX_Y = 32;

        const u8 ENEMY_COUNT = 50;
        for (u8 i = 0; i < ENEMY_COUNT; i++) {
            auto *object =
                objects.append(Object::create(Kind::enemy, engine->sprites.get("zombie"), true,
                                              {float(engine->rand(MAX_X)) - MAX_X / 4.0F,
                                               float(engine->rand(MAX_Y)) - MAX_Y / 2.0F, 64}));
            object->addCollision(Body::movable, {{0, 0}, {0.5F, 0.5F}});
            object->hp = 5;
            object->timer = Timer::init(0.2F);
        }

        map.init(engine);
    }

    void update(Engine *engine, WorldInstances *instances) {

        float yaw =
            engine->is_key_just_released(Key::kp_4) - engine->is_key_just_released(Key::kp_6);
        float pitch =
            engine->is_key_just_released(Key::kp_2) - engine->is_key_just_released(Key::kp_8);
        float roll =
            engine->is_key_just_released(Key::kp_7) - engine->is_key_just_released(Key::kp_9);

        engine->camera.yaw += yaw * 45;
        engine->camera.yaw = int(engine->camera.yaw) % 360;
        engine->camera.pitch += pitch * 5;
        engine->camera.roll += roll * 5;

        // TODO: way to exit house
        // TODO: move NPC to house
        // TODO: don't spawn zombie into house
        // TODO: show debug collision only realted to current level

        // cheat && system
        if (engine->is_key_just_pressed(Key::key_1)) quest.status = Quest::COMPLETED;

        map.update(engine, instances, player->pos);

        for (auto &object : objects) {
            if (!object.alive) continue;
            if (!pause) {
                switch (object.kind) {
                case Object::Kind::player: {
                    if (engine->is_key_just_pressed(Key::space)) {
                        attack->alive = true;
                    }
                    if (engine->is_key_just_pressed(Key::f)) {
                        spell->alive = true;
                        spell->pos = object.pos;
                        spell->direction = player->direction;
                        spell->speed = 8;
                    }

                    const Vec2 velocity = Vec2(float(engine->is_key_pressed(Key::d)) -
                                                   float(engine->is_key_pressed(Key::a)),
                                               float(engine->is_key_pressed(Key::w)) -
                                                   float(engine->is_key_pressed(Key::s)))
                                              .normalize()
                                              .rotate(deg2rad(engine->camera.yaw));

                    int level_diff = engine->is_key_just_released(Key::pageup) -
                                     engine->is_key_just_released(Key::pagedown);
                    player->pos.z =
                        std::clamp(player->pos.z + level_diff, 1.0F, float(Map::MAP_MAX.z - 1));

                    if (engine->is_key_just_released(Key::kp_5)) {
                        map.hide_above_level = !map.hide_above_level;
                    }

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
                        object.speed = 4;
                    } else {
                        object.speed = 0;
                        object.frame = 0;
                        object.timer.reset();
                    }

                    auto d = object.direction.rotate(deg2rad(-engine->camera.yaw));
                    if (d.y > 0) {
                        object.sprite = player_up.get(frame);
                    } else if (d.y < 0) {
                        object.sprite = player_down.get(frame);
                    } else if (d.x > 0) {
                        object.sprite = player_right.get(frame);
                    } else if (d.x < 0) {
                        object.sprite = player_left.get(frame);
                    }

                    break;
                }
                case Object::Kind::enemy: {
                    if (object.speed > 0.0F) {
                        const float KNOCKBACK_FRICTION = 40;
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
                        checkCollisionSAT(attack->rect(), attack->angle, object.getCollision(),
                                          0) and
                        !object.invincible) {
                        object.takeDamage(attack->direction, inventory);
                    }

                    if (spell->alive and
                        checkCollisionAABB(spell->rect(), object.getCollision()) and
                        !object.invincible) {
                        object.takeDamage(spell->direction, inventory);
                        spell->alive = false;
                    }
                    break;
                }
                case Object::Kind::attack: {
                    object.direction = player->direction;
                    object.angle = atan2f(player->direction.y, player->direction.x);
                    object.pos = player->pos + object.direction * 0.5;
                    if (object.timer.advanceAndCheck(engine->dt)) object.alive = false;
                    break;
                }
                case Object::Kind::npc: {
                    quest.pos = object.pos;
                    if (!dialog) {
                        if (checkCollisionAABB(object.getInteraction(), player->getCollision())) {
                            if (engine->is_key_just_pressed(Key::space)) {
                                dialog = true;
                                pause = true;
                                interaction_frame = true;
                            }
                        }
                    }
                    break;
                }
                case Object::Kind::spell:
                    break;
                case Object::Kind::building:
                    // TODO: building move player to second level
                    break;
                }

                if (object.isSolid() and object.body == Object::Body::movable) {
                    Filter q = makeFilter(
                        objects, [](Object *other) { return other->isSolid() and other->alive; });
                    Vec2 velocity = object.direction * engine->dt * object.speed;
                    object.pos.x += velocity.x;
                    for (Object &other : q) {
                        if (other.pos.z != object.pos.z) continue;
                        if (&object != &other and
                            checkCollisionAABB(object.getCollision(), other.getCollision())) {
                            object.pos.x -= velocity.x;
                            break;
                        }
                    }
                    object.pos.y += velocity.y;
                    for (Object &other : q) {
                        if (other.pos.z != object.pos.z) continue;
                        if (&object != &other and
                            checkCollisionAABB(object.getCollision(), other.getCollision())) {
                            object.pos.y -= velocity.y;
                            break;
                        }
                    }
                } else {
                    object.pos += object.direction * engine->dt * object.speed;
                }
            }
        }

        for (auto &object : objects) {
            if (!object.alive) continue;
            bool use_default = true;
            switch (object.kind) {
            case Object::Kind::player:
            case Object::Kind::enemy:
                break;
            case Object::Kind::attack: {
                use_default = false;
                instances->squares.append({{object.pos},
                                           object.size,
                                           object.sprite,
                                           object.tint,
                                           Face::z_pos,
                                           object.angle});
                break;
            }
            case Object::Kind::spell:
            case Object::Kind::npc:
            case Object::Kind::building:
                break;
            }
            if (use_default) {
                instances->squares.append({{object.pos},
                                           object.size,
                                           object.sprite,
                                           object.tint,
                                           Face::billboard,
                                           0.0F});
            }
        }

        engine->camera.pos = player->pos;

        // current location
        {
            bool outside = true;
            for (u8 i = 0; i < u8(ARRAY_LEN(locations)); i++) {
                auto *location = &locations[i];
                if (checkCollisionPointRect(
                        player->pos,
                        {location->rect.position() - Vec3(0.5F, 0.5F), location->rect.size()})) {
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

        // quest marker
        if (quest.status != Quest::REWARDED) {
            const uint SIZE = 5;
            char sign = '!';
            auto color = YELLOW;
            if (quest.status != Quest::AVAILABLE) {
                sign = '?';
                if (quest.status == Quest::ACTIVE) color = WHITE;
            }
            engine->drawWorldSymbol(&instances->squares, {quest.pos.xy(), quest.pos.z + 0.75F},
                                    sign, SIZE, color, engine->default_font);
        }

        // debug (show collision)
        if (engine->debug_mode) {
            // auto home = locations->rect;
            // instances->append({
            //     .position = Vec3(home.position(), 0),
            //     .size = home.size(),
            //     .uv = solid,
            //     .color = 0xff00FF7f,
            //     .face = Face::z_pos,
            //     .rotation = 0,
            // });

            for (auto &object : objects) {
                if (!object.alive) continue;

                if (object.kind == Object::Kind::attack) {
                    instances->squares.append({
                        .position = object.pos,
                        .size = object.size,
                        .uv = solid,
                        .color = 0xff00FF7f,
                        .face = Face::z_pos,
                        .rotation = object.angle,

                    });
                }

                if (object.isInteractable()) {
                    auto collision = object.getInteraction();
                    instances->squares.append({
                        .position = object.pos,
                        .size = collision.size(),
                        .uv = solid,
                        .color = 0xff00007f,
                        .face = Face::z_pos,
                        .rotation = object.angle,

                    });
                }

                if (object.isSolid()) {
                    auto collision = object.getCollision();
                    instances->squares.append({
                        .position = object.pos,
                        .size = collision.size(),
                        .uv = solid,
                        .color = 0x0000ff7f,
                        .face = Face::z_pos,
                        .rotation = object.angle,
                    });
                }
            }
        };
    }

    void updateUI(Engine *engine, Fixed<UIInstance> *instances, int render_width,
                  int render_height) {
        ScopeArena scope(&arena);

        if (engine->is_key_just_pressed(Key::e)) inventory_visible = !inventory_visible;

        while (dialog) {
            Slice<const char> text = {};
            if (quest.status == Quest::REWARDED) {
                if (curr_line == ARRAY_LEN(lines)) {
                    dialog = pause = false;
                    curr_line = 0;
                    break;
                }
                text = sliceFromStrZ(lines[curr_line]);
                curr_line += engine->is_key_just_pressed(Key::space) and (not interaction_frame);
            } else {
                text = sliceFromStrZ(quest.line[quest.status]);
                if (engine->is_key_just_pressed(Key::space) and (not interaction_frame)) {
                    dialog = pause = false;
                    if (quest.status == Quest::COMPLETED) quest.status = Quest::REWARDED;
                    if (quest.status == Quest::AVAILABLE) quest.status = Quest::ACTIVE;
                    break;
                }
            }
            Vec2 pos = {0.0F, float(render_height) * 2.0F / 3.0F};
            instances->append({
                .position = pos,
                .size = {float(render_width), float(render_height) / 3.0F},
                .uv = solid,
                .color = BLACK,
            });
            engine->drawText(instances, text, pos + Vec2(4, 4));
            break;
        }

        if (show_location.active) {
            if (show_location.timer.advanceAndCheck(engine->dt)) {
                show_location.active = false;
            } else {
                auto height = 20.0F;
                const char *name;

                if (show_location.curr_id == -1) {
                    name = "Outside";
                } else {
                    name = locations[show_location.curr_id].name;
                }

                auto width = engine->measureText(sliceFromStrZ(name), height);

                u8 alpha = 255;
                if (show_location.timer.elapsed > 1.0F) {
                    alpha = u8((1.0F - (show_location.timer.elapsed - 1.0F)) * 255);
                }

                engine->drawText(instances, sliceFromStrZ(name),
                                 {(float(render_width) / 2.0F) - (width / 2.0F),
                                  (float(render_height) / 2.0F) - (height / 2.0F)},
                                 height, {WHITE.r, WHITE.g, WHITE.b, alpha});
            }
        }

        if (inventory_visible) {
            for (size_t x_i = 0; x_i < 8; x_i++) {
                for (size_t y_i = 0; y_i < 8; y_i++) {
                    const Vec2 cell_size = engine->sprites.get("inventory_slot").size();
                    const Vec2 position =
                        Vec2{float(x_i), float(y_i)} * cell_size +
                        (Vec2{float(render_width), float(render_height)} - (cell_size * 8.0F));
                    instances->append(
                        {position, cell_size, engine->sprites.get("inventory_slot"), WHITE});
                    const auto *invertory_slot = &inventory[(y_i * 8) + x_i];
                    if (invertory_slot->count != 0) {
                        assert(invertory_slot->count);
                        assert(invertory_slot->count < 100);
                        auto text = scope.tmp.allocPrint("%d", invertory_slot->count);
                        instances->append({position, cell_size,
                                           engine->sprites.get(items[invertory_slot->item_id]),
                                           WHITE});
                        const float FONT_SIZE = 10;
                        const Vec2 text_offset =
                            position +
                            (cell_size -
                             Vec2(engine->measureText({text.len, text.ptr}), FONT_SIZE));
                        engine->drawText(instances, text, text_offset);
                    }
                }
            }
        }

        if (interaction_frame) {
            interaction_frame = false;
        }
    }

    void deinit(Engine *engine) {}
} game;
