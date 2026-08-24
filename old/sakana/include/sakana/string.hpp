#pragma once

#include <string.h>

#include <sakana/types.hpp>

namespace sakana {
class String {
    const char *rawptr;
    const usize len;

    friend usize len(const String &self);
    friend const char* rawptr(const String &self);

  public:
    template <usize N> constexpr String(const char (&rawptr)[N]) : rawptr(rawptr), len(N) {}
    String(const char *rawptr) : rawptr(rawptr), len(strlen(rawptr)) {}
};
} // namespace sakana
