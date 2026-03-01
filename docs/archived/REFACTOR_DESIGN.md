# Noita-like 像素物理引擎重构设计方案

## 📋 目录
1. [设计目标](#设计目标)
2. [核心架构](#核心架构)
3. [模块重构方案](#模块重构方案)
4. [性能优化策略](#性能优化策略)
5. [实施路线图](#实施路线图)

---

## 🎯 设计目标

### 基于Noita参考资料的核心设计理念

#### 1. **像素级物理模拟** (参考资料[1][2][3])
- **目标**: 每个像素独立物理模拟,实现真实的材料交互
- **Noita实现**: 
  - 使用"falling sand"算法变体
  - 每帧更新所有活跃像素
  - 材料有密度、粘度、温度等属性
- **我们的改进**:
  - 采用更高效的并行算法
  - 实现完整的压力系统
  - 支持材料状态转换

#### 2. **Herringbone Wang Tiles地形生成** (参考资料[4])
- **目标**: 无缝、多样化的程序化地形
- **Noita实现**:
  - 使用Herringbone布局(非传统网格)
  - 数百种tile变体
  - 边缘约束匹配算法
- **我们的改进**:
  - 实现Wave Function Collapse算法
  - 支持运行时tile扩展
  - 生物群系分层系统

#### 3. **高性能渲染** (参考资料[5])
- **目标**: 60FPS稳定运行,支持大规模像素世界
- **Noita实现**:
  - GPU加速渲染
  - 脏块优化
  - 多线程模拟
- **我们的改进**:
  - OpenGL/Vulkan后端
  - Compute Shader加速物理模拟
  - 异步渲染管线

#### 4. **材料化学反应系统** (参考资料[3])
- **目标**: 复杂的材料交互和转换
- **Noita实现**:
  - 温度驱动的物态变化
  - 材料接触反应
  - 燃烧、腐蚀、爆炸等
- **我们的改进**:
  - 基于规则的反应引擎
  - 支持链式反应
  - 材料混合系统

---

## 🏗️ 核心架构

### 新架构概览

```
┌─────────────────────────────────────────────────────────────┐
│                        Game Engine                          │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │  ECS Core    │  │  Event Bus   │  │  Config Mgr  │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
├─────────────────────────────────────────────────────────────┤
│                     Simulation Layer                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Pixel World  │  │ Material DB  │  │ Reaction Sys │     │
│  │  (Chunked)   │  │  (Runtime)   │  │  (Rule-based)│     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Physics Sys  │  │ Pressure Sys │  │Electric Sys  │     │
│  │  (Parallel)  │  │  (Jacobi)    │  │  (Graph)     │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
├─────────────────────────────────────────────────────────────┤
│                     Generation Layer                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Wang Tiles   │  │ Biome Config │  │  WFC Engine  │     │
│  │ (Herringbone)│  │  (JSON)      │  │  (Advanced)  │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
├─────────────────────────────────────────────────────────────┤
│                       Render Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ OpenGL/Vulkan│  │ Sprite Batch │  │  Post-Proc   │     │
│  │   Backend    │  │  (Instanced) │  │  (Shaders)   │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
├─────────────────────────────────────────────────────────────┤
│                     Threading Layer                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Work-Stealing│  │  Task Graph  │  │  Async I/O   │     │
│  │    Pool      │  │  (DAG)       │  │              │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### 核心设计原则

1. **数据驱动**: 所有配置从JSON加载,支持热重载
2. **并行优先**: 所有系统设计为可并行执行
3. **模块解耦**: 使用ECS架构,组件与系统分离
4. **性能可测**: 内置性能分析工具,实时监控

---

## 🔧 模块重构方案

### 1. 像素物理引擎核心重构

#### 1.1 新的World架构

**当前问题**:
- 硬编码尺寸(512x512)
- 两套chunk系统并存
- 全局液体更新是性能瓶颈

**新设计方案**:

```cpp
// include/core/World.hpp
namespace noita {

class World {
public:
    struct Config {
        int width = 1024;
        int height = 1024;
        int chunk_size = 128;
        int thread_count = -1;  // -1 = auto
        bool use_gpu_simulation = false;
    };

private:
    // 分块系统
    struct Chunk {
        static constexpr int SIZE = 128;
        
        std::array<Pixel, SIZE * SIZE> pixels;
        std::array<Pixel, SIZE * 4> edge_buffers;  // N,E,S,W
        
        // 双缓冲
        std::array<Pixel, SIZE * SIZE> write_buffer;
        
        // 元数据
        int chunk_x, chunk_y;
        bool is_dirty = false;
        bool is_active = false;
        int complexity_score = 0;
        
        // 邻居指针
        Chunk* neighbors[4] = {nullptr};  // N,E,S,W
        
        void update(float dt);
        void sync_edges();
        void swap_buffers();
    };
    
    std::vector<std::unique_ptr<Chunk>> chunks_;
    int chunks_x_, chunks_y_;
    
    // 线程池
    std::unique_ptr<WorkStealingPool> thread_pool_;
    
    // 活跃区域管理
    struct ActiveRegion {
        int center_x, center_y;
        int radius = 200;
        void update(const Player& player);
    } active_region_;
    
    // 配置
    Config config_;

public:
    World(const Config& config);
    ~World();
    
    // 主更新循环
    void update(float dt);
    
    // 并行更新策略
    void update_parallel(float dt);
    void update_chunk_pass(int pass, float dt);
    
    // 像素访问
    Pixel& get(int x, int y);
    const Pixel& get(int x, int y) const;
    void set(int x, int y, const Pixel& pixel);
    
    // Chunk管理
    Chunk* get_chunk(int x, int y);
    void activate_chunk(int cx, int cy);
    void deactivate_chunk(int cx, int cy);
    
    // 序列化
    void save(const std::string& path);
    void load(const std::string& path);
};

} // namespace noita
```

**关键改进**:

1. **动态尺寸**: 运行时配置世界大小
2. **统一Chunk系统**: 删除冗余的ChunkedPixelWorld
3. **双缓冲**: 避免读写冲突,支持完全并行
4. **边缘同步**: Chunk间通信协议
5. **活跃区域**: 只更新玩家附近区域

#### 1.2 新的Pixel数据结构

**当前问题**:
- 材料类型硬编码(60+种枚举)
- velocity精度不足(int8_t)
- 缺乏材料状态信息

**新设计方案**:

```cpp
// include/core/Pixel.hpp
namespace noita {

// 材料ID (运行时分配)
using MaterialID = uint16_t;
constexpr MaterialID EMPTY_MATERIAL = 0;

// 材料状态
enum class MaterialState : uint8_t {
    Solid = 0,
    Powder,
    Liquid,
    Gas,
    Plasma,
    Energy  // 闪电、激光等
};

// 像素数据结构 (16字节,对齐友好)
struct alignas(16) Pixel {
    MaterialID material = EMPTY_MATERIAL;
    MaterialState state = MaterialState::Solid;
    
    // 物理属性
    float velocity_x = 0.0f;      // 改为float,支持高速
    float velocity_y = 0.0f;
    float temperature = 20.0f;     // 摄氏度
    
    // 生命周期
    uint16_t lifetime = 0;
    uint16_t age = 0;
    
    // 渲染属性
    uint8_t variation = 0;        // 视觉变体
    uint8_t light_level = 0;      // 发光强度
    uint8_t flags = 0;            // 位标志
    uint8_t padding = 0;          // 对齐填充
    
    // 状态查询
    bool is_empty() const { return material == EMPTY_MATERIAL; }
    bool is_solid() const { return state == MaterialState::Solid; }
    bool is_liquid() const { return state == MaterialState::Liquid; }
    bool is_gas() const { return state == MaterialState::Gas; }
    bool is_burning() const { return flags & 0x01; }
    void set_burning(bool value) { 
        if (value) flags |= 0x01; 
        else flags &= ~0x01; 
    }
};

// 静态断言确保大小
static_assert(sizeof(Pixel) == 16, "Pixel must be 16 bytes");
static_assert(alignof(Pixel) == 16, "Pixel must be 16-byte aligned");

} // namespace noita
```

**关键改进**:

1. **运行时材料ID**: 支持动态注册材料
2. **MaterialState枚举**: 明确材料状态
3. **float velocity**: 支持高速运动
4. **位标志**: 高效存储布尔状态
5. **16字节对齐**: SIMD优化友好

---

### 2. 材料系统重构

#### 2.1 运行时材料注册系统

**当前问题**:
- 硬编码60+种材料枚举
- 添加新材料需重新编译
- 属性查询效率低(哈希表)

**新设计方案**:

```cpp
// include/materials/MaterialRegistry.hpp
namespace noita {

struct MaterialProperties {
    std::string name;
    std::string id;
    
    // 物理属性
    MaterialState default_state = MaterialState::Solid;
    float density = 1.0f;
    float viscosity = 1.0f;
    float hardness = 1.0f;
    float elasticity = 0.0f;
    
    // 热力学属性
    float thermal_conductivity = 0.5f;
    float specific_heat = 1.0f;
    float melting_point = 1000.0f;
    float boiling_point = 2000.0f;
    float ignition_point = 500.0f;
    
    // 化学属性
    float flammability = 0.0f;
    float explosiveness = 0.0f;
    float corrosiveness = 0.0f;
    float toxicity = 0.0f;
    
    // 电学属性
    float electrical_conductivity = 0.0f;
    
    // 渲染属性
    struct Color {
        uint8_t r, g, b, a;
        uint8_t variation_count = 1;
    } color;
    
    // 发光属性
    bool emits_light = false;
    uint8_t light_radius = 0;
    uint8_t light_intensity = 0;
    Color light_color;
    
    // 状态转换
    MaterialID melted_form = EMPTY_MATERIAL;
    MaterialID boiled_form = EMPTY_MATERIAL;
    MaterialID burned_form = EMPTY_MATERIAL;
    MaterialID frozen_form = EMPTY_MATERIAL;
};

class MaterialRegistry {
private:
    std::vector<MaterialProperties> materials_;
    std::unordered_map<std::string, MaterialID> name_to_id_;
    
    // 快速查找数组
    std::array<const MaterialProperties*, 65536> lookup_table_;
    
public:
    MaterialRegistry();
    
    // 材料注册
    MaterialID register_material(const std::string& id, 
                                  const MaterialProperties& props);
    
    // 批量注册
    void load_from_json(const std::string& path);
    
    // 查询 (O(1))
    const MaterialProperties& get(MaterialID id) const {
        return materials_[id];  // 直接数组访问
    }
    
    MaterialID get_id(const std::string& name) const {
        return name_to_id_.at(name);
    }
    
    // 迭代
    auto begin() const { return materials_.begin(); }
    auto end() const { return materials_.end(); }
    size_t size() const { return materials_.size(); }
    
    // 单例访问
    static MaterialRegistry& instance();
};

// 便捷宏
#define REGISTER_MATERIAL(id, props) \
    MaterialRegistry::instance().register_material(id, props)

} // namespace noita
```

**JSON配置示例**:

```json
{
  "materials": [
    {
      "id": "sand",
      "name": "Sand",
      "state": "powder",
      "density": 2.5,
      "color": {
        "r": 194, "g": 178, "b": 128, "a": 255,
        "variation_count": 8
      },
      "thermal_conductivity": 0.3,
      "melting_point": 1700.0,
      "melted_form": "glass"
    },
    {
      "id": "water",
      "name": "Water",
      "state": "liquid",
      "density": 1.0,
      "viscosity": 1.0,
      "color": {
        "r": 64, "g": 164, "b": 223, "a": 200,
        "variation_count": 4
      },
      "thermal_conductivity": 0.6,
      "specific_heat": 4.186,
      "melting_point": 0.0,
      "boiling_point": 100.0,
      "frozen_form": "ice",
      "boiled_form": "steam"
    }
  ]
}
```

#### 2.2 基于规则的化学反应引擎

**当前问题**:
- 线性搜索反应列表(O(n))
- 反应定义硬编码
- 缺乏链式反应支持

**新设计方案**:

```cpp
// include/simulation/ReactionEngine.hpp
namespace noita {

// 反应条件
struct ReactionCondition {
    MaterialID material_a;
    MaterialID material_b;
    
    // 可选条件
    std::optional<float> min_temperature;
    std::optional<float> max_temperature;
    std::optional<float> probability;
    
    // 自动生成哈希键
    struct Hash {
        size_t operator()(const ReactionCondition& c) const {
            size_t h1 = std::hash<MaterialID>{}(std::min(c.material_a, c.material_b));
            size_t h2 = std::hash<MaterialID>{}(std::max(c.material_a, c.material_b));
            return h1 ^ (h2 << 1);
        }
    };
    
    bool operator==(const ReactionCondition& other) const {
        return std::tie(material_a, material_b) == 
               std::tie(other.material_a, other.material_b);
    }
};

// 反应结果
struct ReactionResult {
    MaterialID product_a;
    MaterialID product_b;
    float temperature_change = 0.0f;
    float energy_release = 0.0f;
    bool triggers_chain = false;
    std::string chain_reaction_id;
};

// 反应规则
struct ReactionRule {
    std::string id;
    ReactionCondition condition;
    ReactionResult result;
    
    // 链式反应
    std::vector<std::string> follow_up_reactions;
};

class ReactionEngine {
private:
    // 快速查找表 O(1)
    std::unordered_map<ReactionCondition, 
                       ReactionResult, 
                       ReactionCondition::Hash> reaction_map_;
    
    // 链式反应图
    std::unordered_map<std::string, std::vector<ReactionRule>> chain_graph_;
    
    // 反应队列 (异步处理)
    struct PendingReaction {
        int x, y;
        ReactionResult result;
    };
    std::vector<PendingReaction> reaction_queue_;
    
public:
    // 注册反应
    void register_reaction(const ReactionRule& rule);
    void load_from_json(const std::string& path);
    
    // 检查反应 (O(1))
    std::optional<ReactionResult> check_reaction(
        MaterialID a, MaterialID b, float temperature) const;
    
    // 批量处理
    void process_reactions(World& world, float dt);
    
    // 链式反应
    void trigger_chain_reaction(const std::string& chain_id, 
                                 int x, int y, World& world);
    
    // 单例
    static ReactionEngine& instance();
};

} // namespace noita
```

**JSON反应配置**:

```json
{
  "reactions": [
    {
      "id": "water_lava",
      "condition": {
        "materials": ["water", "lava"],
        "probability": 0.8
      },
      "result": {
        "product_a": "steam",
        "product_b": "rock",
        "temperature_change": -200.0,
        "energy_release": 50.0,
        "triggers_chain": true,
        "chain_reaction_id": "steam_explosion"
      }
    },
    {
      "id": "fire_wood",
      "condition": {
        "materials": ["fire", "wood"],
        "min_temperature": 300.0
      },
      "result": {
        "product_a": "fire",
        "product_b": "ash",
        "temperature_change": 100.0,
        "energy_release": 20.0
      }
    }
  ],
  "chain_reactions": {
    "steam_explosion": [
      {
        "id": "steam_expand",
        "condition": {"materials": ["steam", "any"]},
        "result": {
          "product_a": "steam",
          "product_b": "empty",
          "energy_release": 10.0
        }
      }
    ]
  }
}
```

---

### 3. 地形生成系统重构

#### 3.1 Herringbone Wang Tiles实现

**当前问题**:
- 仅4-6种基础tile
- 缺乏Herringbone布局
- 生成算法简单

**新设计方案**:

```cpp
// include/procedural/HerringboneWangTiles.hpp
namespace noita {

// Wang Tile边缘类型
enum class EdgeType : uint8_t {
    Solid = 0,
    Path,
    Half,
    Water,
    Lava,
    Custom
};

// Wang Tile定义
struct WangTile {
    std::string id;
    std::array<EdgeType, 4> edges;  // N, E, S, W
    
    // 像素模式 (16x16)
    static constexpr int TILE_SIZE = 16;
    std::array<MaterialID, TILE_SIZE * TILE_SIZE> pattern;
    
    // 元数据
    int weight = 1;  // 选择权重
    std::vector<std::string> tags;
    
    // 边缘匹配
    bool matches(EdgeType my_edge, EdgeType other_edge) const;
};

// Herringbone布局
class HerringboneLayout {
public:
    struct Cell {
        int x, y;
        bool is_horizontal;  // Herringbone方向
        WangTile* tile = nullptr;
    };
    
private:
    std::vector<std::vector<Cell>> grid_;
    int width_, height_;
    
public:
    HerringboneLayout(int width, int height);
    
    // Herringbone坐标转换
    void cell_to_world(int cx, int cy, int& wx, int& wy) const;
    void world_to_cell(int wx, int wy, int& cx, int& cy) const;
    
    // Tile访问
    Cell& get_cell(int cx, int cy);
    const Cell& get_cell(int cx, int cy) const;
};

// Wang Tile系统
class WangTileSystem {
private:
    std::vector<WangTile> tiles_;
    std::unordered_map<std::string, WangTile*> tile_map_;
    
    // 边缘约束索引
    std::unordered_map<std::array<EdgeType, 4>, 
                       std::vector<WangTile*>> edge_index_;
    
    HerringboneLayout layout_;
    
public:
    WangTileSystem(int world_width, int world_height);
    
    // Tile管理
    void load_tiles_from_json(const std::string& path);
    void add_tile(const WangTile& tile);
    
    // 生成算法
    void generate_wfc();  // Wave Function Collapse
    void generate_simple();  // 简单约束填充
    
    // Tile查找
    std::vector<WangTile*> find_matching_tiles(
        const std::array<std::optional<EdgeType>, 4>& constraints) const;
    
    // 应用到世界
    void apply_to_world(World& world);
    
private:
    // WFC算法核心
    class WaveFunctionCollapse {
        struct CellState {
            std::set<WangTile*> possible_tiles;
            bool collapsed = false;
            double entropy = 0.0;
        };
        
        std::vector<std::vector<CellState>> grid_;
        
    public:
        void initialize(const std::vector<WangTile*>& tiles);
        bool iterate();
        void propagate(int cx, int cy);
        WangTile* collapse(int cx, int cy);
        int find_min_entropy_cell();
    };
};

} // namespace noita
```

**JSON Tile配置**:

```json
{
  "tiles": [
    {
      "id": "floor_basic",
      "edges": ["path", "path", "path", "path"],
      "pattern": "empty",
      "weight": 10,
      "tags": ["floor", "basic"]
    },
    {
      "id": "floor_cracked",
      "edges": ["path", "path", "path", "path"],
      "pattern": {
        "base": "empty",
        "decorations": [
          {"x": 5, "y": 8, "material": "cave_rock"},
          {"x": 10, "y": 12, "material": "cave_rock"}
        ]
      },
      "weight": 3,
      "tags": ["floor", "damaged"]
    },
    {
      "id": "wall_solid",
      "edges": ["solid", "solid", "solid", "solid"],
      "pattern": "rock",
      "weight": 5,
      "tags": ["wall", "solid"]
    },
    {
      "id": "corridor_ns",
      "edges": ["path", "solid", "path", "solid"],
      "pattern": {
        "base": "empty",
        "walls": {
          "left": "rock",
          "right": "rock"
        }
      },
      "weight": 2,
      "tags": ["corridor", "vertical"]
    }
  ]
}
```

#### 3.2 生物群系系统

```cpp
// include/procedural/BiomeSystem.hpp
namespace noita {

struct BiomeLayer {
    std::string name;
    int depth_start, depth_end;
    
    // 材料分布
    struct MaterialDistribution {
        MaterialID material;
        float probability;
        int cluster_size;
    };
    std::vector<MaterialDistribution> materials;
    
    // 结构生成
    struct Structure {
        std::string id;
        float probability;
        int min_distance;
    };
    std::vector<Structure> structures;
    
    // 敌人生成
    struct EnemySpawn {
        std::string enemy_type;
        float probability;
        int min_count, max_count;
    };
    std::vector<EnemySpawn> enemies;
};

class BiomeSystem {
private:
    std::vector<BiomeLayer> layers_;
    std::unordered_map<std::string, BiomeLayer*> layer_map_;
    
public:
    void load_from_json(const std::string& path);
    
    // 查询
    const BiomeLayer& get_layer_at_depth(int depth) const;
    
    // 生成
    void generate_layer(World& world, int depth_start, int depth_end);
    void place_structures(World& world, const BiomeLayer& layer);
    void spawn_enemies(World& world, const BiomeLayer& layer);
};

} // namespace noita
```

---

### 4. 渲染系统重构

#### 4.1 OpenGL后端

**当前问题**:
- 软件渲染,CPU拷贝1MB/帧
- 无GPU加速
- 缺乏后处理效果

**新设计方案**:

```cpp
// include/render/OpenGLRenderer.hpp
namespace noita {

class OpenGLRenderer {
private:
    // 窗口
    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    
    // 像素纹理
    GLuint pixel_texture_ = 0;
    int texture_width_ = 0, texture_height_ = 0;
    
    // 着色器
    struct ShaderProgram {
        GLuint program;
        GLuint vertex_shader;
        GLuint fragment_shader;
        
        // Uniform位置
        GLint camera_pos_loc;
        GLint zoom_loc;
        GLint screen_size_loc;
    };
    
    ShaderProgram main_shader_;
    ShaderProgram post_process_shader_;
    
    // 帧缓冲
    GLuint fbo_ = 0;
    GLuint color_texture_ = 0;
    
    // 顶点缓冲
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    
    // 相机
    struct Camera {
        float x = 0.0f, y = 0.0f;
        float target_x = 0.0f, target_y = 0.0f;
        float zoom = 1.0f;
        float smoothing = 0.1f;
        
        // 震动
        float shake_intensity = 0.0f;
        float shake_duration = 0.0f;
        
        void update(float dt);
        void trigger_shake(float intensity, float duration);
    } camera_;
    
public:
    OpenGLRenderer(int width, int height, const std::string& title);
    ~OpenGLRenderer();
    
    // 初始化
    void initialize();
    void load_shaders();
    
    // 渲染
    void begin_frame();
    void render_world(const World& world);
    void end_frame();
    
    // 相机控制
    void set_camera_target(float x, float y);
    void set_zoom(float zoom);
    void shake_camera(float intensity, float duration);
    
    // 后处理
    void enable_bloom(bool enable);
    void enable_color_correction(bool enable);
    
private:
    void update_pixel_texture(const World& world);
    void render_fullscreen_quad();
    void apply_post_processing();
};

} // namespace noita
```

**主着色器** (`shaders/pixel.vert`, `shaders/pixel.frag`):

```glsl
// pixel.vert
#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texcoord;

out vec2 v_texcoord;

uniform vec2 u_camera_pos;
uniform float u_zoom;
uniform vec2 u_screen_size;

void main() {
    vec2 world_pos = a_position * u_zoom + u_camera_pos;
    vec2 screen_pos = world_pos / u_screen_size * 2.0 - 1.0;
    gl_Position = vec4(screen_pos, 0.0, 1.0);
    v_texcoord = a_texcoord;
}

// pixel.frag
#version 330 core
in vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_pixel_texture;

void main() {
    vec4 color = texture(u_pixel_texture, v_texcoord);
    
    // 颜色校正
    color.rgb = pow(color.rgb, vec3(1.0 / 2.2));
    
    // 边缘平滑
    if (color.a < 0.1) discard;
    
    frag_color = color;
}
```

#### 4.2 Sprite Batching

```cpp
// include/render/SpriteBatch.hpp
namespace noita {

struct SpriteVertex {
    float x, y;          // 位置
    float u, v;          // 纹理坐标
    uint8_t r, g, b, a;  // 颜色
};

class SpriteBatch {
private:
    static constexpr int MAX_SPRITES = 10000;
    static constexpr int VERTEX_PER_SPRITE = 6;
    
    std::vector<SpriteVertex> vertices_;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint texture_ = 0;
    
    OpenGLRenderer* renderer_;
    
public:
    SpriteBatch(OpenGLRenderer* renderer);
    ~SpriteBatch();
    
    // 绘制API
    void draw_rect(float x, float y, float w, float h, 
                   uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void draw_sprite(float x, float y, float w, float h,
                     float u0, float v0, float u1, float v1);
    void draw_circle(float x, float y, float radius, 
                     uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    
    // 批处理
    void begin();
    void end();
    void flush();
    
    // 统计
    int get_draw_calls() const { return draw_calls_; }
    int get_sprite_count() const { return vertices_.size() / 6; }
    
private:
    int draw_calls_ = 0;
};

} // namespace noita
```

---

### 5. 多线程系统重构

#### 5.1 Work-Stealing线程池

**当前问题**:
- 无任务窃取,负载不均衡
- 忙等待浪费CPU
- 无优先级系统

**新设计方案**:

```cpp
// include/threading/WorkStealingPool.hpp
namespace noita {

class WorkStealingPool {
private:
    // 工作窃取队列
    class WorkStealingQueue {
    private:
        std::deque<std::function<void()>> queue_;
        mutable std::mutex mutex_;
        
    public:
        void push(std::function<void()>&& task) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(task));
        }
        
        std::function<void()> pop() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) return nullptr;
            auto task = std::move(queue_.front());
            queue_.pop_front();
            return task;
        }
        
        std::function<void()> steal() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) return nullptr;
            auto task = std::move(queue_.back());
            queue_.pop_back();
            return task;
        }
        
        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }
    };
    
    std::vector<std::unique_ptr<WorkStealingQueue>> local_queues_;
    std::vector<std::thread> workers_;
    
    std::atomic<bool> stop_{false};
    std::atomic<int> active_tasks_{0};
    
    std::mutex completion_mutex_;
    std::condition_variable completion_cv_;
    
    // 线程本地存储
    static thread_local int thread_id_;
    static thread_local WorkStealingQueue* local_queue_;
    
public:
    explicit WorkStealingPool(int thread_count = -1);
    ~WorkStealingPool();
    
    // 提交任务
    template<typename F>
    void enqueue(F&& task) {
        int id = thread_id_;
        if (id >= 0 && id < local_queues_.size()) {
            local_queues_[id]->push(std::forward<F>(task));
        } else {
            // 随机分配
            int target = rand() % local_queues_.size();
            local_queues_[target]->push(std::forward<F>(task));
        }
        active_tasks_++;
    }
    
    // 等待所有任务完成
    void wait_all() {
        std::unique_lock<std::mutex> lock(completion_mutex_);
        completion_cv_.wait(lock, [this]() {
            return active_tasks_ == 0;
        });
    }
    
    // 并行for循环
    template<typename F>
    void parallel_for(int start, int end, F&& func) {
        int range = end - start;
        int chunk_size = range / local_queues_.size();
        
        for (size_t i = 0; i < local_queues_.size(); ++i) {
            int chunk_start = start + i * chunk_size;
            int chunk_end = (i == local_queues_.size() - 1) ? end : chunk_start + chunk_size;
            
            enqueue([chunk_start, chunk_end, &func]() {
                for (int i = chunk_start; i < chunk_end; ++i) {
                    func(i);
                }
            });
        }
        
        wait_all();
    }
    
private:
    void worker_thread(int id);
    std::function<void()> get_task();
};

} // namespace noita
```

#### 5.2 任务图调度器

```cpp
// include/threading/TaskGraph.hpp
namespace noita {

class TaskGraph {
public:
    using TaskID = int;
    
private:
    struct Task {
        std::function<void()> func;
        std::vector<TaskID> dependencies;
        std::vector<TaskID> dependents;
        std::atomic<int> dependency_count{0};
        bool completed = false;
    };
    
