#pragma once

#include "core/Types.hpp"
#include "core/Pixel.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>
#include <array>

namespace noita {

/**
 * @brief 材料属性结构
 * 
 * 定义材料的所有物理、化学和渲染属性
 */
struct MaterialProperties {
    // 基本信息
    std::string id;           ///< 材料唯一标识符 (如 "sand", "water")
    std::string name;         ///< 显示名称 (如 "Sand", "Water")
    std::string description;  ///< 描述文本
    
    // 物理属性
    MaterialState default_state = MaterialState::Solid;
    float32 density = 1.0f;              ///< 密度 (g/cm³)
    float32 viscosity = 1.0f;            ///< 粘度 (1.0=水, 100.0=蜂蜜)
    float32 hardness = 1.0f;             ///< 硬度 (1-10)
    float32 elasticity = 0.0f;           ///< 弹性系数 (0.0-1.0)
    float32 friction = 0.5f;             ///< 摩擦系数 (0.0-1.0)
    
    // 热力学属性
    float32 thermal_conductivity = 0.5f; ///< 热导率 (W/m·K)
    float32 specific_heat = 1.0f;        ///< 比热容 (J/g·K)
    float32 melting_point = 1000.0f;     ///< 熔点 (°C)
    float32 boiling_point = 2000.0f;     ///< 沸点 (°C)
    float32 ignition_point = 500.0f;     ///< 燃点 (°C)
    float32 freezing_point = 0.0f;       ///< 凝固点 (°C)
    
    // 化学属性
    float32 flammability = 0.0f;         ///< 可燃性 (0.0-1.0)
    float32 explosiveness = 0.0f;        ///< 易爆性 (0.0-1.0)
    float32 corrosiveness = 0.0f;        ///< 腐蚀性 (0.0-1.0)
    float32 toxicity = 0.0f;             ///< 毒性 (0.0-1.0)
    float32 acidity = 0.0f;              ///< 酸性 (0.0-14.0, pH值)
    
    // 电学属性
    float32 electrical_conductivity = 0.0f; ///< 电导率 (0.0-1.0)
    
    // 渲染属性
    struct ColorInfo {
        uint8 r, g, b, a;
        uint8 variation_count = 1;  ///< 颜色变体数量
        
        ColorInfo() : r(128), g(128), b(128), a(255), variation_count(1) {}
        ColorInfo(uint8 r, uint8 g, uint8 b, uint8 a = 255, uint8 var = 1)
            : r(r), g(g), b(b), a(a), variation_count(var) {}
    } color;
    
    // 发光属性
    bool emits_light = false;
    uint8 light_radius = 0;
    uint8 light_intensity = 0;
    ColorInfo light_color;
    
    // 状态转换
    MaterialID melted_form = EMPTY_MATERIAL;   ///< 熔化后的材料
    MaterialID boiled_form = EMPTY_MATERIAL;   ///< 沸腾后的材料
    MaterialID burned_form = EMPTY_MATERIAL;   ///< 燃烧后的材料
    MaterialID frozen_form = EMPTY_MATERIAL;   ///< 冻结后的材料
    MaterialID evaporated_form = EMPTY_MATERIAL; ///< 蒸发后的材料
    
    // 特殊效果
    bool is_solid_ground = false;      ///< 是否可作为固体地面
    bool floats_on_water = false;      ///< 是否浮在水上
    bool sinks_in_water = true;        ///< 是否沉入水中
    bool is_gravity_affected = true;   ///< 是否受重力影响
    bool is_breathable = false;        ///< 是否可呼吸(气体)
    
    // 默认构造
    MaterialProperties() = default;
    
    // 简化构造
    MaterialProperties(const std::string& id, const std::string& name, 
                       MaterialState state, float32 density)
        : id(id), name(name), default_state(state), density(density) {}
};

/**
 * @brief 材料注册表
 * 
 * 运行时材料管理系统,支持动态注册和查询
 * 使用单例模式,全局唯一实例
 */
class MaterialRegistry {
public:
    using MaterialCallback = std::function<void(MaterialID, const MaterialProperties&)>;
    
private:
    std::vector<MaterialProperties> materials_;
    std::unordered_map<std::string, MaterialID> name_to_id_;
    
