#ifndef ROOT_HPP
#define ROOT_HPP

#include <stddef.h>
#include <stdint.h>

using f32 = float;
using u16 = uint16_t;
using u32 = uint32_t;
using u8 = uint8_t;
using usize = size_t;

static_assert(sizeof(f32) == 4);

#endif // ROOT_HPP
