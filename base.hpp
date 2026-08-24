#pragma once

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include <SDL3/SDL.h>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

#include <sakana/builtin.hpp>

namespace skn = sakana;

using f32 = float;
using f64 = double;
using s16 = int16_t;
using s32 = int32_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u8 = uint8_t;
using usize = size_t;
using isize = ssize_t;

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

struct Rectangle {
    f32 x = 0;
    f32 y = 0;
    f32 w = 0;
    f32 h = 0;

    Rectangle() {}
    Rectangle(f32 x, f32 y, f32 w, f32 h) : x(x), y(y), w(w), h(h) {}
    Rectangle(glm::vec2 position, glm::vec2 size)
        : x(position.x), y(position.y), w(size.x), h(size.y) {}
    Rectangle operator+(glm::vec2 other) { return {x += other.x, y += other.y, w, h}; }

    Rectangle scale(glm::vec2 other) const {
        return {x * other.x, y * other.y, w * other.x, h * other.y};
    }
    glm::vec2 center() const { return {x + w / 2, y + h / 2}; }
    glm::vec2 position() const { return {x, y}; };
    glm::vec2 size() const { return {w, h}; };
    f32 x_max() const { return x + w; }
    f32 y_max() const { return y + h; }

    bool checkCollision(const Rectangle &other) const {
        return (x < other.x_max() and x_max() > other.x) and
               (y < other.y_max() and y_max() > other.y);
    }

    glm::vec2 overlapSize(const Rectangle &other) const;
};

struct Color {
    f32 r = 0;
    f32 g = 0;
    f32 b = 0;
    f32 a = 0;
};

constexpr Color BLACK = {0, 0, 0, 1};
constexpr Color WHITE = {1, 1, 1, 1};
constexpr Color GRAY = {0.5, 0.5, 0.5, 1};

constexpr Color RED = {1, 0, 0, 1};
constexpr Color YELLOW = {1, 1, 0, 1};
constexpr Color MAGENTA = {1, 0, 1, 1};
constexpr Color GREEN = {0, 1, 0, 1};
constexpr Color CYAN = {0, 1, 1, 1};
constexpr Color BLUE = {0, 0, 1, 1};

template <typename T, usize N> struct Array {
    T data[N]{};

    constexpr T &operator[](usize i) { return data[i]; }
    constexpr const T &operator[](usize i) const { return data[i]; }

    constexpr T *begin() { return data; }
    constexpr T *end() { return data + N; }
};

template <typename T, usize N> struct FixedArray {
    T data[N]{};
    usize len = 0;

    T &operator[](usize i) { return data[i]; }
    const T &operator[](usize i) const { return data[i]; }

    T *begin() { return data; }
    T *end() { return data + len; }

    usize add(const T &value) {
        assert(len < N);
        auto index = len;
        data[index] = value;
        len++;
        return index;
    }

    T &last() {
        assert(len > 0);
        return data[len - 1];
    }

    void clear() { len = 0; }

    template <typename F> void sort(F compar) {
        for (usize i = 1; i < len; i++) {
            T key = data[i];
            auto j = (isize)i - 1;
            while (j >= 0 and compar(data[j], key)) {
                data[j + 1] = data[j];
                j--;
            }
            data[j + 1] = key;
        }
    }
};

template <typename T, usize N> struct IndexedArray {
    T data[N + 1]{};
    usize len = 0;

    T &operator[](usize i) {
        assert(i > 0);
        return data[i];
    }
    const T &operator[](usize i) const {
        assert(i > 0);
        return data[i];
    }

    T *begin() { return data + 1; }
    T *end() { return data + len + 1; }

    usize add(const T &value) {
        assert(len < N);
        len++;
        data[len] = value;
        return len;
    }

    void clear() { len = 0; }
};

template <typename T> struct Slice {
    T *rawptr = nullptr;
    usize len = 0;

    Slice() {}
    Slice(T *rawptr, usize len) : rawptr(rawptr), len(len) {}

    T &operator[](usize i) {
        assert(i < len);
        return rawptr[i];
    }

    const T &operator[](usize i) const {
        assert(i < len);
        return rawptr[i];
    }

    Slice slice(usize start) { return {rawptr + start, len - start}; }

    Slice slice(usize start, usize end) { return {rawptr + start, end - start}; }
};

struct Location {
    const char *file;
    usize line;

    static Location current(const char *file = __builtin_FILE(), usize line = __builtin_LINE()) {
        return {file, line};
    }
};

