#include "materials/MaterialRegistry.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <array>
#include "../../external/nlohmann/json.hpp"

namespace noita {

// 静态成员初始化
MaterialRegistry* MaterialRegistry::instance_ = nullptr;

MaterialRegistry::MaterialRegistry() {
    // 初始化查找表
    lookup_table_.fill(nullptr);
    
    // 注册空材料
    MaterialProperties empty_props;
    empty_props.id = "empty";
    empty_props.name = "Empty";
    empty_props.description = "Empty space";
    empty_props.default_state = MaterialState::Solid;
    empty_props.density = 0.0f;
    empty_props.is_gravity_affected = false;
    
    register_material("empty", empty_props);
}

MaterialRegistry::~MaterialRegistry() {
    materials_.clear();
    name_to_id_.clear();
}

MaterialRegistry& MaterialRegistry::instance() {
    if (!instance_) {
        instance_ = new MaterialRegistry();
    }
    return *instance_;
}

void MaterialRegistry::destroy_instance() {
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}

MaterialID MaterialRegistry::register_material(const std::string& id, 
                                                const MaterialProperties& props) {
    // 检查是否已存在
    auto it = name_to_id_.find(id);
    if (it != name_to_id_.end()) {
        std::cerr << "Warning: Material '" << id << "' already exists, overwriting." << std::endl;
        materials_[it->second] = props;
        return it->second;
    }
    
    // 检查材料数量限制
    if (materials_.size() >= constants::MAX_MATERIALS) {
        std::cerr << "Error: Maximum material count reached (" << constants::MAX_MATERIALS << ")" << std::endl;
        return EMPTY_MATERIAL;
    }
    
    // 验证材料属性
    if (!validate_material(props)) {
        std::cerr << "Error: Invalid material properties for '" << id << "'" << std::endl;
        return EMPTY_MATERIAL;
    }
    
    // 分配新ID
    MaterialID new_id = static_cast<MaterialID>(materials_.size());
    
    // 添加到向量
    materials_.push_back(props);
    
    // 添加到名称映射
    name_to_id_[id] = new_id;
    
    // 更新查找表
    lookup_table_[new_id] = &materials_.back();
    
    return new_id;
}

void MaterialRegistry::register_materials(
    const std::vector<std::pair<std::string, MaterialProperties>>& materials) {
    for (const auto& [id, props] : materials) {
        register_material(id, props);
    }
}

bool MaterialRegistry::load_from_json(const std::string& path) {
    using json = nlohmann::json;
    
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file: " << path << std::endl;
            return false;
        }
        
        json config;
        file >> config;
        
        if (!config.contains("materials") || !config["materials"].is_array()) {
            std::cerr << "Error: Invalid JSON format in " << path << std::endl;
            return false;
        }
        
        for (const auto& mat_json : config["materials"]) {
            MaterialProperties props;
            
            // 基本信息
            props.id = mat_json.value("id", "");
            props.name = mat_json.value("name", props.id);
            props.description = mat_json.value("description", "");
            
            if (props.id.empty()) {
                std::cerr << "Warning: Material missing 'id' field, skipping." << std::endl;
                continue;
            }
            
            // 状态
            std::string state_str = mat_json.value("state", "solid");
            if (state_str == "solid") props.default_state = MaterialState::Solid;
            else if (state_str == "powder") props.default_state = MaterialState::Powder;
            else if (state_str == "liquid") props.default_state = MaterialState::Liquid;
            else if (state_str == "gas") props.default_state = MaterialState::Gas;
            else if (state_str == "plasma") props.default_state = MaterialState::Plasma;
            else if (state_str == "energy") props.default_state = MaterialState::Energy;
            
            // 物理属性
            props.density = mat_json.value("density", 1.0f);
            props.viscosity = mat_json.value("viscosity", 1.0f);
            props.hardness = mat_json.value("hardness", 1.0f);
            props.elasticity = mat_json.value("elasticity", 0.0f);
            props.friction = mat_json.value("friction", 0.5f);
            
            // 热力学属性
            props.thermal_conductivity = mat_json.value("thermal_conductivity", 0.5f);
            props.specific_heat = mat_json.value("specific_heat", 1.0f);
            props.melting_point = mat_json.value("melting_point", 1000.0f);
            props.boiling_point = mat_json.value("boiling_point", 2000.0f);
            props.ignition_point = mat_json.value("ignition_point", 500.0f);
            props.freezing_point = mat_json.value("freezing_point", 0.0f);
            
            // 化学属性
            props.flammability = mat_json.value("flammability", 0.0f);
            props.explosiveness = mat_json.value("explosiveness", 0.0f);
            props.corrosiveness = mat_json.value("corrosiveness", 0.0f);
            props.toxicity = mat_json.value("toxicity", 0.0f);
            props.acidity = mat_json.value("acidity", 0.0f);
            
            // 电学属性
            props.electrical_conductivity = mat_json.value("electrical_conductivity", 0.0f);
            
            // 颜色
            if (mat_json.contains("color")) {
                const auto& color = mat_json["color"];
                props.color.r = color.value("r", 128);
                props.color.g = color.value("g", 128);
                props.color.b = color.value("b", 128);
                props.color.a = color.value("a", 255);
                props.color.variation_count = color.value("variation_count", 1);
            }
            
            // 发光属性
            props.emits_light = mat_json.value("emits_light", false);
            if (props.emits_light) {
                props.light_radius = mat_json.value("light_radius", 0);
                props.light_intensity = mat_json.value("light_intensity", 0);
                
                if (mat_json.contains("light_color")) {
                    const auto& lc = mat_json["light_color"];
                    props.light_color.r = lc.value("r", 255);
                    props.light_color.g = lc.value("g", 255);
                    props.light_color.b = lc.value("b", 255);
                    props.light_color.a = lc.value("a", 255);
                }
            }
            
            // 状态转换
            if (mat_json.contains("melted_form")) {
                props.melted_form = get_id(mat_json["melted_form"]);
            }
            if (mat_json.contains("boiled_form")) {
                props.boiled_form = get_id(mat_json["boiled_form"]);
            }
            if (mat_json.contains("burned_form")) {
                props.burned_form = get_id(mat_json["burned_form"]);
            }
            if (mat_json.contains("frozen_form")) {
                props.frozen_form = get_id(mat_json["frozen_form"]);
            }
            if (mat_json.contains("evaporated_form")) {
                props.evaporated_form = get_id(mat_json["evaporated_form"]);
            }
            
            // 特殊效果
            props.is_solid_ground = mat_json.value("is_solid_ground", false);
            props.floats_on_water = mat_json.value("floats_on_water", false);
            props.sinks_in_water = mat_json.value("sinks_in_water", true);
            props.is_gravity_affected = mat_json.value("is_gravity_affected", true);
            props.is_breathable = mat_json.value("is_breathable", false);
            
            // 注册材料
            register_material(props.id, props);
        }
        
