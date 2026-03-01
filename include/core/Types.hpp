#pragma once
#include <cmath>

#include <cstdint>
#include <cstddef>

namespace noita {

/**
 * @brief 基础类型定义
 * 
 * 提供项目通用的类型别名和常量定义
 */

// 整数类型
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

// 浮点类型
using float32 = float;
using float64 = double;

// 尺寸类型
using size_type = size_t;

// 坐标类型
using Coord = int32;
using Position = struct { Coord x, y; };

// 颜色类型
struct Color {
    uint8 r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8 r, uint8 g, uint8 b, uint8 a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
    // 预定义颜色
    static Color white() { return Color(255, 255, 255, 255); }
    static Color black() { return Color(0, 0, 0, 255); }
    static Color red() { return Color(255, 0, 0, 255); }
    static Color green() { return Color(0, 255, 0, 255); }
    static Color blue() { return Color(0, 0, 255, 255); }
    static Color transparent() { return Color(0, 0, 0, 0); }
};

// 矩形区域
struct Rect {
    int32 x, y;
    int32 width, height;
    
    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int32 x, int32 y, int32 w, int32 h) 
        : x(x), y(y), width(w), height(h) {}
    
    bool contains(int32 px, int32 py) const {
        return px >= x && px < x + width && 
               py >= y && py < y + height;
    }
    
    bool intersects(const Rect& other) const {
        return x < other.x + other.width &&
               x + width > other.x &&
               y < other.y + other.height &&
               y + height > other.y;
    }
};

// 向量类型
template<typename T>
struct Vec2 {
    T x, y;
    
    Vec2() : x(0), y(0) {}
    Vec2(T x, T y) : x(x), y(y) {}
    
    Vec2 operator+(const Vec2& other) const {
        return Vec2(x + other.x, y + other.y);
    }
    
    Vec2 operator-(const Vec2& other) const {
        return Vec2(x - other.x, y - other.y);
    }
    
    Vec2 operator*(T scalar) const {
        return Vec2(x * scalar, y * scalar);
    }
    
    T length_squared() const {
        return x * x + y * y;
    }
    
    float length() const {
        return std::sqrt(static_cast<float>(length_squared()));
    }
};

using Vec2i = Vec2<int32>;
using Vec2f = Vec2<float32>;

// 常量定义
namespace constants {
    constexpr float32 PI = 3.14159265358979323846f;
    constexpr float32 TWO_PI = 2.0f * PI;
    constexpr float32 HALF_PI = PI / 2.0f;
    
    constexpr float32 GRAVITY = 9.81f;
    constexpr float32 DEFAULT_TEMPERATURE = 20.0f;  // 摄氏度
    
    constexpr int32 CHUNK_SIZE = 128;
    constexpr int32 TILE_SIZE = 16;
    
    constexpr int32 MAX_MATERIALS = 65536;
    constexpr int32 MAX_REACTIONS = 10000;
}

} // namespace noita