struct Allocator {
    virtual void *alloc(usize size, Location location = {}) = 0;
    virtual void *realloc(void *ptr, usize size, Location location = {}) = 0;
    virtual void free(void *ptr) = 0;
};

namespace mem {
template <typename T> T *create(Allocator &gpa, Location location = Location::current()) {
    return (T *)gpa.alloc(sizeof(T), location);
}
#define MEM_INIT(T, gpa, ...) mem::init<T>(gpa, Location::current(), __VA_ARGS__)
template <typename T, typename... Args> T *init(Allocator &gpa, Location loc, Args &&...args) {
    auto object = (T *)gpa.alloc(sizeof(T), loc);
    new (object) T(args...);
    return object;
}
template <typename T> void deinit(Allocator &gpa, T *ptr) {
    ptr->~T();
    gpa.free(ptr);
}
template <typename T>
Slice<T> alloc(Allocator &gpa, usize len, Location location = Location::current()) {
    return {(T *)gpa.alloc(sizeof(T) * len, location), len};
}
template <typename T>
Slice<T> realloc(Allocator &gpa, Slice<T> old, usize len, Location location = Location::current()) {
    return {(T *)gpa.realloc(old.rawptr, sizeof(T) * len, location), len};
}
template <typename T> void free(Allocator &gpa, Slice<T> slice) {
    return gpa.free((void *)slice.rawptr);
}

} // namespace mem

struct String {
    // TODO: rename to data
    Slice<const char> c_str{};

    String() {};
    String(Slice<u8> slice) : c_str((char *)slice.rawptr, slice.len) {}
    String(Slice<const char> slice) : c_str(slice) {}
    String(Slice<char> slice) : c_str(slice.rawptr, slice.len) {}
    String(const char *c_str, usize len) : c_str({c_str, len}) {}
    static String fromCStr(const char *str) { return {str, strlen(str)}; }
    template <size_t N> String(const char (&c_str)[N]) : c_str(c_str, N - 1) {}

    bool operator==(String other) const {
        if (c_str.len != other.c_str.len) {
            return false;
        }
        for (usize i = 0; i < c_str.len; i++) {
            if (c_str[i] != other.c_str[i]) return false;
        }
        return true;
    }

    const char *begin() const { return c_str.rawptr; }
    const char *end() const { return c_str.rawptr + c_str.len; }

    template <typename T> T parseNumber() {
        f64 result = 0.0f;
        f64 fraction = 0.1f;
        bool decimal = false;
        bool negative = false;

        for (usize i = 0; i < this->c_str.len; i++) {
            auto c = c_str[i];

            if (c == '-') {
                negative = true;
                continue;
            }

            if (c == '.') {
                decimal = true;
                continue;
            }

            assert(c >= '0' and c <= '9');

            f64 digit = (f64)(c - '0');

            if (!decimal) {
                result = result * 10 + digit;
            } else {
                result = digit * fraction;
                fraction *= 0.1;
            }
        }

        return (T)(negative ? -result : result);
    }

    String dupe(Allocator &gpa, Location location = Location::current()) {
        auto data = mem::alloc<char>(gpa, c_str.len, location);
        memcpy(data.rawptr, c_str.rawptr, c_str.len);
        return data;
    }

    void deinit(Allocator &gpa) { mem::free(gpa, c_str); }
};

template <typename T> struct Optional {
    bool has_value = false;
    T payload;

    Optional() : has_value(false) {}
    Optional(T payload) : has_value(true), payload(payload) {}
};

#define SDL_ENSURE(check, message)                                                                 \
    do {                                                                                           \
        if (!(check)) {                                                                            \
            SDL_Log("Couldn't %s: %s", (message), SDL_GetError());                                 \
            abort();                                                                               \
        }                                                                                          \
    } while (false)

struct CAllocator : Allocator {
    void *alloc(usize size, [[maybe_unused]] Location location = Location::current()) {
        return ::malloc(size);
    }
    void *realloc(void *ptr, usize size, [[maybe_unused]] Location location = Location::current()) {
        return ::realloc(ptr, size);
    }
    void free(void *ptr) { ::free(ptr); }
};

