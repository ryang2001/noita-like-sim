#include "core/World.hpp"
#include "materials/MaterialRegistry.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
#include <algorithm>

namespace noita {

// 静态成员初始化
World* World::instance_ = nullptr;

World::World(const WorldConfig& config)
    : config_(config) {
    
    // 设置全局实例
    instance_ = this;
    
    // 初始化线程池
    int thread_count = config.thread_count;
    if (thread_count <= 0) {
        thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 4;
    }
    thread_pool_ = std::make_unique<WorkStealingPool>(thread_count);
    
    // 初始化分块
    initialize_chunks();
    
    // 初始化棋盘格更新器
    checkerboard_updater_ = std::make_unique<CheckerboardUpdater>(chunks_x_, chunks_y_);
    
    std::cout << "World initialized: " << config_.width << "x" << config_.height
              << " (" << chunks_x_ << "x" << chunks_y_ << " chunks)" << std::endl;
}

World::~World() {
    instance_ = nullptr;
    chunks_.clear();
}

void World::initialize_chunks() {
    // 计算块数量
    chunks_x_ = (config_.width + Chunk::SIZE - 1) / Chunk::SIZE;
    chunks_y_ = (config_.height + Chunk::SIZE - 1) / Chunk::SIZE;
    
    // 创建块
    chunks_.reserve(chunks_x_ * chunks_y_);
    for (int cy = 0; cy < chunks_y_; ++cy) {
        for (int cx = 0; cx < chunks_x_; ++cx) {
            chunks_.push_back(std::make_unique<Chunk>(cx, cy));
        }
    }
    
    // 连接邻居
    for (int cy = 0; cy < chunks_y_; ++cy) {
        for (int cx = 0; cx < chunks_x_; ++cx) {
            Chunk* chunk = get_chunk_by_index(cx, cy);
            if (!chunk) continue;
            
            // 北邻居
            if (cy > 0) {
                chunk->set_neighbor(0, get_chunk_by_index(cx, cy - 1));
            }
            
            // 东邻居
            if (cx < chunks_x_ - 1) {
                chunk->set_neighbor(1, get_chunk_by_index(cx + 1, cy));
            }
            
            // 南邻居
            if (cy < chunks_y_ - 1) {
                chunk->set_neighbor(2, get_chunk_by_index(cx, cy + 1));
            }
            
            // 西邻居
            if (cx > 0) {
                chunk->set_neighbor(3, get_chunk_by_index(cx - 1, cy));
            }
        }
    }
}

void World::update(float dt) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 重置统计
    stats_.pixels_updated = 0;
    stats_.chunks_updated = 0;
    
    // 清除所有chunk的同步标志和像素的Updated标志
    // 注意: 不清除updated_this_frame_标志,因为debug模式需要使用
    for (auto& chunk : chunks_) {
        chunk->clear_synced_flag();
        chunk->clear_pixel_updated_flags();
    }
    
    // 增加帧计数器
    uint32_t frame = frame_counter_.fetch_add(1);
    
    // 棋盘格4-pass更新
    for (int pass = 0; pass < 4; ++pass) {
        // 获取当前pass需要更新的块
        auto chunk_indices = checkerboard_updater_->get_chunks_for_pass(pass);
        
        if (config_.use_parallel && thread_pool_) {
            // 并行更新
            std::vector<std::future<void>> futures;
            for (int idx : chunk_indices) {
                if (idx >= 0 && idx < static_cast<int>(chunks_.size())) {
                    int cx = idx % chunks_x_;
                    int cy = idx / chunks_x_;
                    futures.push_back(thread_pool_->enqueue([this, idx, frame, pass, cx, cy]() {
                        active_chunk_x_.store(cx);
                        active_chunk_y_.store(cy);
                        chunks_[idx]->update(frame, pass);
                        active_chunk_x_.store(-1);
                        active_chunk_y_.store(-1);
                    }));
                }
            }
            
            // 等待所有任务完成
            for (auto& future : futures) {
                future.get();
            }
        } else {
            // 串行更新
            for (int idx : chunk_indices) {
                if (idx >= 0 && idx < static_cast<int>(chunks_.size())) {
                    chunks_[idx]->update(frame, pass);
                }
            }
        }
        
        // 关键修复: 每个pass后立即同步边界!
        // 这样液体可以跨块流动
        for (int idx : chunk_indices) {
            if (idx >= 0 && idx < static_cast<int>(chunks_.size())) {
                chunks_[idx]->sync_borders();
            }
        }
        
        // 调用pass完成回调(渲染)
        // 如果设置了回调,每个pass后都调用(用于debug模式)
        if (pass_callback_) {
            pass_callback_(pass);
        }
    }
    
