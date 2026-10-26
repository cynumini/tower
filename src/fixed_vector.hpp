#pragma once

#include <SDL3/SDL_assert.h>

template <typename T> struct FixedVector {
    T *ptr;
    size_t capacity;
    size_t size;
};

template <typename T> T *begin(FixedVector<T> &v) { return v.ptr; }

template <typename T> T *end(FixedVector<T> &v) { return v.ptr + v.size; }

template <typename T> const T *begin(const FixedVector<T> &v) { return v.ptr; }

template <typename T> const T *end(const FixedVector<T> &v) { return v.ptr + v.size; }

template <typename T, typename... Args> T &append(FixedVector<T> &v, Args &&...args) {
    SDL_assert(v.size < v.capacity);
    T *p = v.ptr + v.size;
    new (p) T(std::forward<Args>(args)...);
    v.size += 1;
    return *p;
}
