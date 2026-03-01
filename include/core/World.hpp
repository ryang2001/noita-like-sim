#pragma once

#include "core/Types.hpp"
#include "core/Pixel.hpp"
#include "core/Chunk.hpp"
#include "threading/WorkStealingPool.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <unordered_set>
#include <mutex>

namespace noita {

/**
 * @brief 世界配置
 */
struct WorldConfig {
    int width = 512;
    int height = 512;
    int chunk_size = 64;  // Noita标准
    int thread_count = -1;  // -1 = 自动检测
    bool use_parallel = true;
};

/**
 * @brief 世界类 - 基于Noita的像素物理引擎
 * 
 * 核心技术:
 * 1. 64×64分块系统
 * 2. 4-pass棋盘格并行更新
 * 3. 从下往上的更新顺序
 * 4. 脏矩形优化
 */
class World {
public:
    using PixelCallback = std::function<void(int, int, Pixel&)>;
    using PassCallback = std::function<void(int)>;  // pass完成回调
    
    // 全局访问接口(用于Chunk访问World)
    static World* get_instance() { return instance_; }
    
private:
    static World* instance_;  // 全局实例
    WorldConfig config_;
    
    // 分块系统
    std::vector<std::unique_ptr<Chunk>> chunks_;
    int chunks_x_, chunks_y_;
    
    // 棋盘格更新器
    std::unique_ptr<CheckerboardUpdater> checkerboard_updater_;
    
    // 线程池
    std::unique_ptr<WorkStealingPool> thread_pool_;
    
    // 帧计数器
    std::atomic<uint32_t> frame_counter_{0};
    
    // 活跃区域
    struct ActiveRegion {
        int center_x = 256;
        int center_y = 256;
        int radius = 200;
        
        void update(int player_x, int player_y) {
            center_x = player_x;
            center_y = player_y;
        }
        
        bool contains(int x, int y) const {
            int dx = x - center_x;
            int dy = y - center_y;
            return (dx * dx + dy * dy) <= (radius * radius);
        }
    } active_region_;
    
    // Pass回调
    PassCallback pass_callback_;
    
    // 线程正在更新的chunk(用于debug显示)
    std::atomic<int> active_chunk_x_{-1};
    std::atomic<int> active_chunk_y_{-1};
    
    // 性能统计
    struct Stats {
        std::atomic<int> pixels_updated{0};
        std::atomic<int> chunks_updated{0};
        std::atomic<float> update_time_ms{0.0f};
    } stats_;

public:
    explicit World(const WorldConfig& config = WorldConfig());
    ~World();
    
    // 主更新循环
    void update(float dt);

    // 并行更新(棋盘格模式)
    void update_parallel(float dt);
    void update_serial(float dt);

    // 4-pass棋盘格并行更新(新实现)
    void update_checkerboard(float dt);
    void update_checkerboard_pass(int pass, float dt);
    
    // 像素访问
    Pixel& get(int x, int y);
    const Pixel& get(int x, int y) const;
    void set(int x, int y, const Pixel& pixel);
    void set_material(int x, int y, MaterialID material);
    
    // 块访问
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
    
    // 活跃区域
    void set_active_region(int center_x, int center_y, int radius);
    void update_active_region(int player_x, int player_y);
    
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
    int get_width() const { return config_.width; }
    int get_height() const { return config_.height; }
    int get_chunks_x() const { return chunks_x_; }
    int get_chunks_y() const { return chunks_y_; }
      int get_chunk_count_x() const { return chunks_x_; }
      int get_chunk_count_y() const { return chunks_y_; }
    
    // 性能统计
    const Stats& get_stats() const { return stats_; }
    void reset_stats();
    
    // Pass回调
    void set_pass_callback(PassCallback callback) { pass_callback_ = callback; }
    
    // 获取线程正在更新的chunk
    int get_active_chunk_x() const { return active_chunk_x_.load(); }
    int get_active_chunk_y() const { return active_chunk_y_.load(); }
    
    // 序列化
    void save(const std::string& path);
    void load(const std::string& path);
    
private:
    // 初始化
    void initialize_chunks();
    void connect_chunk_neighbors();
    
    // 更新辅助
    void update_chunk_pass(int pass, float dt);
    void sync_all_borders();
    
    // 生成辅助
    float noise(int x, int y, float scale);
    void apply_material_with_variation(int x, int y, MaterialID material);
};

} // namespace noita
