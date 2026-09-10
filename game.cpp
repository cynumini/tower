#include "game.hpp"

uint gameUpdate(Slice<Instance> instances) {
    uint instance_len = 0;
    instances.ptr[0] = {{1, 1}, {24, 48}, {0, 0, 1, 1}, WHITE, 0, 0};
    return ++instance_len;
}