        std::cout << "Loaded " << config["materials"].size() << " materials from " << path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON file " << path << ": " << e.what() << std::endl;
        return false;
    }
}

int MaterialRegistry::load_from_directory(const std::string& dir_path) {
    // 简化实现: 只加载单个文件
    // 实际项目中应该遍历目录加载所有JSON文件
    std::string file_path = dir_path + "/basic_materials.json";
    if (load_from_json(file_path)) {
        return static_cast<int>(materials_.size());
    }
    return 0;
}

const MaterialProperties& MaterialRegistry::get(MaterialID id) const {
    if (id < materials_.size()) {
        return materials_[id];
    }
    // 返回空材料
    static MaterialProperties empty_props;
    return empty_props;
}

MaterialID MaterialRegistry::get_id(const std::string& name) const {
    auto it = name_to_id_.find(name);
    if (it != name_to_id_.end()) {
        return it->second;
    }
    return EMPTY_MATERIAL;
}

bool MaterialRegistry::exists(MaterialID id) const {
    return id < materials_.size();
}

bool MaterialRegistry::exists(const std::string& name) const {
    return name_to_id_.find(name) != name_to_id_.end();
}

void MaterialRegistry::for_each(MaterialCallback callback) const {
    for (MaterialID id = 0; id < materials_.size(); ++id) {
        callback(id, materials_[id]);
    }
}

void MaterialRegistry::clear() {
    materials_.clear();
    name_to_id_.clear();
    lookup_table_.fill(nullptr);
    
    // 重新注册空材料
    MaterialProperties empty_props;
    empty_props.id = "empty";
    empty_props.name = "Empty";
    empty_props.default_state = MaterialState::Solid;
    empty_props.density = 0.0f;
    empty_props.is_gravity_affected = false;
    
    register_material("empty", empty_props);
}

void MaterialRegistry::reload(const std::string& dir_path) {
    clear();
    load_from_directory(dir_path);
}

std::vector<MaterialID> MaterialRegistry::get_all_ids() const {
    std::vector<MaterialID> ids;
    ids.reserve(materials_.size());
    for (MaterialID id = 0; id < materials_.size(); ++id) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<MaterialID> MaterialRegistry::filter_by_state(MaterialState state) const {
    std::vector<MaterialID> result;
    for (MaterialID id = 0; id < materials_.size(); ++id) {
        if (materials_[id].default_state == state) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<MaterialID> MaterialRegistry::filter_by_tag(const std::string& tag) const {
    // TODO: 实现标签系统
    return std::vector<MaterialID>();
}

bool MaterialRegistry::validate_material(const MaterialProperties& props) const {
    // 基本验证
    if (props.id.empty()) {
        return false;
    }
    
    // 密度验证
    if (props.density < 0.0f) {
        std::cerr << "Warning: Material '" << props.id << "' has negative density." << std::endl;
        return false;
    }
    
    // 温度验证
    if (props.melting_point > props.boiling_point) {
        std::cerr << "Warning: Material '" << props.id << "' has melting point > boiling point." << std::endl;
    }
    
    return true;
}

} // namespace noita
