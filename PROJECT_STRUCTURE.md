# Noita-like Pixel Physics Engine - 项目结构文档

## 📁 项目概览

**项目名称**: Noita-like Pixel Physics Engine
**项目类型**: 像素物理引擎模拟
**技术栈**: C++17, SDL2, CMake, MinGW/Visual Studio

## 🏗️ 目录结构

```
sim/
├── include/                    # 头文件
│   ├── core/                  # 核心系统
│   │   ├── Chunk.hpp         # 块系统定义
│   │   ├── Pixel.hpp         # 像素定义
│   │   ├── Types.hpp         # 类型定义
│   │   └── World.hpp         # 世界系统定义
│   ├── materials/            # 材料系统
│   │   └── MaterialRegistry.hpp  # 材料注册表
│   └── threading/            # 线程系统
│       └── WorkStealingPool.hpp  # 工作窃取线程池
│
├── src/                       # 源文件
│   ├── core/                 # 核心实现
│   │   ├── Chunk.cpp        # 块系统实现
│   │   └── World.cpp        # 世界系统实现
│   ├── materials/           # 材料实现
│   │   └── MaterialRegistry.cpp  # 材料注册表实现
│   ├── threading/           # 线程实现
│   │   └── WorkStealingPool.cpp  # 线程池实现
│   └── demo_simple.cpp      # 主程序
│
├── config/                   # 配置文件
│   ├── materials/           # 材料配置
│   │   └── basic_materials.json  # 基础材料定义
│   └── reactions/           # 反应配置
│       └── basic_reactions.json  # 基础反应定义
│
├── external/                 # 外部依赖
│   ├── SDL2-2.30.1/        # SDL2库
│   └── box2d/              # Box2D物理引擎(备用)
│
├── tests/                    # 测试文件
│   ├── auto_test.cpp       # 自动测试
│   ├── compile_check.cpp   # 编译检查
│   ├── test_core.cpp       # 核心测试
│   ├── verify_headers.cpp  # 头文件验证
│   └── verify_headers2.cpp # 头文件验证2
│
├── assets/                  # 资产文件
├── build/                   # Visual Studio构建目录
├── build_mingw/            # MinGW构建目录
├── build_demo/             # Demo构建目录
├── CMakeLists.txt          # CMake配置文件
└── README.md              # 项目说明
```

## 🎯 核心模块

### 1. Chunk系统 (块系统)

**位置**: `include/core/Chunk.hpp`, `src/core/Chunk.cpp`

**功能**:
- 64×64像素的有效区域
- 32像素的边界扩展(上下左右)
- 128×128的总大小
- 棋盘格4-pass更新
- 边界同步

**关键类**:
```cpp
class Chunk {
public:
    static constexpr int SIZE = 64;        // 有效区域大小
    static constexpr int BORDER = 32;      // 边界扩展大小

    // 像素访问
    Pixel& get(int local_x, int local_y);
    Pixel& get_world(int world_x, int world_y);
    void set(int local_x, int local_y, const Pixel& pixel);

    // 更新
    void update(uint32_t frame, int pass);
    void sync_borders();

    // 邻居
    void set_neighbor(int direction, Chunk* neighbor);
    Chunk* get_neighbor(int direction) const;

    // 调试
    bool was_updated_this_frame() const;
    void clear_updated_flag();

private:
    std::array<Pixel, (SIZE + BORDER * 2) * (SIZE + BORDER * 2)> pixels_;
    int chunk_x_, chunk_y_;
    int world_x_, world_y_;
    std::atomic<uint32_t> last_update_frame_{0};
    bool updated_this_frame_ = false;
    Chunk* neighbors_[4] = {nullptr};

    // 更新辅助
    void update_pixel(int local_x, int local_y, int pass);
    bool is_in_pass(int local_x, int local_y, int pass) const;
    void update_powder(int local_x, int local_y);
    void update_liquid(int local_x, int local_y);
    void update_gas(int local_x, int local_y);
    void update_fire(int local_x, int local_y);
    void update_temperature(int local_x, int local_y);
    bool try_move(int from_x, int from_y, int to_x, int to_y);
};
```

**棋盘格更新器**:
```cpp
class CheckerboardUpdater {
public:
    static constexpr int PASS_COUNT = 4;

    std::vector<int> get_chunks_for_pass(int pass) const;
    bool should_update_chunk(int chunk_x, int chunk_y, int pass) const;
};
```

