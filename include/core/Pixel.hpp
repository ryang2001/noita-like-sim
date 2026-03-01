#pragma once

#include <cstdint>
#include <type_traits>

namespace noita {

/**
 * @brief 材料ID类型 (运行时分配)
 * 
 * 使用uint16_t支持最多65536种材料
 * 0保留为空材料
 */
using MaterialID = uint16_t;
constexpr MaterialID EMPTY_MATERIAL = 0;
constexpr MaterialID INVALID_MATERIAL = 65535;

/**
 * @brief 材料状态枚举
 * 
 * 定义材料的基本物理状态
 */
enum class MaterialState : uint8_t {
    Solid = 0,      ///< 固体 (岩石、金属等)
    Powder,         ///< 粉末 (沙子、土壤等)
    Liquid,         ///< 液体 (水、岩浆等)
    Gas,            ///< 气体 (蒸汽、烟雾等)
    Plasma,         ///< 等离子体 (火焰、闪电等)
    Energy,         ///< 能量 (激光、魔法等)
    COUNT           ///< 状态总数
};

/**
 * @brief 像素标志位
 * 
 * 使用位域存储多个布尔状态,节省内存
 */
enum class PixelFlags : uint8_t {
    None = 0,
    Burning = 1 << 0,       ///< 正在燃烧
    Wet = 1 << 1,           ///< 潮湿
    Electrified = 1 << 2,   ///< 带电
    Frozen = 1 << 3,        ///< 冻结
    Poisoned = 1 << 4,      ///< 中毒
    Glowing = 1 << 5,       ///< 发光
    Updated = 1 << 6,       ///< 本帧已更新
    Reserved = 1 << 7       ///< 保留位
};

// 支持位运算
inline PixelFlags operator|(PixelFlags a, PixelFlags b) {
    return static_cast<PixelFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline PixelFlags operator&(PixelFlags a, PixelFlags b) {
    return static_cast<PixelFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline PixelFlags& operator|=(PixelFlags& a, PixelFlags b) {
    a = a | b;
    return a;
}

inline bool has_flag(PixelFlags flags, PixelFlags flag) {
    return (flags & flag) == flag;
}

/**
 * @brief 像素数据结构
 *
 * 每个像素占用16字节,16字节对齐以支持SIMD优化
 *
 * 内存布局:
 * - material (2字节): 材料ID
 * - state (1字节): 材料状态
 * - flags (1字节): 状态标志
 * - padding (4字节): 对齐填充
 * - temperature (4字节): 温度(摄氏度)
 * - lifetime (2字节): 生命周期
 * - variation (1字节): 视觉变体
 * - light_level (1字节): 发光强度
 *
 * 注意: 不存储速度,每帧只移动一个像素距离 (Noita设计)
 */
struct alignas(16) Pixel {
    // 材料信息 (4字节)
    MaterialID material = EMPTY_MATERIAL;
    MaterialState state = MaterialState::Solid;
    PixelFlags flags = PixelFlags::None;
    uint8_t padding1 = 0;

    // 物理属性 (4字节)
    float temperature = 20.0f;  // 默认室温

    // 生命周期和渲染 (4字节)
    uint16_t lifetime = 0;
    uint8_t variation = 0;
    uint8_t light_level = 0;

    // 填充以保持16字节对齐
    uint8_t padding2[8] = {};
    
    // 默认构造
    Pixel() = default;
    
    // 带材料ID的构造
    explicit Pixel(MaterialID mat, MaterialState st = MaterialState::Solid)
        : material(mat), state(st) {}
    
    // 状态查询
    bool is_empty() const { return material == EMPTY_MATERIAL; }
    bool is_solid() const { return state == MaterialState::Solid; }
    bool is_powder() const { return state == MaterialState::Powder; }
    bool is_liquid() const { return state == MaterialState::Liquid; }
    bool is_gas() const { return state == MaterialState::Gas; }
    bool is_plasma() const { return state == MaterialState::Plasma; }
    bool is_energy() const { return state == MaterialState::Energy; }
    
    // 标志位操作
    bool is_burning() const { return has_flag(flags, PixelFlags::Burning); }
    bool is_wet() const { return has_flag(flags, PixelFlags::Wet); }
    bool is_electrified() const { return has_flag(flags, PixelFlags::Electrified); }
    bool is_frozen() const { return has_flag(flags, PixelFlags::Frozen); }
    bool is_poisoned() const { return has_flag(flags, PixelFlags::Poisoned); }
    bool is_glowing() const { return has_flag(flags, PixelFlags::Glowing); }
    bool is_updated() const { return has_flag(flags, PixelFlags::Updated); }
    
    void set_burning(bool value) {
        if (value) flags |= PixelFlags::Burning;
        else flags = flags & static_cast<PixelFlags>(~static_cast<uint8_t>(PixelFlags::Burning));
    }
    
    void set_wet(bool value) {
        if (value) flags |= PixelFlags::Wet;
        else flags = flags & static_cast<PixelFlags>(~static_cast<uint8_t>(PixelFlags::Wet));
    }
    
    void set_electrified(bool value) {
        if (value) flags |= PixelFlags::Electrified;
        else flags = flags & static_cast<PixelFlags>(~static_cast<uint8_t>(PixelFlags::Electrified));
    }
    
    void set_frozen(bool value) {
        if (value) flags |= PixelFlags::Frozen;
        else flags = flags & static_cast<PixelFlags>(~static_cast<uint8_t>(PixelFlags::Frozen));
    }
    
    void set_updated(bool value) {
        if (value) flags |= PixelFlags::Updated;
        else flags = flags & static_cast<PixelFlags>(~static_cast<uint8_t>(PixelFlags::Updated));
    }

    // 温度操作
    void add_temperature(float delta) {
        temperature += delta;
    }
    
    void set_temperature(float temp) {
        temperature = temp;
    }
    
    // 生命周期操作
    void decrease_lifetime() {
        if (lifetime > 0) lifetime--;
    }
    
    bool is_dead() const {
        return lifetime == 0 && material != EMPTY_MATERIAL;
    }
    
    // 清空像素
    void clear() {
        material = EMPTY_MATERIAL;
        state = MaterialState::Solid;
        flags = PixelFlags::None;
        temperature = 20.0f;
        lifetime = 0;
        variation = 0;
        light_level = 0;
    }
};

// 静态断言确保大小和对齐
static_assert(sizeof(Pixel) == 32, "Pixel must be 32 bytes");
static_assert(alignof(Pixel) == 16, "Pixel must be 16-byte aligned");
static_assert(std::is_trivially_copyable<Pixel>::value, "Pixel must be trivially copyable");

} // namespace noita
