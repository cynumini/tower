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
    Vector2 operator/(Vector2 other) const { return {x / other.x, y / other.y}; }
    Vector2 operator+(Vector2 other) const { return {x + other.x, y + other.y}; }
    Vector2 operator-(Vector2 other) const { return {x - other.x, y - other.y}; }
    Vector2 operator*(f32 n) const { return {x * n, y * n}; }
    Vector2 operator/(f32 n) const { return {x / n, y / n}; }

    void operator+=(Vector2 other) { x += other.x, y += other.y; }
    void operator/=(f32 value) { x /= value, y /= value; }

    Vector2 normalize() const;
};

struct Vector4;

struct Vector3 {
    f32 x = 0;
    f32 y = 0;
    f32 z = 0;

    Vector3() = default;
    Vector3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
    Vector3(Vector4 v);

    Vector3 operator*(f32 n) const { return {x * n, y * n, z * n}; }

    void operator+=(Vector3 other) { x += other.x, y += other.y, z += other.z; }
};

struct Vector4 {
    f32 x = 0;
    f32 y = 0;
    f32 z = 0;
    f32 w = 0;

    Vector4() {}
    Vector4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
    Vector4(Vector2 v, f32 z, f32 w) : x(v.x), y(v.y), z(z), w(w) {}
    Vector4(Vector3 v, f32 w) : x(v.x), y(v.y), z(v.z), w(w) {}

    constexpr f32 &operator[](usize i) { return (&x)[i]; }
    constexpr const f32 &operator[](usize i) const { return (&x)[i]; }
};

static inline f32 degToRad(f32 deg) { return f32(f64(deg) * M_PI / 180.0); }
static inline f32 radToDeg(f32 rad) { return f32(f64(rad) / M_PI * 180.0); }

struct Rectangle {
    f32 x = 0;
    f32 y = 0;
    f32 w = 0;
    f32 h = 0;

    Rectangle() {}
    Rectangle(f32 x, f32 y, f32 w, f32 h) : x(x), y(y), w(w), h(h) {}
    Rectangle(Vector2 position, Vector2 size)
        : x(position.x), y(position.y), w(size.x), h(size.y) {}
    Rectangle operator+(Vector2 other) { return {x += other.x, y += other.y, w, h}; }

    Rectangle scale(Vector2 other) const {
        return {x * other.x, y * other.y, w * other.x, h * other.y};
    }
    Vector2 center() const { return {x + w / 2, y + h / 2}; }
    Vector2 position() const { return {x, y}; };
    Vector2 size() const { return {w, h}; };
    f32 x_max() const { return x + w; }
    f32 y_max() const { return y + h; }

    bool checkCollision(const Rectangle &other) const {
        return (x < other.x_max() and x_max() > other.x) and
               (y < other.y_max() and y_max() > other.y);
    }

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