    std::vector<Task> tasks_;
    std::queue<TaskID> ready_queue_;
    std::mutex queue_mutex_;
    
    WorkStealingPool& pool_;
    
public:
    explicit TaskGraph(WorkStealingPool& pool);
    
    // 添加任务
    TaskID add_task(std::function<void()> func);
    
    // 添加依赖
    void add_dependency(TaskID dependent, TaskID dependency);
    
    // 执行
    void execute();
    
    // 清空
    void clear();
    
private:
    void on_task_completed(TaskID id);
    void schedule_ready_tasks();
};

} // namespace noita
```

**使用示例**:

```cpp
// 构建任务图
TaskGraph graph(pool);

auto update_solids = graph.add_task([&]() {
    world.update_solids(dt);
});

auto update_liquids = graph.add_task([&]() {
    world.update_liquids(dt);
});

auto update_gases = graph.add_task([&]() {
    world.update_gases(dt);
});

auto update_reactions = graph.add_task([&]() {
    reaction_engine.process_reactions(world, dt);
});

// 设置依赖: 反应必须在物理更新后
graph.add_dependency(update_reactions, update_solids);
graph.add_dependency(update_reactions, update_liquids);
graph.add_dependency(update_reactions, update_gases);

// 执行
graph.execute();
```

---

## ⚡ 性能优化策略

### 1. 并行算法优化

#### 1.1 棋盘格并行改进

**当前问题**: 4-pass系统同步开销大

**新方案**: 8-pass + 双缓冲

```cpp
void World::update_parallel(float dt) {
    // 8-pass系统,减少每pass的chunk数量
    for (int pass = 0; pass < 8; ++pass) {
        thread_pool_->parallel_for(0, chunks_.size(), [&](int i) {
            if (should_update_in_pass(i, pass)) {
                chunks_[i]->update(dt);
            }
        });
        thread_pool_->wait_all();
    }
    
    // 同步边缘
    for (auto& chunk : chunks_) {
        chunk->sync_edges();
    }
    
    // 交换缓冲
    for (auto& chunk : chunks_) {
        chunk->swap_buffers();
    }
}
```

#### 1.2 流体求解器并行化

**Jacobi迭代并行化**:

```cpp
void World::update_liquids_parallel(float dt) {
    // Jacobi迭代求解压力
    for (int iter = 0; iter < 4; ++iter) {
        thread_pool_->parallel_for(0, chunks_.size(), [&](int i) {
            update_liquid_chunk_jacobi(chunks_[i].get(), iter);
        });
        thread_pool_->wait_all();
    }
    
    // 应用速度场
    thread_pool_->parallel_for(0, chunks_.size(), [&](int i) {
        apply_velocity_field(chunks_[i].get());
    });
    thread_pool_->wait_all();
}
```

### 2. SIMD优化

```cpp
// include/core/SIMD.hpp
#include <immintrin.h>

