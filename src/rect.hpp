#pragma once

#include <cmath>

#include <glm/glm.hpp>

constexpr glm::vec2 fRectSize(SDL_FRect rect) { return {rect.w, rect.h}; }
constexpr glm::vec2 fRectPos(SDL_FRect rect) { return {rect.x, rect.y}; }

constexpr bool checkCollisionAABB(SDL_FRect a, SDL_FRect b) {
    return std::abs(a.x - b.x) * 2.0F < a.w + b.w && std::abs(a.y - b.y) * 2.0F < a.h + b.h;
}

constexpr bool checkCollisionPointRect(glm::vec2 p, SDL_FRect r) {
    return std::abs(p.x - r.x) * 2.0F < r.w && std::abs(p.y - r.y) * 2.0F < r.h;
}

constexpr std::array<glm::vec2, 4> calcRectPoints(SDL_FRect rect, float angle) {
    auto half = glm::vec2{rect.w, rect.h} * 0.5F;
    glm::vec2 center = {rect.x, rect.y};

    std::array<glm::vec2, 4> points = {-half, {half.x, -half.y}, half, {-half.x, half.y}};

    float c = std::cos(angle);
    float s = std::sin(angle);

    for (std::size_t i = 0; i < 4; i++) {
        float x = points[i].x;
        float y = points[i].y;

        points[i] = glm::vec2{(x * c) - (y * s), (x * s) + (y * c)} + center;
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

constexpr bool checkCollisionSAT(SDL_FRect a, float a_angle, SDL_FRect b, float b_angle) {
    auto a_points = calcRectPoints(a, a_angle);
    auto b_points = calcRectPoints(b, b_angle);

    std::array<glm::vec2, 4> axes = {a_points[1] - a_points[0], a_points[2] - a_points[1],
                                     b_points[1] - b_points[0], b_points[2] - b_points[1]};

    for (std::size_t i = 0; i < 4; i++) {
        std::array<float, 4> a_values = {
            glm::dot(a_points[0], axes[i]), glm::dot(a_points[1], axes[i]),
            glm::dot(a_points[2], axes[i]), glm::dot(a_points[3], axes[i])};

        std::array<float, 4> b_values = {
            glm::dot(b_points[0], axes[i]), glm::dot(b_points[1], axes[i]),
            glm::dot(b_points[2], axes[i]), glm::dot(b_points[3], axes[i])};

        if (max(a_values) < min(b_values) || max(b_values) < min(a_values)) return false;
    }

    return true;
}

// Rect

// SDL_FRect
constexpr SDL_FRect toFRect(glm::vec2 pos, glm::vec2 size) {
    return {.x = pos.x, .y = pos.y, .w = size.x, .h = size.y};
}
constexpr SDL_FRect &operator/=(SDL_FRect &a, float b) {
    a.x /= b;
    a.y /= b;
    a.w /= b;
    a.h /= b;
    return a;
}
