#ifndef MATH_HPP
#define MATH_HPP

#include <root.hpp>

struct Vector2 {
    f32 x{};
    f32 y{};

    constexpr Vector2 add(Vector2 other) const { return {x + other.x, y + other.y}; }
};

struct Rectangle {
    Vector2 position{};
    Vector2 size{};
};

struct Matrix4 {
    float v[4][4]{};

    static constexpr Matrix4 identity() {
        Matrix4 m{};
        m.v[0][0] = 1.0f;
        m.v[1][1] = 1.0f;
        m.v[2][2] = 1.0f;
        m.v[3][3] = 1.0f;
        return m;
    }

    static constexpr Matrix4 orthographic(float left, float right, float bottom, float top, float near, float far) {
        Matrix4 m{};
        float rl = right - left;
        float tb = top - bottom;
        float fn = far - near;
        m.v[0][0] = 2.f / rl;
        m.v[1][1] = 2.f / tb;
        m.v[2][2] = 2.f / fn;
        m.v[3][0] = -((right + left) / (rl));
        m.v[3][1] = -((top + bottom) / (tb));
        m.v[3][2] = -((far + near) / (fn));
        m.v[3][3] = 1;
        return m;
    }
};

#endif // MATH_HPP
