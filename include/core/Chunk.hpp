#pragma once

#include "core/Types.hpp"
#include "core/Pixel.hpp"
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace noita {

/**
 * @brief 分块系统 - 严格遵循Noita架构
 * 
 * 核心设计:
 * 1. 每个块64×64像素(有效区域)
 * 2. 32像素边界扩展(用于跨块移动)
 * 3. 总大小: 128×128像素
 * 4. 棋盘格4-pass更新
 * 5. 简单的元胞自动机规则
 */
class Chunk {
public:
    // Noita标准: 64×64像素块
    static constexpr int SIZE = 64;
    static constexpr int BORDER = 32;  // 边界扩展
    
private:
    // 像素数据 (包含边界扩展)
    // 实际大小: (64 + 32*2) × (64 + 32*2) = 128×128
    std::array<Pixel, (SIZE + BORDER * 2) * (SIZE + BORDER * 2)> pixels_;
    
    // 块坐标
    int chunk_x_, chunk_y_;
    
    // 世界坐标偏移
    int world_x_, world_y_;
    
    // 更新帧计数(防止多次更新)
    std::atomic<uint32_t> last_update_frame_{0};
    std::atomic<int> last_update_pass_{-1};
    
    // 调试: 是否在当前帧被更新
    bool updated_this_frame_ = false;
    
    // 同步标志(避免重复同步)
    std::atomic<bool> synced_this_frame_{false};
    
    // 邻居指针(用于边界同步)
    Chunk* neighbors_[4] = {nullptr};  // N, E, S, W
    
public:
    Chunk(int cx, int cy);
    ~Chunk() = default;
    
    // 像素访问(本地坐标: -32到95)
    Pixel& get(int local_x, int local_y);
    const Pixel& get(int local_x, int local_y) const;
    
    // 有效区域访问(本地坐标: 0到63)
    Pixel& get_valid(int local_x, int local_y);
    const Pixel& get_valid(int local_x, int local_y) const;
    
    // 世界坐标访问
    Pixel& get_world(int world_x, int world_y);
    const Pixel& get_world(int world_x, int world_y) const;
    
    // 设置像素
    void set(int local_x, int local_y, const Pixel& pixel);
    void set_world(int world_x, int world_y, const Pixel& pixel);
    
    // 更新(棋盘格pass)
    void update(uint32_t frame, int pass);
    
    // 边界同步(在所有pass完成后)
    void sync_borders();
    
    // 坐标转换
    void local_to_world(int local_x, int local_y, int& world_x, int& world_y) const;
    void world_to_local(int world_x, int world_y, int& local_x, int& local_y) const;
    
    // 邻居管理
    void set_neighbor(int direction, Chunk* neighbor);
    Chunk* get_neighbor(int direction) const;
    
    // 坐标获取
    int get_chunk_x() const { return chunk_x_; }
    int get_chunk_y() const { return chunk_y_; }
    int get_world_x() const { return world_x_; }
    int get_world_y() const { return world_y_; }
    
    // 调试: 检查是否在当前帧被更新
    bool was_updated_this_frame() const { return updated_this_frame_; }
    void clear_updated_flag() { updated_this_frame_ = false; }
    void clear_synced_flag() { synced_this_frame_ = false; }
    
    // 清除所有像素的Updated标志
    void clear_pixel_updated_flags() {
        for (int y = -BORDER; y < SIZE + BORDER; ++y) {
            for (int x = -BORDER; x < SIZE + BORDER; ++x) {
                get(x, y).flags = static_cast<PixelFlags>(static_cast<uint8_t>(get(x, y).flags) & ~static_cast<uint8_t>(PixelFlags::Updated));
            }
        }
    }
    
private:
    // 更新单个像素(简单规则)
    void update_pixel(int local_x, int local_y, int pass);
    
    // 检查是否在当前pass的更新范围内
    bool is_in_pass(int local_x, int local_y, int pass) const;
    
    // 简单的元胞自动机规则
    void update_powder(int local_x, int local_y);
    void update_liquid(int local_x, int local_y);
    void update_gas(int local_x, int local_y);
    void update_fire(int local_x, int local_y);
    
    // 温度系统
    void update_temperature(int local_x, int local_y);
    
    // 尝试移动像素(支持边界扩展区)
    bool try_move(int from_x, int from_y, int to_x, int to_y);
    
    // 密度交换
    void try_density_swap(int x1, int y1, int x2, int y2);
};

/**
 * @brief 棋盘格更新管理器
 * 
 * 实现Noita的4-pass棋盘格并行更新模式
 */
class CheckerboardUpdater {
public:
    static constexpr int PASS_COUNT = 4;
    
private:
    int chunks_x_, chunks_y_;
    
public:
    CheckerboardUpdater(int chunks_x, int chunks_y);
    
    // 获取指定pass需要更新的块
    std::vector<int> get_chunks_for_pass(int pass) const;
    
    // 检查块是否在指定pass中更新
    bool should_update_chunk(int chunk_x, int chunk_y, int pass) const;
};

} // namespace noita
