#include <sakana/sakana.hpp>

typedef SDL_FRect Rect;

// vec2
static vec2 operator*(vec2 a, vec2 b) { return {a.x * b.x, a.y * b.y}; }
static vec2 operator*(vec2 self, f32 other) { return {self.x * other, self.y * other}; }
static vec2 operator+(vec2 a, vec2 b) { return {a.x + b.x, a.y + b.y}; }
static vec2 operator-(vec2 a) { return {-a.x, -a.y}; }
static vec2 operator-(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
static void operator+=(vec2 &self, vec2 other) { self.x += other.x, self.y += other.y; }

static f32 vec2Length(vec2 self) { return SDL_sqrtf((self.x * self.x) + (self.y * self.y)); };
static f32 vec2Dot(vec2 a, vec2 b) { return (a.x * b.x) + (a.y * b.y); }

static vec2 normalizeVec2(vec2 self) {
    auto length = vec2Length(self);
    if (length > 0) {
        return {self.x / length, self.y / length};
    }
    return self;
};

// Rect
static void operator/=(Rect &self, vec2 other) {
    self.x /= other.x;
    self.y /= other.y;
    self.w /= other.x;
    self.h /= other.y;
}

static Rect rectFromVec2(vec2 position, vec2 size) {
    return {position.x, position.y, size.x, size.y};
}

static bool checkCollisionAABB(Rect a, Rect b) {
    return a.x < (b.x + b.w) and b.x < (a.x + a.w) and a.y < (b.y + b.h) and b.y < (a.y + a.h);
};

union Points4 {
    vec2 v[4];
    struct {
        vec2 p0;
        vec2 p1;
        vec2 p2;
        vec2 p3;
    };
};

static Points4 calcRectPoints(Rect rect, f32 angle) {
    f32 half_w = rect.w / 2.0F;
    f32 half_h = rect.h / 2.0F;
    Points4 points = {{
        {-half_w, -half_h},
        {half_w, -half_h},
        {half_w, half_h},
        {-half_w, half_h},
    }};
    for (usize i = 0; i < 4; i++) {
        points.v[i] = {(points.v[i].x * SDL_cosf(angle)) - (points.v[i].y * SDL_sinf(angle)),
                       (points.v[i].x * SDL_sinf(angle)) + (points.v[i].y * SDL_cosf(angle))};
        points.v[i] += vec2{rect.x, rect.y};
    }
    return points;
}

static f32 max(const f32 values[4]) {
    f32 value = values[0];
    for (usize i = 1; i < 4; i++) value = values[i] > value ? values[i] : value;
    return value;
}

static f32 min(const f32 values[4]) {
    f32 value = values[0];
    for (usize i = 1; i < 4; i++) value = values[i] < value ? values[i] : value;
    return value;
}

static bool checkCollisionSAT(Rect a, f32 a_angle, Rect b, f32 b_angle) {
    auto a_points = calcRectPoints(a, a_angle);
    auto b_points = calcRectPoints(b, b_angle);
    const vec2 axes[4] = {a_points.p1 - a_points.p0, a_points.p2 - a_points.p1,
                          b_points.p1 - b_points.p0, b_points.p2 - b_points.p1};

    for (usize i = 0; i < 4; i++) {
        f32 a_values[4] = {vec2Dot(a_points.p0, axes[i]), vec2Dot(a_points.p1, axes[i]),
                           vec2Dot(a_points.p2, axes[i]), vec2Dot(a_points.p3, axes[i])};

        f32 b_values[4] = {vec2Dot(b_points.p0, axes[i]), vec2Dot(b_points.p1, axes[i]),
                           vec2Dot(b_points.p2, axes[i]), vec2Dot(b_points.p3, axes[i])};

        if (max(a_values) < min(b_values) or max(b_values) < min(a_values)) return false;
    }

    return true;
}
