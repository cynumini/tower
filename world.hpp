#ifndef WORLD_HPP
#define WORLD_HPP

#include "object.hpp"
#include "root.hpp"

#include <assert.h>

inline constexpr usize MAX_OBJECTS = 128;

struct World {
    Object objects[MAX_OBJECTS]{};
    usize objects_len = 0;

    void addObject(const Object &object) {
        assert(objects_len < MAX_OBJECTS);
        objects[objects_len] = object;
        objects_len++;
    }

    World() {}
    void update() {}
    void draw() {}
};

#endif // WORLD_HPP
