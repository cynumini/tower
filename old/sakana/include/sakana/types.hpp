#pragma once

#include <stddef.h>

namespace sakana {
using i32 = int;
static_assert(sizeof(i32) == 4);

using usize = size_t;
} // namespace sakana
