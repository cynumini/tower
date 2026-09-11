#include "game.hpp"

uint gameUpdate(Slice<Instance> instances) {
    uint instance_len = 0;
    instances.ptr[0] = {{0, 0}, {512, 512}, {0, 0, 1, 1}, WHITE, 0, 1};
    instances.ptr[1] = {{0, 0}, {24, 48}, {0, 0, 1, 1}, WHITE, 0, 0};
    instance_len = 2;
    return instance_len;
}