    constexpr static Matrix4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near,
                                          f32 far) {
        f32 rl = right - left;
        f32 tb = top - bottom;
        f32 fn = far - near;
        auto m1 = Matrix4::scaling(2.f / rl, 2.f / tb, 1 / fn);
        auto m2 =
            Matrix4::translation(-((right + left) / rl), -((top + bottom) / tb), -(near / fn));
        return m2 * m1;
    }

    constexpr static Vector4 orthographicUnproject(Vector4 ndc, f32 left, f32 right, f32 bottom,
                                                   f32 top, f32 near, f32 far) {
        return {(ndc.x * (right - left) + (right + left)) * 0.5f,
                (ndc.y * (top - bottom) + (top + bottom)) * 0.5f, ndc.z * (far - near) + near,
                1.0f};
    }

    constexpr static Matrix4 rotate(Vector3 axis, f32 angle) {
        auto result = Matrix4::identity();

        f32 x = axis.x, y = axis.y, z = axis.z;

        f32 length_squared = x * x + y * y + z * z;

        if ((length_squared != 1.0f) and (length_squared != 0.0f)) {
            f32 ilength = 1.0f / sqrtf(length_squared);
            x *= ilength;
            y *= ilength;
            z *= ilength;
        }

        f32 sin_result = sinf(angle);
        f32 cos_result = cosf(angle);
        f32 t = 1.0f - cos_result;

        result[0][0] = x * x * t + cos_result;
        result[0][1] = y * x * t + z * sin_result;
        result[0][2] = z * x * t - y * sin_result;

        result[1][0] = x * y * t - z * sin_result;
        result[1][1] = y * y * t + cos_result;
        result[1][2] = z * y * t + x * sin_result;

        result[2][0] = x * z * t + y * sin_result;
        result[2][1] = y * z * t - x * sin_result;
        result[2][2] = z * z * t + cos_result;

        return result;
    }

    static Matrix4 rotateXYZ(Vector3 angle) {
        auto result = Matrix4::identity();

        f32 cos_z = cosf(-angle.z);
        f32 sin_z = sinf(-angle.z);
        f32 cos_y = cosf(-angle.y);
        f32 sin_y = sinf(-angle.y);
        f32 cos_x = cosf(-angle.x);
        f32 sin_x = sinf(-angle.x);

        result[0][0] = cos_z * cos_y;
        result[0][1] = (cos_z * sin_y * sin_x) - (sin_z * cos_x);
        result[0][2] = (cos_z * sin_y * cos_x) + (sin_z * sin_x);

        result[1][0] = sin_z * cos_y;
        result[1][1] = (sin_z * sin_y * sin_x) + (cos_z * cos_x);
        result[1][2] = (sin_z * sin_y * cos_x) - (cos_z * sin_x);

        result[2][0] = -sin_y;
        result[2][1] = cos_y * sin_x;
        result[2][2] = cos_y * cos_x;

        return result;
    }

    static Matrix4 rotateZYX(Vector3 angle) {
        auto result = Matrix4::identity();

        f32 cos_z = cosf(angle.z);
        f32 sin_z = sinf(angle.z);
        f32 cos_y = cosf(angle.y);
        f32 sin_y = sinf(angle.y);
        f32 cos_x = cosf(angle.x);
        f32 sin_x = sinf(angle.x);

        result[0][0] = cos_z * cos_y;
        result[0][1] = cos_y * sin_z;
        result[0][2] = -sin_y;

        result[1][0] = (cos_z * sin_y * sin_x) - (cos_x * sin_z);
        result[1][1] = (sin_z * sin_y * sin_x) + (cos_z * cos_x);
        result[1][2] = cos_y * sin_x;

        result[2][0] = (cos_z * cos_x * sin_y) + (sin_z * sin_x);
        result[2][1] = (cos_x * sin_z * sin_y) - (cos_z * sin_x);
        result[2][2] = cos_y * cos_x;

        return result;
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

    constexpr Vector4 operator*(const Vector4 &other) const {
        Vector4 result{};

        for (usize row = 0; row < 4; row++) {
            for (usize col = 0; col < 4; col++) {
                result[row] += v[col][row] * other[col];
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
template <typename T>
Slice<T> alloc(Allocator &gpa, usize len, Location location = Location::current()) {
    return {(T *)gpa.alloc(sizeof(T) * len, location), len};
}
template <typename T>
Slice<T> realloc(Allocator &gpa, Slice<T> old, usize len, Location location = Location::current()) {
    return {(T *)gpa.realloc(old.rawptr, sizeof(T) * len, location), len};
}
template <typename T> void free(Allocator &gpa, Slice<T> slice) { return gpa.free((void *)slice.rawptr); }

} // namespace mem

struct String {
    // TODO: consider use slice
    Slice<const char> c_str{};

    String() {};
    String(Slice<u8> slice) : c_str((char *)slice.rawptr, slice.len) {}
    String(Slice<const char> slice) : c_str(slice) {}
    String(Slice<char> slice) : c_str(slice.rawptr, slice.len) {}
    String(const char *c_str, usize len) : c_str({c_str, len}) {}
    static String fromCStr(const char *str) { return {str, strlen(str)}; }
    template <size_t N> String(const char (&c_str)[N]) : c_str(c_str, N - 1) {}

    bool operator==(String other) const { return strcmp(c_str.rawptr, other.c_str.rawptr) == 0; }

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
                result += result * 10 + digit;
            } else {
                result += digit * fraction;
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

[[noreturn]]
void unreachable(const char *string, usize line);

#define UNREACHABLE() unreachable(__FILE__, __LINE__)

[[noreturn]]
void todo(const char *string, usize line, const char *message);

#define TODO(message) todo(__FILE__, __LINE__, message)

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
        UNREACHABLE();
    }
};

struct CSV {
    Slice<u8> buffer;
    bool header = false;
    usize position = 0;

    CSV(Allocator &gpa, String name, bool header = false, Location location = Location::current())
        : header(header) {
        buffer = OS::readEntireFile(gpa, name, location);
    }
    void deinit(Allocator &gpa) { mem::free(gpa, buffer); }

    // please deinit dynamic array
    Optional<DynamicArray<String>> readline(Allocator &gpa) {
        if (header) {
            while (buffer[position] != '\n' and position < buffer.len) {
                position += 1;
            }
            position += 1;
            header = false;
        }

        if (position >= buffer.len) {
            return {};
        }

        DynamicArray<String> result;

        usize start = position;
        while (position < buffer.len) {
            auto c = buffer[position];
            if (c == ',' or c == '\n') {
                result.add(gpa, {buffer.slice(start, position)});
                start = position + 1;
                if (c == '\n') {
                    position++;
                    break;
                }
            }
            position++;
        }

        if (position == buffer.len and start < position) {
            result.add(gpa, {buffer.slice(start, position)});
        }

        return {result};
    }
};
