#pragma once

#include <sakana/string.hpp>
#include <sakana/types.hpp>

namespace sakana {

struct SourceCodeLocation {
    String file_path;
    i32 line;
    String procedure;
};

static SourceCodeLocation callerLocation(String file_path = __builtin_FILE(),
                                         i32 line = __builtin_LINE(),
                                         String procedure = __builtin_FUNCTION()

) {
    return {file_path, line, procedure};
}

} // namespace sakana
