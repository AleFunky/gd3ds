#pragma once

#include <filesystem>
#include <cstdint>
#include <functional>
#include <cmath>

namespace gd3ds
{

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

template <typename... args>
using Callback = std::function<void(args...)>;

struct PathHash {
    std::size_t operator()(const std::filesystem::path& p) const {
        return std::filesystem::hash_value(p);
    }
};

struct Color4F;

struct Color3B {
    u8 r, g, b;

    static Color3B fromColor4F(const Color4F& color);

    bool operator==(const Color3B& other) const = default;

    static const Color3B White;
    static const Color3B Black;
    static const Color3B Red;
    static const Color3B Green;
    static const Color3B Blue;
    static const Color3B Cyan;
    static const Color3B Magenta;
    static const Color3B Yellow;
    static const Color3B GoldenYellow;
    static const Color3B VividRed;
    static const Color3B VividBlue;
    static const Color3B BrightOrange;
    static const Color3B NeonGreen;
};

inline constexpr Color3B Color3B::White = {255, 255, 255};
inline constexpr Color3B Color3B::Black = {0, 0, 0};
inline constexpr Color3B Color3B::Red   = {255, 0, 0};
inline constexpr Color3B Color3B::Green = {0, 255, 0};
inline constexpr Color3B Color3B::Blue  = {0, 0, 255};
inline constexpr Color3B Color3B::Cyan   = {0, 255, 255};
inline constexpr Color3B Color3B::Magenta = {255, 0, 255};
inline constexpr Color3B Color3B::Yellow  = {255, 255, 0};
inline constexpr Color3B Color3B::GoldenYellow  = {255, 200, 0};
inline constexpr Color3B Color3B::VividRed  = {255, 50, 50};
inline constexpr Color3B Color3B::VividBlue  = {0, 150, 255};
inline constexpr Color3B Color3B::BrightOrange = {255, 150, 0};
inline constexpr Color3B Color3B::NeonGreen = {0, 255, 100};

struct Color4B {
    u8 r, g, b, a;

    bool operator==(const Color4B& other) const = default;
};
struct Color4F {
    float r, g, b, a;

    bool operator==(const Color4F& other) const = default;

    static Color4F fromColor4B(const Color4B& color) {
        return {
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            color.a / 255.0f
        };
    }

    static Color4F fromColor3B(const Color3B& color, float alpha = 1.0f) {
        return {
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            alpha
        };
    }
};

inline Color3B Color3B::fromColor4F(const Color4F& color) {
    return {
        static_cast<u8>(color.r * 255.0f),
        static_cast<u8>(color.g * 255.0f),
        static_cast<u8>(color.b * 255.0f)
    };
}

class Point;

struct Size {
    float width;
    float height;

    constexpr Size() = default;

    constexpr Size(float width, float height) : width(width), height(height) {}

    constexpr Size(const Size& other) : width(other.width), height(other.height) {}

    constexpr Size(const Point& other);

    bool operator==(const Size& other) const = default;

    Size operator+(const Size& right) const {
        return Size(width + right.width, height + right.height);
    }

    Size operator-(const Size& right) const {
        return Size(width - right.width, height - right.height);
    }

    Size operator*(const Size& right) const {
        return Size(width * right.width, height * right.height);
    }

    Size operator/(const Size& right) const {
        return Size(width / right.width, height / right.height);
    }

    Size operator*(float a) const {
        return Size(width * a, height * a);
    }

    Size operator/(float a) const {
        return Size(width / a, height / a);
    }

    Size& operator=(const Size& other) {
        width = other.width;
        height = other.height;
        return *this;
    }

    float getAspect() {
        return width / height;
    }

    void swap() {
        std::swap(width, height);
    }

    float getPerimeter() {
        return (width + height) * 2.0f;
    }

    Size toPoints() const;

    Size toPixels() const;
};

struct Point {
    float x;
    float y;

    constexpr Point() = default;

    constexpr Point(float x, float y) : x(x), y(y) {}

    constexpr Point(const Point& other) : x(other.x), y(other.y) {}