### 2. World系统 (世界系统)

**位置**: `include/core/World.hpp`, `src/core/World.cpp`

**功能**:
- 管理所有Chunk
- 棋盘格4-pass更新调度
- 边界同步
- 性能统计
- Pass回调

**关键类**:
```cpp
class World {
public:
    using PixelCallback = std::function<void(int, int, Pixel&)>;
    using PassCallback = std::function<void(int)>;

    // 全局访问
    static World* get_instance();

    // 主更新循环
    void update(float dt);
    void update_parallel(float dt);
    void update_serial(float dt);

    // 像素访问
    Pixel& get(int x, int y);
    const Pixel& get(int x, int y) const;
    void set(int x, int y, const Pixel& pixel);
    void set_material(int x, int y, MaterialID material);

    // Chunk访问
    Chunk* get_chunk(int x, int y);
    const Chunk* get_chunk(int x, int y) const;
    Chunk* get_chunk_by_index(int cx, int cy);
    const Chunk* get_chunk_by_index(int cx, int cy) const;

    // 世界操作
    void clear();
    void fill(MaterialID material);
    void fill_rect(int x, int y, int w, int h, MaterialID material);
    void fill_circle(int cx, int cy, int radius, MaterialID material);

    // 程序化生成
    void generate_caves();
    void generate_surface();
    void generate_biome(int biome_type);

    // 遍历
    void for_each_pixel(PixelCallback callback);
    void for_each_chunk(std::function<void(Chunk&)> callback);

    // 坐标转换
    void world_to_chunk(int world_x, int world_y, int& chunk_x, int& chunk_y) const;
    void chunk_to_world(int chunk_x, int chunk_y, int& world_x, int& world_y) const;

    // 边界检查
    bool is_valid(int x, int y) const;
    bool is_in_bounds(int x, int y) const;

    // 尺寸
    int get_width() const;
    int get_height() const;
    int get_chunks_x() const;
    int get_chunks_y() const;

    // 性能统计
    const Stats& get_stats() const;
    void reset_stats();

    // Pass回调
    void set_pass_callback(PassCallback callback);

    // 获取线程正在更新的chunk
    int get_active_chunk_x() const;
    int get_active_chunk_y() const;

    // 序列化
    void save(const std::string& path);
    void load(const std::string& path);

private:
    WorldConfig config_;
    std::vector<std::unique_ptr<Chunk>> chunks_;
    int chunks_x_, chunks_y_;
    std::unique_ptr<CheckerboardUpdater> checkerboard_updater_;
    std::unique_ptr<WorkStealingPool> thread_pool_;
    std::atomic<uint32_t> frame_counter_{0};
    PassCallback pass_callback_;
    std::atomic<int> active_chunk_x_{-1};
    std::atomic<int> active_chunk_y_{-1};

    struct ActiveRegion { ... } active_region_;
    struct Stats { ... } stats_;
};
```

### 3. Pixel系统 (像素系统)

**位置**: `include/core/Pixel.hpp`

**功能**:
- 存储像素数据
- 材料ID
- 状态(固体/粉末/液体/气体/等离子体/能量)
- 温度
- 生命周期
- 速度
- 标志

**关键结构**:
```cpp
struct Pixel {
    MaterialID material = EMPTY_MATERIAL;
    MaterialState state = MaterialState::Empty;
    float temperature = 20.0f;
    int lifetime = 0;
    float velocity_x = 0.0f;
    float velocity_y = 0.0f;
    uint32_t flags = 0;

    bool is_empty() const;
    bool is_solid() const;
    bool is_powder() const;
    bool is_liquid() const;
    bool is_gas() const;
    bool is_plasma() const;
    bool is_energy() const;
};
```

### 4. MaterialRegistry系统 (材料注册表)

**位置**: `include/materials/MaterialRegistry.hpp`, `src/materials/MaterialRegistry.cpp`

**功能**:
- 材料注册和管理
- 从JSON加载材料
- 材料属性查询
- 反应系统

