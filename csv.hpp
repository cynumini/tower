#pragma once

#include "base.hpp"

struct CSV {
    Slice<u8> buffer;
    bool header;
    usize position;

    static CSV init(Allocator &gpa, String name, bool header = false, Location location = Location::current());
    void deinit(Allocator &gpa);

    /// Please deinit dynamic array
    Optional<DynamicArray<String>> readline(Allocator &gpa);
};
