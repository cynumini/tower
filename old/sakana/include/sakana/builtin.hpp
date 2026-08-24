#pragma once

#include <stdlib.h>

#include <sakana/core.hpp>

namespace sakana {

usize len(const String &self);
const char *rawptr(const String &self);

[[noreturn]] void unreachable(SourceCodeLocation loc = callerLocation());

[[noreturn]] void panic(const String &message, SourceCodeLocation loc = callerLocation());
} // namespace sakana