**关键类**:
```cpp
class MaterialRegistry {
public:
    static MaterialRegistry& instance();

    void load_from_json(const std::string& path);
    MaterialID register_material(const MaterialProperties& props);
    MaterialID get_id(const std::string& name) const;
    const MaterialProperties& get(MaterialID id) const;
    const std::string& get_name(MaterialID id) const;

    // 反应
    void register_reaction(const Reaction& reaction);
    bool check_reaction(int x, int y, const Pixel& pixel);

private:
    std::vector<MaterialProperties> materials_;
    std::unordered_map<std::string, MaterialID> name_to_id_;
    std::vector<Reaction> reactions_;
};
```

**材料属性**:
```cpp
struct MaterialProperties {
    std::string name;
    MaterialState state;
    float density;
    float flammability;
    float melting_point;
    float freezing_point;
    float boiling_point;
    uint32_t color;
    float conductivity;
    float viscosity;
    // ...
};
```

### 5. WorkStealingPool系统 (工作窃取线程池)

**位置**: `include/threading/WorkStealingPool.hpp`, `src/threading/WorkStealingPool.cpp`

**功能**:
- 工作窃取算法
- 负载均衡
- 并行更新

**关键类**:
```cpp
class WorkStealingPool {
public:
    using Task = std::function<void()>;

    explicit WorkStealingPool(int num_threads);
    ~WorkStealingPool();

    std::future<void> enqueue(Task&& task);

private:
    class WorkStealingQueue { ... };

    std::vector<std::thread> threads_;
    std::vector<std::unique_ptr<WorkStealingQueue>> queues_;
    std::atomic<bool> stop_{false};
};
```

## 🎮 主程序 (demo_simple.cpp)

**位置**: `src/demo_simple.cpp`

**功能**:
- SDL2初始化
- 主循环
- 渲染
- 输入处理
- Debug功能

**全局变量**:
```cpp
World* world;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* world_texture;

Camera camera;
PerfStats perf_stats;

MaterialID current_material;
int brush_size;
bool debug_mode;
bool show_chunk_borders;
bool show_chunk_updates;
bool show_thread_activity;
bool show_fps;
bool paused;
bool render_per_pass;
float target_fps;
```

**主要函数**:
```cpp
bool init_sdl();
void cleanup_sdl();
void render_world();
void handle_input(bool& running);
void init_material_list();
```

**Debug功能**:
- F1: 切换debug模式
- F2: 显示chunk边界
- F3: 显示FPS
- F4: 显示chunk更新状态(绿色=已更新,红色=未更新)
- F5: 每个pass渲染(显示4-pass更新过程)
- F6: 显示线程活动(黄色=正在更新)
- F7: 降低FPS
- F8: 提高FPS

## 🔧 构建系统

### CMakeLists.txt

**关键配置**:
```cmake
cmake_minimum_required(VERSION 3.15)
project(NoitaPixelPhysics CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL2
set(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-2.30.1")
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# 源文件
add_executable(demo
    src/demo_simple.cpp
    src/core/Chunk.cpp
    src/core/World.cpp
    src/materials/MaterialRegistry.cpp
    src/threading/WorkStealingPool.cpp
)

# 链接
target_link_libraries(demo PRIVATE ${SDL2_LIBRARIES} Threads::Threads)

# Windows特定
if(WIN32)
    target_compile_definitions(demo PRIVATE SDL_MAIN_HANDLED)
    list(APPEND LINK_LIBS opengl32 gdi32 winmm)
endif()
```

### 构建方式

**Visual Studio**:
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

**MinGW**:
```bash
mkdir build_mingw && cd build_mingw
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc
mingw32-make -j8
```

## 📊 配置文件

### basic_materials.json

**材料定义**:
```json
{
    "materials": [
        {
            "name": "empty",
            "state": "empty",
            "density": 0.0,
            "color": "0x00000000"
        },
        {
            "name": "sand",
            "state": "powder",
            "density": 1.5,
            "flammability": 0.0,
            "melting_point": 1700.0,
            "color": "0xFFE6C229"
        },
        {
            "name": "water",
            "state": "liquid",
            "density": 1.0,
            "flammability": 0.0,
            "melting_point": 0.0,
            "freezing_point": 0.0,
            "boiling_point": 100.0,
            "color": "0xFF3B82F6"
        }
        // ... 更多材料
    ]
}
```

### basic_reactions.json