    // 快速查找数组 (O(1)访问)
    std::array<const MaterialProperties*, constants::MAX_MATERIALS> lookup_table_;
    
    // 单例实例
    static MaterialRegistry* instance_;
    
    // 私有构造
    MaterialRegistry();
    
public:
    // 禁止拷贝
    MaterialRegistry(const MaterialRegistry&) = delete;
    MaterialRegistry& operator=(const MaterialRegistry&) = delete;
    
    // 析构
    ~MaterialRegistry();
    
    /**
     * @brief 获取单例实例
     */
    static MaterialRegistry& instance();
    
    /**
     * @brief 销毁单例实例
     */
    static void destroy_instance();
    
    /**
     * @brief 注册新材料
     * @param id 材料唯一标识符
     * @param props 材料属性
     * @return 分配的材料ID
     */
    MaterialID register_material(const std::string& id, 
                                  const MaterialProperties& props);
    
    /**
     * @brief 批量注册材料
     * @param materials 材料列表
     */
    void register_materials(const std::vector<std::pair<std::string, MaterialProperties>>& materials);
    
    /**
     * @brief 从JSON文件加载材料
     * @param path JSON文件路径
     * @return 是否成功
     */
    bool load_from_json(const std::string& path);
    
    /**
     * @brief 从目录加载所有材料文件
     * @param dir_path 目录路径
     * @return 加载的材料数量
     */
    int load_from_directory(const std::string& dir_path);
    
    /**
     * @brief 获取材料属性 (O(1))
     * @param id 材料ID
     * @return 材料属性引用
     */
    const MaterialProperties& get(MaterialID id) const;
    
    /**
     * @brief 通过名称获取材料ID
     * @param name 材料名称或ID
     * @return 材料ID,不存在返回EMPTY_MATERIAL
     */
    MaterialID get_id(const std::string& name) const;
    
    /**
     * @brief 检查材料是否存在
     * @param id 材料ID
     */
    bool exists(MaterialID id) const;
    
    /**
     * @brief 检查材料是否存在
     * @param name 材料名称
     */
    bool exists(const std::string& name) const;
    
    /**
     * @brief 获取材料数量
     */
    size_t size() const { return materials_.size(); }
    
    /**
     * @brief 遍历所有材料
     * @param callback 回调函数
     */
    void for_each(MaterialCallback callback) const;
    
    /**
     * @brief 迭代器支持
     */
    auto begin() const { return materials_.begin(); }
    auto end() const { return materials_.end(); }
    
    /**
     * @brief 清空所有材料
     */
    void clear();
    
    /**
     * @brief 重新加载所有材料
     * @param dir_path 材料配置目录
     */
    void reload(const std::string& dir_path);
    
    /**
     * @brief 获取所有材料ID列表
     */
    std::vector<MaterialID> get_all_ids() const;
    
    /**
     * @brief 按状态筛选材料
     * @param state 材料状态
     * @return 符合条件的材料ID列表
     */
    std::vector<MaterialID> filter_by_state(MaterialState state) const;
    
    /**
     * @brief 按标签筛选材料
     * @param tag 标签名称
     * @return 符合条件的材料ID列表
     */
    std::vector<MaterialID> filter_by_tag(const std::string& tag) const;
    
private:
    // 初始化默认材料
    void initialize_default_materials();
    
    // 解析JSON材料定义
    std::optional<MaterialProperties> parse_material_json(const std::string& json_str);
    
    // 验证材料属性
    bool validate_material(const MaterialProperties& props) const;
};

// 便捷宏定义
#define REGISTER_MATERIAL(id, props) \
    noita::MaterialRegistry::instance().register_material(id, props)

#define GET_MATERIAL(id) \
    noita::MaterialRegistry::instance().get(id)

#define GET_MATERIAL_ID(name) \
    noita::MaterialRegistry::instance().get_id(name)

} // namespace noita
