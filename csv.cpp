#include "csv.hpp"

CSV CSV::init(Allocator &gpa, String name, bool header, Location location) {
    return {OS::readEntireFile(gpa, name, location), header, 0};
}

void CSV::deinit(Allocator &gpa) { mem::free(gpa, buffer); }

Optional<DynamicArray<String>> CSV::readline(Allocator &gpa) {
    if (header) {
        while (buffer[position] != '\n' and position < buffer.len) {
            position += 1;
        }
        position += 1;
        header = false;
    }

    if (position >= buffer.len) {
        return {};
    }

    DynamicArray<String> result;

    usize start = position;
    while (position < buffer.len) {
        auto c = buffer[position];
        if (c == ',' or c == '\n') {
            result.add(gpa, {buffer.slice(start, position)});
            start = position + 1;
            if (c == '\n') {
                position++;
                break;
            }
        }
        position++;
    }

    if (position == buffer.len and start < position) {
        result.add(gpa, {buffer.slice(start, position)});
    }

    return {result};
}