namespace noita {

// AVX2批量更新8个像素
void update_pixels_avx2(Pixel* pixels, int count) {
    for (int i = 0; i < count; i += 8) {
        // 加载8个像素的材料ID
        __m256i materials = _mm256_loadu_si256(
            reinterpret_cast<__m256i*>(&pixels[i].material)
        );
        
        // 批量比较
        __m256i is_liquid = _mm256_cmpeq_epi8(
            materials, _mm256_set1_epi8(static_cast<uint8_t>(MaterialState::Liquid))
        );
        
        // 批量处理液体
        // ...
    }
}

} // namespace noita
```

### 3. 缓存优化

```cpp
// 确保Pixel结构缓存友好
static_assert(sizeof(Pixel) == 16, "Pixel size mismatch");
static_assert(alignof(Pixel) == 16, "Pixel alignment mismatch");

// Chunk大小选择
constexpr int CHUNK_SIZE = 128;  // 128x128 = 16KB (L1缓存友好)

// 预取优化
void World::update_chunk(Chunk* chunk) {
    // 预取下一个chunk
    if (chunk->neighbors[EAST]) {
        _mm_prefetch(chunk->neighbors[EAST]->pixels.data(), _MM_HINT_T0);
    }
    
    // 更新当前chunk
    for (auto& pixel : chunk->pixels) {
        update_pixel(pixel);
    }
}
```

---

## 📅 实施路线图

### 阶段1: 核心重构 (2-3周)

**目标**: 重构核心架构,保持功能不变

**任务**:
1. ✅ 实现新的Pixel数据结构
2. ✅ 实现MaterialRegistry运行时系统
3. ✅ 重构World为统一Chunk系统
4. ✅ 实现WorkStealingPool
5. ✅ 迁移现有材料到JSON配置

**验收标准**:
- 所有现有功能正常工作
- 性能不低于当前版本
- 代码通过单元测试

### 阶段2: 性能优化 (2-3周)

**目标**: 提升性能2-3倍

**任务**:
1. ✅ 实现双缓冲Chunk系统
2. ✅ 实现并行流体求解器
3. ✅ 优化ReactionEngine为O(1)查找
4. ✅ 实现SIMD优化
5. ✅ 添加性能分析工具

**验收标准**:
- 512x512世界稳定60FPS
- CPU占用降低50%
- 内存占用不变

### 阶段3: 渲染升级 (2-3周)

**目标**: GPU加速渲染

**任务**:
1. ✅ 实现OpenGLRenderer
2. ✅ 实现SpriteBatch
3. ✅ 实现后处理效果
4. ✅ 实现相机平滑和震动
5. ✅ 性能对比测试

**验收标准**:
- 渲染CPU占用降低80%
- 支持后处理效果
- 跨平台兼容

### 阶段4: 地形生成 (2-3周)

**目标**: Noita风格的地形生成

**任务**:
1. ✅ 实现HerringboneWangTiles
2. ✅ 实现WaveFunctionCollapse算法
3. ✅ 实现BiomeSystem
4. ✅ 创建tile资源库(50+ tiles)
5. ✅ 创建生物群系配置

**验收标准**:
- 生成无缝地形
- 支持多种生物群系
- 可配置化

### 阶段5: 高级功能 (3-4周)

**目标**: 完善游戏功能

**任务**:
1. ✅ 实现完整的温度系统
2. ✅ 实现材料状态转换
3. ✅ 实现链式反应系统
4. ✅ 实现光照系统
5. ✅ 实现材料混合

**验收标准**:
- 所有材料交互正常
- 温度模拟真实
- 视觉效果提升

---

## 📊 预期性能提升

| 指标 | 当前 | 目标 | 提升 |
|------|------|------|------|
| 帧率(512x512) | 45 FPS | 60 FPS | +33% |
| CPU占用 | 80% | 40% | -50% |
| 内存占用 | 150MB | 120MB | -20% |
| 启动时间 | 2s | 0.5s | -75% |
| 材料数量 | 60(硬编码) | 无限(JSON) | ∞ |
| Tile数量 | 4-6 | 50+ | +800% |

---

## 🎯 总结

本重构方案基于Noita的核心设计理念,通过以下关键改进:

1. **架构现代化**: ECS + 数据驱动 + 模块化
2. **性能优化**: 并行算法 + SIMD + GPU加速
3. **扩展性提升**: JSON配置 + 运行时注册
4. **真实感增强**: 完整物理 + 温度系统 + 压力系统

预计可将性能提升2-3倍,同时大幅提升代码的可维护性和扩展性,为后续开发奠定坚实基础。