template <typename T> struct DynamicArray {
    Slice<T> data{};
    usize len = 0;
    usize capacity = 0;

    T &operator[](usize i) {
        assert(i < len);
        return data[i];
    }
    const T &operator[](usize i) const {
        assert(i < len);
        return data[i];
    }

    T *begin() { return data.rawptr; }
    T *end() { return data.rawptr + len; }

    usize add(Allocator &gpa, const T &value, Location location = Location::current()) {
        if (len == capacity) {
            capacity = capacity == 0 ? 2 : capacity * 2;
            data = mem::realloc<T>(gpa, data, capacity, location);
        }
        auto index = len;
        data[index] = value;
        len++;
        return index;
    }

    T &last() {
        assert(len > 0);
        return data[len - 1];
    }

    void clear() { len = 0; }

    void deinit(Allocator &gpa) { mem::free(gpa, data); }

    template <typename F> void sort(F compar) {
        for (usize i = 1; i < len; i++) {
            T key = data[i];
            auto j = (isize)i - 1;
            while (j >= 0 and compar(data[j], key)) {
                data[j + 1] = data[j];
                j--;
            }
            data[j + 1] = key;
        }
    }
};

struct DebugAllocator : Allocator {
    struct Record {
        Location location;
        void *ptr;
        bool free;
    };

    Allocator &child_allocator;
    DynamicArray<Record> records;

    DebugAllocator(Allocator &child_allocator) : child_allocator(child_allocator) {};
    ~DebugAllocator() {
        for (auto record : records) {
            if (record.free == false) {
                printf("%s:%zu: allocation here\n", record.location.file, record.location.line);
            }
        }
        records.deinit(child_allocator);
    }

    void *alloc(usize size, Location location = Location::current()) {
        auto ptr = child_allocator.alloc(size, location);
        records.add(child_allocator, {location, ptr, false});
        return ptr;
    }

    void *realloc(void *ptr, usize size, Location location = Location::current()) {
        auto new_ptr = child_allocator.realloc(ptr, size, location);
        if (new_ptr != ptr) {
            for (auto &record : records) {
                if (record.ptr == ptr) {
                    record.free = true;
                }
            }
            records.add(child_allocator, {location, new_ptr, false});
        } else {
            for (auto &record : records) {
                if (record.ptr == ptr) {
                    record.location = location;
                }
            }
        }
        return new_ptr;
    }

    void free(void *ptr) {
        for (auto &record : records) {
            if (record.ptr == ptr) {
                record.free = true;
            }
        }
        return child_allocator.free(ptr);
    }
};

namespace OS {
/// Caller owns the memory
Slice<u8> readEntireFile(Allocator &gpa, const String &name,
                         Location location = Location::current());
} // namespace OS

inline usize FNV1aHash(const String &string) {
    constexpr usize fnv_prime = 1099511628211ull;
    constexpr usize fnv_offset_basis = 14695981039346656037ull;
    usize hash = fnv_offset_basis;
    for (auto c : string) {
        hash = hash xor (usize) c;
        hash = hash * fnv_prime;
    }
    return hash;
}

template <typename T, usize N> struct HashMap {
    struct Item {
        String key;
        T value;
    };

    struct Iterator {
        HashMap<T, N> *hash_map;
        usize index;

        Item &operator*() { return hash_map->data[index]; }
        bool operator!=(const Iterator &other) const { return index != other.index; }

        Iterator &operator++() {
            for (usize i = index + 1; i < N; i++) {
                if (hash_map->used[i]) {
                    index = i;
                    return *this;
                }
            }

            index = N;
            return *this;
        }
    };

    Item data[N]{};
    bool used[N]{};

    // T &operator[](usize i) {
    //     assert(i > 0);
    //     return data[i];
    // }

    // const T &operator[](usize i) const {
    //     assert(i > 0);
    //     return data[i];
    // }

    // return true on update
    bool put(const String &key, const T &value) {
        auto hash = FNV1aHash(key) % N;
        for (auto i = hash; i < N; i++) {
            if (used[i]) {
                if (data[i].key == key) {
                    data[i].value = value;
                    // update
                    return true;
                }
            } else {
                data[i] = {key, value};
                used[i] = true;
                // add
                return false;
            }
        }
        skn::unreachable();
    }

    Iterator begin() {
        for (usize i = 0; i < N; i++) {
            if (used[i]) {
                return Iterator{.hash_map = this, .index = i};
            }
        }
        return end();
    }

    Iterator end() { return Iterator{.hash_map = this, .index = N}; }

    const T &operator[](const String &key) const {
        auto hash = FNV1aHash(key) % N;
        for (auto i = hash; i < N; i++) {
            if (!used[i]) skn::panic("The key is not in hashmap");
            if (data[i].key == key) return data[i].value;
        }
        skn::panic("The key is not in hashmap");
    }
};
