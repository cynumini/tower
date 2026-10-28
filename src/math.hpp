#pragma once

#include <cmath>

// int vectors
using Vector2 = SDL_Point;
struct Vector3 {
    int x = 0, y = 0, z = 0;
};
struct Vector4 {
    int x = 0, y = 0, z = 0, w = 0;
};
// float vectors
using FVector2 = SDL_FPoint;
struct FVector3 {
    float x = 0, y = 0, z = 0;
};
struct FVector4 {
    float x = 0, y = 0, z = 0, w = 0;
};
// rects
using Rect = SDL_Rect;
using FRect = SDL_FRect;

// Vector2
constexpr Vector2 toVector2(FVector2 v) { return {.x = int(v.x), .y = int(v.y)}; }
constexpr Vector2 operator+(const Vector2 &a, const Vector2 &b) {
    return {.x = a.x + b.x, .y = a.y + b.y};
}
constexpr Vector2 operator-(const Vector2 &a, const Vector2 &b) {
    return {.x = a.x - b.x, .y = a.y - b.y};
}
constexpr Vector2 operator*(const Vector2 &a, const Vector2 &b) {
    return {.x = a.x * b.x, .y = a.y * b.y};
}
constexpr Vector2 operator/(const Vector2 &a, const Vector2 &b) {
    return {.x = a.x / b.x, .y = a.y / b.y};
}
constexpr bool operator==(const Vector2 &a, const Vector2 &b) {
    return a.x == b.x and a.y == b.y;
}
constexpr Vector2 operator*(const Vector2 &a, int b) { return {.x = a.x * b, .y = a.y * b}; }
// Vector3
constexpr Vector3 toVector3(const Vector2 &v) { return {.x = v.x, .y = v.y, .z = 0}; }
constexpr Vector3 toVector3(const FVector3 v) {
    return {.x = int(v.x), .y = int(v.y), .z = int(v.z)};
}
constexpr Vector3 operator+(const Vector3 &a, const Vector3 &b) {
    return {.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}
constexpr Vector3 operator-(const Vector3 &a, const Vector3 &b) {
    return {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}
constexpr Vector3 operator*(const Vector3 &a, const Vector3 &b) {
    return {.x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z};
}
constexpr Vector3 operator/(const Vector3 &a, const Vector3 &b) {
    return {.x = a.x / b.x, .y = a.y / b.y, .z = a.z / b.z};
}
constexpr bool operator==(const Vector3 &a, const Vector3 &b) {
    return a.x == b.x and a.y == b.y and a.z == b.z;
}

// Vector4
// FVector2
constexpr float length(const FVector2 &v) { return std::sqrt(v.x * v.x + v.y * v.y); }
constexpr FVector2 normalize(FVector2 v) {
    auto l = length(v);
    if (l > 0) return {.x = v.x / l, .y = v.y / l};
    return {.x = v.x, .y = v.y};
}
constexpr FVector2 size(FRect rect) { return {.x = rect.w, .y = rect.h}; }
constexpr FVector2 rotate(const FVector2 &v, float rad) {
    float c = std::cos(rad);
    float s = std::sin(rad);
    return {.x = v.x * c - v.y * s, .y = v.x * s + v.y * c};
}
constexpr FVector2 toFVector2(Vector2 v) { return {.x = float(v.x), .y = float(v.y)}; }
constexpr FVector2 toFVector2(Vector3 v) { return {.x = float(v.x), .y = float(v.y)}; }
constexpr FVector2 toFVector2(FVector3 v) { return {.x = v.x, .y = v.y}; }
constexpr FVector2 operator-(const FVector2 &v) { return {.x = -v.x, .y = -v.y}; }
constexpr FVector2 operator+(const FVector2 &self, const FVector2 &other) {
    return {.x = self.x + other.x, .y = self.y + other.y};
}
constexpr FVector2 operator*(const Vector2 &self, const FVector2 &other) {
    return {.x = float(self.x) * other.x, .y = float(self.y) * other.y};
}
constexpr FVector2 operator*(const FVector2 &self, const Vector2 &other) {
    return {.x = self.x * float(other.x), .y = self.y * float(other.y)};
}
constexpr FVector2 operator*(const Vector2 &a, const float b) {
    return {.x = a.x * b, .y = a.y * b};
}
constexpr FVector2 operator*(const FVector2 &a, const float b) {
    return {.x = a.x * b, .y = a.y * b};
}
constexpr FVector2 operator*(const FVector2 &a, const int b) {
    return {.x = a.x * float(b), .y = a.y * float(b)};
}
constexpr FVector2 operator-(const FVector2 &self, const FVector2 &other) {
    return {.x = self.x - other.x, .y = self.y - other.y};
}
constexpr FVector2 operator/(const FVector2 &a, const float b) {
    return {.x = a.x / b, .y = a.y / b};
}
constexpr FVector2 operator*(const FVector2 &a, const FVector2 &b) {
    return {.x = a.x * b.x, .y = a.y * b.y};
}
constexpr FVector2 operator/(const FVector2 &a, const FVector2 &b) {
    return {.x = a.x / b.x, .y = a.y / b.y};
}
constexpr FVector2 operator-(const Vector2 &a, const FVector2 &b) {
    return {.x = float(a.x) - b.x, .y = float(a.y) - b.y};
}
constexpr FVector2 &operator+=(FVector2 &self, const FVector2 &other) {
    self.x += other.x;
    self.y += other.y;
    return self;
}

constexpr float dot(const FVector2 &a, const FVector2 &b) { return (a.x * b.x) + (a.y * b.y); }

// FVector3
constexpr FVector3 rotateZ(const FVector3 &v, float rad) {
    const float c = std::cos(rad);
    const float s = std::sin(rad);
    return {.x = v.x * c - v.y * s, .y = v.x * s + v.y * c, .z = v.z};
}
constexpr FVector3 operator+(const Vector2 &a, const Vector3 &b) {
    return {.x = float(a.x) + float(b.x), .y = float(a.y) + float(b.y), .z = float(b.z)};
}
constexpr FVector3 operator+(const FVector3 &a, const Vector3 &b) {
    return {.x = a.x + float(b.x), .y = a.y + float(b.y), .z = a.z + float(b.z)};
}
constexpr FVector3 operator-(const FVector3 &self) {
    return {.x = -self.x, .y = -self.y, .z = -self.z};
}
constexpr FVector3 &operator+=(FVector3 &self, const FVector3 &other) {
    self.x += other.x;
    self.y += other.y;
    self.z += other.z;
    return self;
}
constexpr FVector3 operator-(const FVector3 &self, const FVector3 &other) {
    return {.x = self.x - other.x, .y = self.y - other.y, .z = self.z - other.z};
}
constexpr FVector3 operator+(const FVector3 &self, const FVector3 &other) {
    return {.x = self.x + other.x, .y = self.y + other.y, .z = self.z + other.z};
}
constexpr float length(const FVector3 &v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
constexpr FVector3 normalize(FVector3 v) {
    auto l = length(v);
    if (l > 0) return {.x = v.x / l, .y = v.y / l, .z = v.z / l};
    return {.x = v.x, .y = v.y, .z = v.z};
}
constexpr FVector3 floor(const FVector3 &v) {
    return {.x = std::floor(v.x), .y = std::floor(v.y), .z = std::floor(v.z)};
}
constexpr FVector3 operator*(const Vector3 &a, const float b) {
    return {.x = a.x * b, .y = a.y * b, .z = a.z * b};
}
constexpr FVector3 operator*(const FVector3 &a, int b) {
    return {.x = a.x * float(b), .y = a.y * float(b), .z = a.z * float(b)};
}
constexpr FVector3 &operator+=(FVector3 &self, const FVector2 &other) {
    self.x += other.x;
    self.y += other.y;
    return self;
}

constexpr FVector3 operator+(const FVector3 &a, const FVector2 &b) {
    return {.x = a.x + b.x, .y = a.y + b.y, .z = a.z};
}
constexpr FVector3 toFVector3(FVector4 v) { return {.x = v.x, .y = v.y, .z = v.z}; }
constexpr FVector3 toFVector3(FVector2 v) { return {.x = v.x, .y = v.y, .z = 0.F}; }
constexpr FVector3 toFVector3(Vector3 v) {
    return {.x = float(v.x), .y = float(v.y), .z = float(v.z)};
}
// FVector4

// Vector4

// Rects

constexpr bool checkCollisionAABB(FRect a, FRect b) {
    return std::abs(a.x - b.x) * 2.0F < a.w + b.w && std::abs(a.y - b.y) * 2.0F < a.h + b.h;
}

constexpr bool checkCollisionPointRect(FVector2 p, FRect r) {
    return std::abs(p.x - r.x) * 2.0F < r.w && std::abs(p.y - r.y) * 2.0F < r.h;
}

constexpr std::array<FVector2, 4> calcRectPoints(FRect rect, float angle) {
    auto half = FVector2{rect.w, rect.h} * 0.5F;
    FVector2 center = {rect.x, rect.y};

    std::array<FVector2, 4> points = {-half, {half.x, -half.y}, half, {-half.x, half.y}};

    float c = std::cos(angle);
    float s = std::sin(angle);

    for (std::size_t i = 0; i < 4; i++) {
        float x = points[i].x;
        float y = points[i].y;

        points[i] = FVector2{(x * c) - (y * s), (x * s) + (y * c)} + center;
    }

    return points;
}

template <typename T, std::size_t N> float max(const std::array<T, N> &values) {
    static_assert(N > 0);

    float best = values[0];
    for (const auto &value : values) best = value > best ? value : best;

    return best;
}

template <typename T, std::size_t N> T min(const std::array<T, N> &values) {
    static_assert(N > 0);

    T best = values[0];
    for (const auto &value : values) best = value < best ? value : best;

    return best;
}

constexpr bool checkCollisionSAT(FRect a, float a_angle, FRect b, float b_angle) {
    auto a_points = calcRectPoints(a, a_angle);
    auto b_points = calcRectPoints(b, b_angle);

    std::array<FVector2, 4> axes = {a_points[1] - a_points[0], a_points[2] - a_points[1],
                                    b_points[1] - b_points[0], b_points[2] - b_points[1]};

    for (std::size_t i = 0; i < 4; i++) {
        std::array<float, 4> a_values = {dot(a_points[0], axes[i]), dot(a_points[1], axes[i]),
                                         dot(a_points[2], axes[i]), dot(a_points[3], axes[i])};

        std::array<float, 4> b_values = {dot(b_points[0], axes[i]), dot(b_points[1], axes[i]),
                                         dot(b_points[2], axes[i]), dot(b_points[3], axes[i])};

        if (max(a_values) < min(b_values) || max(b_values) < min(a_values)) return false;
    }

    return true;
}

// Rect

// FRect
constexpr FRect toFRect(FVector2 pos, FVector2 size) {
    return {.x = pos.x, .y = pos.y, .w = size.x, .h = size.y};
}
constexpr FRect &operator/=(FRect &a, float b) {
    a.x /= b;
    a.y /= b;
    a.w /= b;
    a.h /= b;
    return a;
}

// Angles
constexpr float deg2rad(float degrees) { return degrees * (M_PIf / 180.0F); }
constexpr float rad2deg(float radians) { return radians * (180.0F / M_PIf); }

// Mat4
struct Mat4 {
    std::array<std::array<float, 4>, 4> m;

    constexpr Mat4 operator*(const Mat4 &other) const {
        Mat4 r = {};
        for (size_t y = 0; y < 4; y++)
            for (size_t x = 0; x < 4; x++)
                for (size_t i = 0; i < 4; i++) r.m[y][x] += m[y][i] * other.m[i][x];
        return r;
    }

    constexpr FVector4 operator*(const FVector4 &v) const {
        return {.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
                .y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
                .z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
                .w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w};
    }

    static constexpr Mat4 scale(FVector3 v) {
        return {.m = {{std::array<float, 4>{v.x, 0.0F, 0.0F, 0.0F},
                       std::array<float, 4>{0.0F, v.y, 0.0F, 0.0F},
                       std::array<float, 4>{0.0F, 0.0F, v.z, 0.0F},
                       std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}}}};
    }

    static constexpr Mat4 translation(FVector3 v) {
        return {.m = {{std::array<float, 4>{1.0F, 0.0F, 0.0F, v.x},
                       std::array<float, 4>{0.0F, 1.0F, 0.0F, v.y},
                       std::array<float, 4>{0.0F, 0.0F, 1.0F, v.z},
                       std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}}}};
    }

    static constexpr Mat4 ortho(float left, float right, float bottom, float top, float near,
                                float far) {
        float rl = right - left;
        float tb = top - bottom;
        float fn = far - near;

        return {.m = {{std::array<float, 4>{2.0F / rl, 0.0F, 0.0F, -((right + left) / rl)},
                       std::array<float, 4>{0.0F, 2.0F / tb, 0.0F, -((top + bottom) / tb)},
                       std::array<float, 4>{0.0F, 0.0F, 1.0F / fn, -(near / fn)},
                       std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}}}};
    }

    constexpr static FVector4 unprojectOrtho(FVector3 ndc, float left, float right, float bottom,
                                             float top, float near, float far) {
        return {left + (ndc.x + 1.0F) * 0.5F * (right - left),
                bottom + (ndc.y + 1.0F) * 0.5F * (top - bottom), near + ndc.z * (far - near), 1};
    }

    // Pitch
    static constexpr Mat4 rotationX(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);

        return {.m = {{std::array<float, 4>{1.0F, 0.0F, 0.0F, 0.0F},
                       std::array<float, 4>{0.0F, c, s, 0.0F},
                       std::array<float, 4>{0.0F, -s, c, 0.0F},
                       std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}}}};
    }

    static constexpr Mat4 iso45() {
        constexpr float s = 0.7071067811865475F;

        return {.m = {{std::array<float, 4>{1.0F, 0.0F, 0.0F, 0.0F},
                       std::array<float, 4>{0.0F, 1.0F, 1.0F, 0.0F},
                       std::array<float, 4>{0.0F, -s, s, 0.0F},
                       std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}}}};
    }

    static constexpr Mat4 inverseIso45() {
        constexpr float s = 0.7071067811865475F;

        return {.m = {{std::array<float, 4>{1.0F, 0.0F, 0.0F, 0.0F},
                       std::array<float, 4>{0.0F, 0.5F, -s, 0.0F},
                       std::array<float, 4>{0.0F, 0.5F, s, 0.0F},
                       std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}}}};
    }

    // Roll
    static constexpr Mat4 rotationY(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);

        return {.m = {{std::array<float, 4>{c, 0.0F, -s, 0.0F},
                       std::array<float, 4>{0.0F, 1.0F, 0.0F, 0.0F},
                       std::array<float, 4>{s, 0.0F, c, 0.0F},
                       std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}}}};
    }

    // Yaw
    static constexpr Mat4 rotationZ(float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);

        return {.m = {{std::array<float, 4>{c, s, 0.0F, 0.0F},
                       std::array<float, 4>{-s, c, 0.0F, 0.0F},
                       std::array<float, 4>{0.0F, 0.0F, 1.0F, 0.0F},
                       std::array<float, 4>{0.0F, 0.0F, 0.0F, 1.0F}}}};
    }
};