**反应定义**:
```json
{
    "reactions": [
        {
            "material": "fire",
            "target": "wood",
            "result": "fire",
            "probability": 0.1
        },
        {
            "material": "water",
            "target": "fire",
            "result": "steam",
            "probability": 1.0
        }
        // ... 更多反应
    ]
}
```

## 🎨 核心算法

### 1. 棋盘格4-pass更新

**原理**:
- 使用`(chunk_x + chunk_y) % 4`决定更新顺序
- 每个pass更新1/4的chunk
- 防止同一像素被多次更新

**代码**:
```cpp
for (int pass = 0; pass < 4; ++pass) {
    auto chunk_indices = checkerboard_updater_->get_chunks_for_pass(pass);
    for (int idx : chunk_indices) {
        chunks_[idx]->update(frame, pass);
    }
}
```

### 2. 元胞自动机规则

**粉末**:
```
1. 尝试向下移动
2. 尝试向左下/右下移动(随机)
```

**液体**:
```
1. 尝试向下移动
2. 尝试向左下/右下移动(随机)
3. 尝试向左/右移动(随机)
```

**气体**:
```
1. 尝试向上移动
2. 尝试向左上/右上移动(随机)
3. 尝试向左/右移动(随机)
```

**火焰**:
```
1. 减少生命周期
2. 点燃周围可燃材料
3. 向上飘动
4. 与水反应生成蒸汽
```

### 3. 边界同步

**原理**:
- 每个chunk有4个邻居(北东南西)
- 边界扩展区的像素同步到邻居
- 双向同步保证数据一致性

**代码**:
```cpp
void Chunk::sync_borders() {
    if (neighbors_[0]) {  // 北
        for (int x = 0; x < SIZE; ++x) {
            Pixel& my_border = get(x, -1);
            if (!my_border.is_empty()) {
                Pixel& neighbor_valid = neighbors_[0]->get(x, SIZE - 1);
                if (neighbor_valid.is_empty()) {
                    neighbor_valid = my_border;
                    my_border = Pixel();
                }
            }
        }
    }
    // ... 其他邻居
}
```

## 🐛 调试功能

### 1. Chunk边界显示

**F2键**: 显示所有chunk的边界

**颜色**:
- 绿色: 默认
- 红色: 未更新(F4开启时)
- 绿色: 已更新(F4开启时)

### 2. Chunk更新状态显示

**F4键**: 显示每个chunk是否在当前帧被更新

**颜色**:
- 绿色: 已更新
- 红色: 未更新

### 3. 线程活动显示

**F6键**: 显示线程正在更新的chunk

**颜色**:
- 黄色: 线程正在更新

### 4. Pass渲染

**F5键**: 每个pass完成后立即渲染

**效果**:
- 可以看到4-pass的更新过程
- 每个pass之间有50ms延迟

### 5. FPS控制

**F7键**: 降低FPS(最小10)
**F8键**: 提高FPS(最大120)

## 📈 性能优化

### 1. 工作窃取线程池

**原理**:
- 每个线程有自己的任务队列
- 空闲线程可以从其他线程窃取任务
- 负载均衡

### 2. 棋盘格更新

**原理**:
- 减少线程竞争
- 避免同一像素被多次更新
- 提高缓存命中率

### 3. 边界扩展

**原理**:
- 减少跨块访问
- 批量边界同步
- 提高局部性

## 🔮 扩展方向

### 1. 更多材料

- 石油
- 熔岩
- 雪
- 冰
- 油漆
- 酸
- 毒药
- 粘液
- 血液

### 2. 更多反应

- 燃烧系统
- 腐蚀系统
- 中和反应
- 聚合反应

### 3. 物理效果

- 重力
- 压力
- 浮力
- 粘度
- 表面张力

### 4. 游戏机制

- 玩家角色
- 物品系统
- 魔法系统
- 生物系统

## 📝 总结

**项目特点**:
- 严格遵循Noita的设计理念
- 64×64块+32边界扩展
- 棋盘格4-pass更新
- 简单的元胞自动机规则
- 完整的调试系统
- 可调FPS
- 线程活动显示

**技术亮点**:
- 高性能并行更新
- 工作窃取线程池
- 边界同步机制
- 温度系统
- 反应系统

**应用场景**:
- 游戏开发
- 物理模拟
- 科学可视化
- 教育演示

项目完整结构整理完毕! 🎯