    constexpr Point(const Size& other) : x(other.width), y(other.height) {}

    bool operator==(const Point& other) const = default;

    Point operator+(const Point& right) const { return {x + right.x, y + right.y}; }
    Point operator-(const Point& right) const { return {x - right.x, y - right.y}; }
    Point operator*(const Point& right) const { return {x * right.x, y * right.y}; }
    Point operator/(const Point& right) const { return {x / right.x, y / right.y}; }

    Point operator+(float a) const { return {x + a, y + a}; }
    Point operator-(float a) const { return {x - a, y - a}; }
    Point operator*(float a) const { return {x * a, y * a}; }
    Point operator/(float a) const { return {x / a, y / a}; }

    Point& operator+=(const Point& right) { *this = *this + right; return *this; }
    Point& operator-=(const Point& right) { *this = *this - right; return *this; }
    Point& operator*=(const Point& right) { *this = *this * right; return *this; }
    Point& operator/=(const Point& right) { *this = *this / right; return *this; }

    Point& operator+=(float a) { *this = *this + a; return *this; }
    Point& operator-=(float a) { *this = *this - a; return *this; }
    Point& operator*=(float a) { *this = *this * a; return *this; }
    Point& operator/=(float a) { *this = *this / a; return *this; }

    Point operator-() const { return {-x, -y}; }

    Point& operator=(const Point& right) {
        x = right.x;
        y = right.y;
        return *this;
    }

    float getLength() const {
        return std::sqrt(x * x + y * y);
    }

    float getLengthSq() const {
        return x * x + y * y;
    }

    float getDistanceSq(const Point& other) const {
        float dx = other.x - x;
        float dy = other.y - y;
        return (dx * dx + dy * dy);
    }

    float getDistance(const Point& other) const {
        return std::sqrt(getDistanceSq(other));
    }

    Point normalize() const {
        float lenInv = 1.0f / getLength();
        return { x * lenInv, y * lenInv };
    }

    // Returns in radians
    float getAngle() const {
        return atan2f(y, x);
    }

    // Returns the counter-clockwise perpendicular vector of this vector
    Point getPerpCCW() const {
        return { -y, x };
    }

    // Returns the clockwise perpendicular vector of this vector
    Point getPerpCW() const {
        return { y, -x };
    }

    // Returns the dot product of the two vectors
    static float dot(const Point& a, const Point& b) {
        return a.x * b.x + a.y * b.y;
    }

    void swap() {
        std::swap(x, y);
    }

    Point toPoints() const;

    Point toPixels() const;
};

constexpr Size::Size(const Point& other)
    : width(other.x), height(other.y) {}

struct Rect {
    Point origin;
    Size size;

    constexpr Rect() = default;

    constexpr Rect(float x, float y, float width, float height) : origin{x, y}, size{width, height} {}

    constexpr Rect(const Point& origin, const Size& size) : origin(origin), size(size) {}

    bool operator==(const Rect& other) const = default;

    Rect& operator=(const Rect& right) {
        origin = right.origin;
        size   = right.size;
        return *this;
    }

    float getMinX() const {
        return origin.x;
    }

    float getMinY() const {
        return origin.y;
    }

    float getMaxX() const {
        return origin.x + size.width;
    }

    float getMaxY() const {
        return origin.y + size.height;
    }

    bool containsPoint(const Point& point) const {
        return point.x > getMinX() &&
               point.x < getMaxX() &&
               point.y > getMinY() &&
               point.y < getMinY();
    }

    bool intersectsRect(const Rect& rect) const {
        return !(rect.getMinX() >= getMaxX() ||
                 rect.getMaxX() <= getMinX() ||
                 rect.getMinY() >= getMaxY() ||
                 rect.getMaxY() <= getMinY());
    }
};

inline constexpr Point PointZero(0.0f, 0.0f);
inline constexpr Size SizeZero(0.0f, 0.0f);
inline constexpr Rect RectZero(PointZero, SizeZero);

enum class BlendMode {
    Normal,
    Additive,
    Multiplicative
};

} // end of namespace gd3ds