    // 计算更新时间
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    stats_.update_time_ms = duration.count() / 1000.0f;
}

Pixel& World::get(int x, int y) {
    Chunk* chunk = get_chunk(x, y);
    if (!chunk) {
        static Pixel empty_pixel;
        return empty_pixel;
    }
    return chunk->get_world(x, y);
}

const Pixel& World::get(int x, int y) const {
    const Chunk* chunk = get_chunk(x, y);
    if (!chunk) {
        static const Pixel empty_pixel;
        return empty_pixel;
    }
    return chunk->get_world(x, y);
}

void World::set(int x, int y, const Pixel& pixel) {
    Chunk* chunk = get_chunk(x, y);
    if (chunk) {
        chunk->set_world(x, y, pixel);
    }
}

void World::set_material(int x, int y, MaterialID material) {
    Pixel pixel;
    pixel.material = material;
    
    const auto& props = MaterialRegistry::instance().get(material);
    pixel.state = props.default_state;
    
    set(x, y, pixel);
}

Chunk* World::get_chunk(int x, int y) {
    int cx = x / Chunk::SIZE;
    int cy = y / Chunk::SIZE;
    return get_chunk_by_index(cx, cy);
}

const Chunk* World::get_chunk(int x, int y) const {
    int cx = x / Chunk::SIZE;
    int cy = y / Chunk::SIZE;
    return get_chunk_by_index(cx, cy);
}

Chunk* World::get_chunk_by_index(int cx, int cy) {
    if (cx < 0 || cx >= chunks_x_ || cy < 0 || cy >= chunks_y_) {
        return nullptr;
    }
    return chunks_[cy * chunks_x_ + cx].get();
}

const Chunk* World::get_chunk_by_index(int cx, int cy) const {
    if (cx < 0 || cx >= chunks_x_ || cy < 0 || cy >= chunks_y_) {
        return nullptr;
    }
    return chunks_[cy * chunks_x_ + cx].get();
}

void World::clear() {
    for (auto& chunk : chunks_) {
        for (int y = 0; y < Chunk::SIZE; ++y) {
            for (int x = 0; x < Chunk::SIZE; ++x) {
                chunk->set(x, y, Pixel());
            }
        }
    }
}

void World::fill(MaterialID material) {
    for (int y = 0; y < config_.height; ++y) {
        for (int x = 0; x < config_.width; ++x) {
            set_material(x, y, material);
        }
    }
}

void World::fill_rect(int x, int y, int w, int h, MaterialID material) {
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            set_material(x + dx, y + dy, material);
        }
    }
}

void World::fill_circle(int cx, int cy, int radius, MaterialID material) {
    for (int y = cy - radius; y <= cy + radius; ++y) {
        for (int x = cx - radius; x <= cx + radius; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx * dx + dy * dy <= radius * radius) {
                set_material(x, y, material);
            }
        }
    }
}

void World::generate_caves() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (int y = 0; y < config_.height; ++y) {
        for (int x = 0; x < config_.width; ++x) {
            if (dis(gen) < 0.3) {
                set_material(x, y, MaterialRegistry::instance().get_id("air"));
            } else {
                set_material(x, y, MaterialRegistry::instance().get_id("stone"));
            }
        }
    }
}

