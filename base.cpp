#include "base.hpp"

glm::vec2 Rectangle::overlapSize(const Rectangle &other) const {
    auto dx = fmin(x_max(), other.x_max()) - fmax(x, other.x);
    auto dy = fmin(y_max(), other.y_max()) - fmax(y, other.y);
    if (dx >= 0 and dy >= 0) {
        return {dx, dy};
    }
    return {};
}

void unreachable(const char *string, usize line) {
    printf("%s:%zu: unreachable\n", string, line);
    abort();
}

void todo(const char *string, usize line, const char *message) {
    printf("%s:%zu: %s\n", string, line, message);
    abort();
}

namespace OS {
Slice<u8> readEntireFile(Allocator &gpa, const String &name, Location location) {
    auto stream = fopen(name.c_str.rawptr, "r");
    assert(stream != nullptr);
    assert(fseek(stream, 0, SEEK_END) == 0);
    auto position = ftell(stream);
    assert(position != -1);
    auto n = (usize)position;
    assert(fseek(stream, 0, SEEK_SET) == 0);
    auto data = mem::alloc<u8>(gpa, n, location);
    assert(fread(data.rawptr, sizeof(u8), n, stream) == n);
    assert(fclose(stream) == 0);
    return data;
}
} // namespace OS

