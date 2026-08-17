#pragma once

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include <SDL3/SDL.h>

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

struct Vector2 {
    f32 x = 0;
    f32 y = 0;

    Vector2 operator*(Vector2 other) const { return {x * other.x, y * other.y}; }
    Vector2 operator+(Vector2 other) const { return {x + other.x, y + other.y}; }
    Vector2 operator-(Vector2 other) const { return {x - other.x, y - other.y}; }
    Vector2 operator*(f32 n) const { return {x * n, y * n}; }

    void operator+=(Vector2 other) { x += other.x, y += other.y; }
    void operator/=(f32 value) { x /= value, y /= value; }

    Vector2 normalize() const;
};

struct Vector3 {
    f32 x = 0;
    f32 y = 0;
    f32 z = 0;
};

struct Rectangle {
    f32 x = 0;
    f32 y = 0;
    f32 w = 0;
    f32 h = 0;

    Rectangle() {}
    Rectangle(f32 x, f32 y, f32 w, f32 h) : x(x), y(y), w(w), h(h) {}
    Rectangle(Vector2 position, Vector2 size) : x(position.x), y(position.y), w(size.x), h(size.y) {}
    Rectangle operator+(Vector2 other) { return {x += other.x, y += other.y, w, h}; }

    Rectangle scale(Vector2 other) const { return {x * other.x, y * other.y, w * other.x, h * other.y}; }
    Vector2 center() const { return {x + w / 2, y + h / 2}; }
    Vector2 position() const { return {x, y}; };
    Vector2 size() const { return {w, h}; };
    f32 x_max() const { return x + w; }
    f32 y_max() const { return y + h; }

    bool checkCollision(const Rectangle &other) const { return (x < other.x_max() and x_max() > other.x) and (y < other.y_max() and y_max() > other.y); }

    Vector2 overlapSize(const Rectangle &other) const;
};

struct Matrix4 {
    f32 v[4][4]{};

    constexpr static Matrix4 identity() {
        Matrix4 m{};
        m.v[0][0] = 1.0f;
        m.v[1][1] = 1.0f;
        m.v[2][2] = 1.0f;
        m.v[3][3] = 1.0f;
        return m;
    }

    constexpr static Matrix4 translation(f32 x, f32 y, f32 z) {
        auto m = Matrix4::identity();
        m.v[3][0] = x;
        m.v[3][1] = y;
        m.v[3][2] = z;
        return m;
    }

    constexpr static Matrix4 scaling(f32 x, f32 y, f32 z) {
        auto m = Matrix4::identity();
        m.v[0][0] = x;
        m.v[1][1] = y;
        m.v[2][2] = z;
        return m;
    }

    constexpr static Matrix4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
        f32 rl = right - left;
        f32 tb = top - bottom;
        f32 fn = far - near;
        auto m1 = Matrix4::scaling(2.f / rl, 2.f / tb, 2.f / fn);
        auto m2 = Matrix4::translation(-((right + left) / rl), -((top + bottom) / tb), -((far + near) / fn));
        return m2 * m1;
    }

    constexpr f32 (&operator[](usize i))[4] { return v[i]; }
    constexpr const f32 (&operator[](usize i) const)[4] { return v[i]; }

    constexpr Matrix4 operator*(const Matrix4 &other) const {
        Matrix4 result{};
        for (usize col = 0; col < 4; col++) {
            for (usize row = 0; row < 4; row++) {
                for (usize i = 0; i < 4; i++) {
                    result[col][row] += v[i][row] * other[col][i];
                }
            }
        }
        return result;
    }

    constexpr bool operator==(const Matrix4 &other) const {
        for (usize col = 0; col < 4; col++) {
            for (usize row = 0; row < 4; row++) {
                if (v[col][row] != other[col][row]) {
                    return false;
                }
            }
        }
        return true;
    }

    void print() {
        for (usize col = 0; col < 4; col++) {
            for (usize row = 0; row < 4; row++) {
                printf("%f, ", v[col][row]);
            }
        }
        printf("\n");
    }
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
constexpr Color GREEN = {0, 1, 0, 1};
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
    T *rawptr;
    usize len;

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
    void deinit() { delete[] rawptr; }
};

struct String {
    const char *c_str;
    usize len = 0;

    String(const char *c_str) : c_str(c_str) { len = strlen(c_str); }

    String(Slice<u8> slice) : c_str((char *)slice.rawptr), len(slice.len) {}

    const char *begin() const { return c_str; }

    const char *end() const { return c_str + len; }
};

#define SDL_ENSURE(check, message)                                                                                                                             \
    do {                                                                                                                                                       \
        if (!(check)) {                                                                                                                                        \
            SDL_Log("Couldn't %s: %s", (message), SDL_GetError());                                                                                             \
            abort();                                                                                                                                           \
        }                                                                                                                                                      \
    } while (false)

[[noreturn]]
void unreachable(const char *string, usize line);

#define UNREACHABLE() unreachable(__FILE__, __LINE__)

[[noreturn]]
void todo(const char *string, usize line, const char *message);

#define TODO(message) todo(__FILE__, __LINE__, message)

namespace OS {
/// Caller owns the memory
Slice<u8> readEntireFile(const String &name);
} // namespace OS