void World::generate_surface() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(-5, 5);
    
    int surface_y = config_.height / 4;
    
    for (int x = 0; x < config_.width; ++x) {
        surface_y += dis(gen);
        surface_y = std::max(10, std::min(config_.height - 10, surface_y));
        
        for (int y = 0; y < surface_y; ++y) {
            set_material(x, y, MaterialRegistry::instance().get_id("air"));
        }
        for (int y = surface_y; y < config_.height; ++y) {
            if (y < surface_y + 5) {
                set_material(x, y, MaterialRegistry::instance().get_id("dirt"));
            } else {
                set_material(x, y, MaterialRegistry::instance().get_id("stone"));
            }
        }
    }
}

void World::generate_biome(int biome_type) {
    generate_surface();
}

void World::set_active_region(int center_x, int center_y, int radius) {
    active_region_.center_x = center_x;
    active_region_.center_y = center_y;
    active_region_.radius = radius;
}

void World::update_active_region(int player_x, int player_y) {
    active_region_.update(player_x, player_y);
}

void World::for_each_pixel(PixelCallback callback) {
    for (int y = 0; y < config_.height; ++y) {
        for (int x = 0; x < config_.width; ++x) {
            callback(x, y, get(x, y));
        }
    }
}

void World::for_each_chunk(std::function<void(Chunk&)> callback) {
    for (auto& chunk : chunks_) {
        callback(*chunk);
    }
}

void World::world_to_chunk(int world_x, int world_y, int& chunk_x, int& chunk_y) const {
    chunk_x = world_x / Chunk::SIZE;
    chunk_y = world_y / Chunk::SIZE;
}

void World::chunk_to_world(int chunk_x, int chunk_y, int& world_x, int& world_y) const {
    world_x = chunk_x * Chunk::SIZE;
    world_y = chunk_y * Chunk::SIZE;
}

bool World::is_valid(int x, int y) const {
    return x >= 0 && x < config_.width && y >= 0 && y < config_.height;
}

bool World::is_in_bounds(int x, int y) const {
    return is_valid(x, y);
}

void World::reset_stats() {
    stats_.pixels_updated = 0;
    stats_.chunks_updated = 0;
    stats_.update_time_ms = 0.0f;
}

void World::save(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to save world to: " << path << std::endl;
        return;
    }
    
    // 写入世界尺寸
    file.write(reinterpret_cast<const char*>(&config_.width), sizeof(int));
    file.write(reinterpret_cast<const char*>(&config_.height), sizeof(int));
    
    // 写入所有像素
    for (int y = 0; y < config_.height; ++y) {
        for (int x = 0; x < config_.width; ++x) {
            const Pixel& pixel = get(x, y);
            file.write(reinterpret_cast<const char*>(&pixel), sizeof(Pixel));
        }
    }
    
    std::cout << "World saved to: " << path << std::endl;
}

void World::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to load world from: " << path << std::endl;
        return;
    }
    
    // 读取世界尺寸
    int width, height;
    file.read(reinterpret_cast<char*>(&width), sizeof(int));
    file.read(reinterpret_cast<char*>(&height), sizeof(int));
    
    if (width != config_.width || height != config_.height) {
        std::cerr << "World size mismatch!" << std::endl;
        return;
    }
    
    // 读取所有像素
    for (int y = 0; y < config_.height; ++y) {
        for (int x = 0; x < config_.width; ++x) {
            Pixel pixel;
            file.read(reinterpret_cast<char*>(&pixel), sizeof(Pixel));
            set(x, y, pixel);
        }
    }
    
    std::cout << "World loaded from: " << path << std::endl;
}

float World::noise(int x, int y, float scale) {
    return 0.5f;
}

void World::apply_material_with_variation(int x, int y, MaterialID material) {
    set_material(x, y, material);
}

} // namespace noita
