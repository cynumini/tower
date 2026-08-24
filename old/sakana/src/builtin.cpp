#include <stdio.h>

#include <sakana/builtin.hpp>

namespace sakana {
usize len(const String &self) { return self.len; }
const char *rawptr(const String &self) { return self.rawptr; }

void unreachable(SourceCodeLocation loc) {
    printf("%*s:%d: unreachable\n", int(len(loc.file_path)), rawptr(loc.file_path),
           loc.line);
    abort();
}

void panic(const String &message, SourceCodeLocation loc) {
    printf("%*s:%d: panic: %*s\n", int(len(loc.file_path)), rawptr(loc.file_path),
           loc.line, int(len(message)), rawptr(message));
    abort();
}
} // namespace sakana
