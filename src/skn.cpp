#pragma once

#include <cmath>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>


typedef unsigned int uint;

#define KB(value) ((value) * 1024)
#define MB(value) (KB(value) * 1024)

// defer
template <typename F> struct privDefer {
    F f;
    privDefer(F f) : f(f) {}
    ~privDefer() noexcept { f(); }
};

template <typename F> privDefer<F> defer_func(F f) { return privDefer<F>(f); }

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code) auto DEFER_3(_defer_) = defer_func([&]() { code; })

#define TODO(text) __builtin_debugtrap()

// Slice
template <typename T, bool zero = false> struct OldSlice {
    size_t len;
    T *ptr;

    OldSlice() : len(0), ptr(0) {}

    OldSlice(size_t len, T *ptr) : len(len), ptr(ptr) {}

    OldSlice<const char>(const char *text) : len(strlen(text)), ptr(text) {}

    OldSlice<const char>(const OldSlice<char> &slice) : len(slice.len), ptr(slice.ptr) {}

    T &operator[](size_t index) {
        SDL_assert(index < len);
        return ptr[index];
    }

    const T &operator[](size_t index) const {
        SDL_assert(index < len);
        return ptr[index];
    }

    bool operator==(OldSlice other) const {
        if (len != other.len) return false;
        for (size_t i = 0; i < len; i++) {
            if (ptr[i] != other[i]) return false;
        }
        return true;
    }

    T *begin() { return ptr; }
    T *end() { return ptr + len; }

    OldSlice<T, false> withoutZero() {
        static_assert(zero);
        return {len, ptr};
    }

    OldSlice<T, false> withZero() {
        static_assert(zero);
        return {len + 1, ptr};
    }

    void sort(int (*sortFn)(const void *a, const void *b), size_t offset = 0) {
        SDL_assert(offset <= len);
        qsort(ptr + offset, len - offset, sizeof(T), sortFn);
    }

    static OldSlice<const char> fromStrZ(const char *str) { return {strlen(str), str}; }
};

template <typename T> using SliceZ = OldSlice<T, true>;

static std::string_view getStem(const char *str_z) {
    ptrdiff_t len = strlen(str_z);
    SDL_assert(len > 0);
    for (ptrdiff_t i = len - 1; i >= 0; i--) {
        if (str_z[i] == '.') return {str_z, size_t(i)};
    }
    return {str_z, size_t(len)};
}

// Array
template <typename T, size_t N> struct OldArray {
    T data[N];

    constexpr T &operator[](size_t index) {
        SDL_assert(index < len());
        return data[index];
    }

    const T &operator[](size_t index) const {
        SDL_assert(index < len());
        return data[index];
    }

    constexpr size_t len() const {
        return N;
    }

    T *begin() { return data; }
    T *end() { return data + len(); }
};

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

template <typename T> T *appendOld(T *data, size_t *len, size_t capacity, T value) {
    SDL_assert(*len < capacity);
    const size_t index = *len;
    data[(*len)++] = value;
    return &data[index];
}

template <typename T> void swapRemove(T *data, size_t *len, size_t index) {
    SDL_assert(index < *len);
    data[index] = data[--(*len)];
}

template <typename T> T swapPop(T *data, size_t *len, size_t index) {
    SDL_assert(index < *len);
    T removed = data[index];
    data[index] = data[--(*len)];
    return removed;
}

// Fixed
template <typename T> struct Fixed : public OldSlice<T> {
    size_t capacity;

    static Fixed create(OldSlice<T> slice) { return {{0, slice.ptr}, slice.len}; }

    void init(OldSlice<T> slice) {
        this->ptr = slice.ptr;
        this->len = 0;
        capacity = slice.len;
    }

    T *append(T value) { return ::appendOld(this->ptr, &this->len, capacity, value); }
    void swapRemove(size_t index) { ::swapRemove(this->ptr, &this->len, index); }
    T swapPop(size_t index) { return ::swapPop(this->ptr, &this->len, index); }
};

// FixedStack
template <typename T, size_t N> struct FixedStack : OldArray<T, N> {
    static_assert(N > 0);
    size_t len;
    size_t next;

    void push(T value) {
        this->data[next] = value;
        next = (next + 1) % N;
        if (len < N) len++;
    }

    T pop() {
        SDL_assert(len > 0);
        len--;
        next = (N + next - 1) % N;
        return this->data[next];
    }

    T peek() {
        SDL_assert(len > 0);
        auto index = (N + next - 1) % N;
        return this->data[index];
    }

    void reset() {
        len = 0;
        next = 0;
    }
};
