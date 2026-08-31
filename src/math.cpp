#include <sakana/sakana.hpp>

typedef SDL_FRect Rect;

// vec2
static vec2 operator*(vec2 self, float other) { return {self.x * other, self.y * other}; }
static vec2 operator*(vec2 a, vec2 b) { return {a.x * b.x, a.y * b.y}; }
static vec2 operator-(vec2 a, vec2 b) { return {a.x - b.x, a.y - b.y}; }
static void operator+=(vec2 &self, vec2 other) {
    self.x += other.x;
    self.y += other.y;
}

static float vec2Length(vec2 self) { return SDL_sqrtf((self.x * self.x) + (self.y * self.y)); };

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

static bool checkCollisonAABB(Rect a, Rect b) {
    return a.x < (b.x + b.w) and b.x < (a.x + a.w) and a.y < (b.y + b.h) and b.y < (a.y + a.h);
};

static bool checkCollisonSAT(Rect a, float a_angle, Rect b, float b_angle) {
    vec2 a_points[4] = {
        {a.x, a.y},
        {a.x + a.w, a.y},
        {a.x + a.w, a.y + a.h},
        {a.x, a.y + a.h},

    };
    return false;
}
