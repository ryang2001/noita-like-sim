#include "core/Chunk.hpp"
#include "materials/MaterialRegistry.hpp"
#include <algorithm>
#include <random>

namespace noita {

// 随机数生成器
static std::mt19937 gen(std::random_device{}());

Chunk::Chunk(int cx, int cy)
    : chunk_x_(cx), chunk_y_(cy), world_x_(cx * SIZE), world_y_(cy * SIZE) {
    // 初始化所有像素为空
    for (auto& pixel : pixels_) {
        pixel = Pixel();
    }
}

Pixel& Chunk::get(int local_x, int local_y) {
    // 边界检查(允许访问边界扩展区)
    if (local_x < -BORDER || local_x >= SIZE + BORDER ||
        local_y < -BORDER || local_y >= SIZE + BORDER) {
        // 返回一个特殊的空像素引用
        static Pixel empty_pixel;
        return empty_pixel;
    }
    // 转换为数组索引
    int array_x = local_x + BORDER;
    int array_y = local_y + BORDER;
    return pixels_[array_y * (SIZE + BORDER * 2) + array_x];
}

const Pixel& Chunk::get(int local_x, int local_y) const {
    if (local_x < -BORDER || local_x >= SIZE + BORDER ||
        local_y < -BORDER || local_y >= SIZE + BORDER) {
        static const Pixel empty_pixel;
        return empty_pixel;
    }
    int array_x = local_x + BORDER;
    int array_y = local_y + BORDER;
    return pixels_[array_y * (SIZE + BORDER * 2) + array_x];
}

Pixel& Chunk::get_valid(int local_x, int local_y) {
    if (local_x < 0 || local_x >= SIZE || local_y < 0 || local_y >= SIZE) {
        static Pixel empty_pixel;
        return empty_pixel;
    }
    return get(local_x, local_y);
}

const Pixel& Chunk::get_valid(int local_x, int local_y) const {
    if (local_x < 0 || local_x >= SIZE || local_y < 0 || local_y >= SIZE) {
        static const Pixel empty_pixel;
        return empty_pixel;
    }
    return get(local_x, local_y);
}

Pixel& Chunk::get_world(int world_x, int world_y) {
    int local_x = world_x - world_x_;
    int local_y = world_y - world_y_;
    return get(local_x, local_y);
}

const Pixel& Chunk::get_world(int world_x, int world_y) const {
    int local_x = world_x - world_x_;
    int local_y = world_y - world_y_;
    return get(local_x, local_y);
}

void Chunk::set(int local_x, int local_y, const Pixel& pixel) {
    if (local_x >= -BORDER && local_x < SIZE + BORDER &&
        local_y >= -BORDER && local_y < SIZE + BORDER) {
        get(local_x, local_y) = pixel;
    }
}

void Chunk::set_world(int world_x, int world_y, const Pixel& pixel) {
    int local_x = world_x - world_x_;
    int local_y = world_y - world_y_;
    set(local_x, local_y, pixel);
}

void Chunk::local_to_world(int local_x, int local_y, int& world_x, int& world_y) const {
    world_x = world_x_ + local_x;
    world_y = world_y_ + local_y;
}

void Chunk::world_to_local(int world_x, int world_y, int& local_x, int& local_y) const {
    local_x = world_x - world_x_;
    local_y = world_y - world_y_;
}

void Chunk::set_neighbor(int direction, Chunk* neighbor) {
    if (direction >= 0 && direction < 4) {
        neighbors_[direction] = neighbor;
    }
}

Chunk* Chunk::get_neighbor(int direction) const {
    if (direction >= 0 && direction < 4) {
        return neighbors_[direction];
    }
    return nullptr;
}

void Chunk::update(uint32_t frame, int pass) {
    // 防止同一帧同一pass多次更新
    if (last_update_frame_.load() == frame && last_update_pass_.load() == pass) {
        return;
    }
    last_update_frame_ = frame;
    last_update_pass_ = pass;
    
    // 设置更新标志
    updated_this_frame_ = true;
    
    // 从下往上更新(Noita核心)
    // 使用局部随机数生成器,避免线程竞争
    std::mt19937 local_gen(std::random_device{}());
    std::uniform_int_distribution<> dis(0, 1);
    
    for (int y = SIZE + BORDER - 1; y >= -BORDER; --y) {
        // 随机化x方向的更新顺序
        bool left_to_right = dis(local_gen) == 0;
        
        if (left_to_right) {
            for (int x = -BORDER; x < SIZE + BORDER; ++x) {
                if (is_in_pass(x, y, pass)) {
                    update_pixel(x, y, pass);
                }
            }
        } else {
            for (int x = SIZE + BORDER - 1; x >= -BORDER; --x) {
                if (is_in_pass(x, y, pass)) {
                    update_pixel(x, y, pass);
                }
            }
        }
    }
}

bool Chunk::is_in_pass(int local_x, int local_y, int pass) const {
    // 只更新有效区域内的像素(0到63)
    // 边界扩展区的像素不更新,由sync_borders负责同步
    if (local_x >= 0 && local_x < SIZE && local_y >= 0 && local_y < SIZE) {
        return true;
    }
    
    // 边界扩展区的像素不更新
    return false;
}

void Chunk::update_pixel(int local_x, int local_y, int pass) {
    Pixel& pixel = get(local_x, local_y);
    
    // 跳过空像素
    if (pixel.is_empty()) {
        return;
    }
    
    // 防止同一帧多次更新
    if (has_flag(pixel.flags, PixelFlags::Updated)) {
        return;
    }
    
    // 标记为已更新
    pixel.flags |= PixelFlags::Updated;
    
    // 更新温度
    update_temperature(local_x, local_y);
    
    // 根据状态更新
    switch (pixel.state) {
        case MaterialState::Powder:
            update_powder(local_x, local_y);
            break;
        case MaterialState::Liquid:
            update_liquid(local_x, local_y);
            break;
        case MaterialState::Gas:
            update_gas(local_x, local_y);
            break;
        case MaterialState::Plasma:
            update_fire(local_x, local_y);
            break;
        default:
            break;
    }
}

void Chunk::update_temperature(int local_x, int local_y) {
    Pixel& pixel = get(local_x, local_y);
    if (pixel.is_empty()) return;
    
    const auto& props = MaterialRegistry::instance().get(pixel.material);
    
    // 检查温度变化
    // 气化
    if (pixel.temperature >= props.boiling_point) {
        if (pixel.state == MaterialState::Liquid) {
            pixel.state = MaterialState::Gas;
            // 可以在这里改变材料(如水->蒸汽)
        }
    }
    // 凝固
    else if (pixel.temperature <= props.freezing_point) {
        if (pixel.state == MaterialState::Liquid) {
            pixel.state = MaterialState::Solid;
        }
    }
    // 熔化
    else if (pixel.temperature >= props.melting_point) {
        if (pixel.state == MaterialState::Solid) {
            pixel.state = MaterialState::Liquid;
        }
    }
}

void Chunk::update_powder(int local_x, int local_y) {
    Pixel& powder = get(local_x, local_y);
    if (powder.is_empty()) return;
    
    // 粉末规则: 先下,再左下/右下
    if (try_move(local_x, local_y, local_x, local_y + 1)) {
        return;
    }
    
    // 随机选择左下或右下
    std::uniform_int_distribution<> dis(0, 1);
    int dir = dis(gen) * 2 - 1;
    
    if (try_move(local_x, local_y, local_x + dir, local_y + 1)) {
        return;
    }
    
    if (try_move(local_x, local_y, local_x - dir, local_y + 1)) {
        return;
    }
}

void Chunk::update_liquid(int local_x, int local_y) {
    Pixel& liquid = get(local_x, local_y);
    if (liquid.is_empty()) return;
    
    // 液体规则: 先下,再左右下角,再左右
    if (try_move(local_x, local_y, local_x, local_y + 1)) {
        return;
    }
    
    // 随机选择左下或右下
    std::uniform_int_distribution<> dis(0, 1);
    int dir = dis(gen) * 2 - 1;
    
    if (try_move(local_x, local_y, local_x + dir, local_y + 1)) {
        return;
    }
    
    if (try_move(local_x, local_y, local_x - dir, local_y + 1)) {
        return;
    }
    
    // 左右
    if (try_move(local_x, local_y, local_x + dir, local_y)) {
        return;
    }
    
    if (try_move(local_x, local_y, local_x - dir, local_y)) {
        return;
    }
}

void Chunk::update_gas(int local_x, int local_y) {
    Pixel& gas = get(local_x, local_y);
    if (gas.is_empty()) return;
    
    // 气体规则: 先上,再左右上角,再左右(与液体相反)
    if (try_move(local_x, local_y, local_x, local_y - 1)) {
        return;
    }
    
    // 随机选择左上或右上
    std::uniform_int_distribution<> dis(0, 1);
    int dir = dis(gen) * 2 - 1;
    
    if (try_move(local_x, local_y, local_x + dir, local_y - 1)) {
        return;
    }
    
    if (try_move(local_x, local_y, local_x - dir, local_y - 1)) {
        return;
    }
    
    // 左右
    if (try_move(local_x, local_y, local_x + dir, local_y)) {
        return;
    }
    
    if (try_move(local_x, local_y, local_x - dir, local_y)) {
        return;
    }
}

void Chunk::update_fire(int local_x, int local_y) {
    Pixel& fire = get(local_x, local_y);
    if (fire.is_empty()) return;
    
    // 火焰生命周期
    if (fire.lifetime > 0) {
        fire.lifetime--;
        if (fire.lifetime <= 0) {
            get(local_x, local_y) = Pixel();
            return;
        }
    }
    
    // 检查周围8个方向
    int dirs[8][2] = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0},          {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
    };
    
    std::uniform_int_distribution<> prob_dis(0, 99);
    
    for (int i = 0; i < 8; ++i) {
        int nx = local_x + dirs[i][0];
        int ny = local_y + dirs[i][1];
        
        Pixel& neighbor = get(nx, ny);
        
        if (neighbor.is_empty()) {
            // 火焰向空处扩散(低概率)
            if (prob_dis(gen) < 3) {
                Pixel new_fire = fire;
                new_fire.lifetime = 20 + (prob_dis(gen) % 20);
                get(nx, ny) = new_fire;
            }
        } else {
            const auto& neighbor_props = MaterialRegistry::instance().get(neighbor.material);
            
            if (neighbor_props.flammability > 0.0f) {
                // 可燃材料: 点燃
                if (prob_dis(gen) < static_cast<int>(neighbor_props.flammability * 40)) {
                    Pixel new_fire;
                    new_fire.material = MaterialRegistry::instance().get_id("fire");
                    new_fire.state = MaterialState::Plasma;
                    new_fire.lifetime = 30 + (prob_dis(gen) % 30);
                    get(nx, ny) = new_fire;
                    fire.lifetime += 5;
                    
                    // 增加温度
                    neighbor.temperature += 50.0f;
                }
            } else if (neighbor.material == MaterialRegistry::instance().get_id("water")) {
                // 水扑灭火焰
                get(local_x, local_y) = Pixel();
                
                Pixel steam;
                steam.material = MaterialRegistry::instance().get_id("steam");
                steam.state = MaterialState::Gas;
                steam.lifetime = 100;
                get(nx, ny) = steam;
                return;
            }
        }
    }
    
    // 火焰向上飘动
    if (prob_dis(gen) < 30) {
        if (try_move(local_x, local_y, local_x, local_y - 1)) {
            return;
        }
    }
}

bool Chunk::try_move(int from_x, int from_y, int to_x, int to_y) {
    Pixel& from = get(from_x, from_y);
    if (from.is_empty()) return false;
    
    // 检查目标位置是否在边界扩展区内
    if (to_x < -BORDER || to_x >= SIZE + BORDER ||
        to_y < -BORDER || to_y >= SIZE + BORDER) {
        return false;
    }
    
    Pixel& to = get(to_x, to_y);
    
    // 检查目标位置
    if (to.is_empty()) {
        // 直接移动
        to = from;
        from = Pixel();
        return true;
    }
    
    // 固体材料不能被穿透
    if (to.is_solid()) {
        return false;
    }
    
    // 同种液体/气体不交换(避免无限交换)
    // 只有密度不同或状态不同才交换
    if (from.material == to.material) {
        return false;
    }
    
    // 检查密度交换
    const auto& from_props = MaterialRegistry::instance().get(from.material);
    const auto& to_props = MaterialRegistry::instance().get(to.material);
    
    if (from_props.density > to_props.density && to.is_liquid()) {
        // 密度大,交换
        std::swap(from, to);
        return true;
    }
    
    // 气体可以被液体/粉末置换
    if (to.is_gas() && (from.is_liquid() || from.is_powder())) {
        std::swap(from, to);
        return true;
    }
    
    return false;
}

void Chunk::try_density_swap(int x1, int y1, int x2, int y2) {
    Pixel& p1 = get(x1, y1);
    Pixel& p2 = get(x2, y2);
    
    if (p1.is_empty() || p2.is_empty()) return;
    
    const auto& props1 = MaterialRegistry::instance().get(p1.material);
    const auto& props2 = MaterialRegistry::instance().get(p2.material);
    
    // 密度大的下沉
    if (props1.density > props2.density) {
        std::swap(p1, p2);
    }
}

void Chunk::sync_borders() {
    // 避免重复同步
    if (synced_this_frame_.exchange(true)) {
        return;
    }
    
    // 同步边界像素到邻居块
    // 北邻居 (neighbors_[0])
    if (neighbors_[0]) {
        // 同步整个北边界扩展区 (y = -BORDER 到 -1)
        for (int y = -BORDER; y < 0; ++y) {
            for (int x = 0; x < SIZE; ++x) {
                Pixel& my_border = get(x, y);
                
                // 计算在邻居中的对应位置
                int neighbor_y = SIZE + y;  // y=-1 -> SIZE-1, y=-BORDER -> SIZE-BORDER
                if (neighbor_y >= 0 && neighbor_y < SIZE) {
                    Pixel& neighbor_valid = neighbors_[0]->get(x, neighbor_y);
                    
                    // 双向同步
                    if (!my_border.is_empty() && neighbor_valid.is_empty()) {
                        // 边界扩展区的像素移动到邻居
                        neighbor_valid = my_border;
                        my_border = Pixel();
                    } else if (my_border.is_empty() && !neighbor_valid.is_empty()) {
                        // 邻居的像素移动到边界扩展区
                        my_border = neighbor_valid;
                        neighbor_valid = Pixel();
                    }
                }
            }
        }
    }
    
    // 东邻居 (neighbors_[1])
    if (neighbors_[1]) {
        // 同步整个东边界扩展区 (x = SIZE 到 SIZE+BORDER-1)
        for (int x = SIZE; x < SIZE + BORDER; ++x) {
            for (int y = 0; y < SIZE; ++y) {
                Pixel& my_border = get(x, y);
                
                // 计算在邻居中的对应位置
                int neighbor_x = x - SIZE;  // x=SIZE -> 0, x=SIZE+BORDER-1 -> BORDER-1
                if (neighbor_x >= 0 && neighbor_x < SIZE) {
                    Pixel& neighbor_valid = neighbors_[1]->get(neighbor_x, y);
                    
                    // 双向同步
                    if (!my_border.is_empty() && neighbor_valid.is_empty()) {
                        // 边界扩展区的像素移动到邻居
                        neighbor_valid = my_border;
                        my_border = Pixel();
                    } else if (my_border.is_empty() && !neighbor_valid.is_empty()) {
                        // 邻居的像素移动到边界扩展区
                        my_border = neighbor_valid;
                        neighbor_valid = Pixel();
                    }
                }
            }
        }
    }
    
    // 南邻居 (neighbors_[2])
    if (neighbors_[2]) {
        // 同步整个南边界扩展区 (y = SIZE 到 SIZE+BORDER-1)
        for (int y = SIZE; y < SIZE + BORDER; ++y) {
            for (int x = 0; x < SIZE; ++x) {
                Pixel& my_border = get(x, y);
                
                // 计算在邻居中的对应位置
                int neighbor_y = y - SIZE;  // y=SIZE -> 0, y=SIZE+BORDER-1 -> BORDER-1
                if (neighbor_y >= 0 && neighbor_y < SIZE) {
                    Pixel& neighbor_valid = neighbors_[2]->get(x, neighbor_y);
                    
                    // 双向同步
                    if (!my_border.is_empty() && neighbor_valid.is_empty()) {
                        // 边界扩展区的像素移动到邻居
                        neighbor_valid = my_border;
                        my_border = Pixel();
                    } else if (my_border.is_empty() && !neighbor_valid.is_empty()) {
                        // 邻居的像素移动到边界扩展区
                        my_border = neighbor_valid;
                        neighbor_valid = Pixel();
                    }
                }
            }
        }
    }
    
    // 西邻居 (neighbors_[3])
    if (neighbors_[3]) {
        // 同步整个西边界扩展区 (x = -BORDER 到 -1)
        for (int x = -BORDER; x < 0; ++x) {
            for (int y = 0; y < SIZE; ++y) {
                Pixel& my_border = get(x, y);
                
                // 计算在邻居中的对应位置
                int neighbor_x = SIZE + x;  // x=-1 -> SIZE-1, x=-BORDER -> SIZE-BORDER
                if (neighbor_x >= 0 && neighbor_x < SIZE) {
                    Pixel& neighbor_valid = neighbors_[3]->get(neighbor_x, y);
                    
                    // 双向同步
                    if (!my_border.is_empty() && neighbor_valid.is_empty()) {
                        // 边界扩展区的像素移动到邻居
                        neighbor_valid = my_border;
                        my_border = Pixel();
                    } else if (my_border.is_empty() && !neighbor_valid.is_empty()) {
                        // 邻居的像素移动到边界扩展区
                        my_border = neighbor_valid;
                        neighbor_valid = Pixel();
                    }
                }
            }
        }
    }
}

// CheckerboardUpdater实现

CheckerboardUpdater::CheckerboardUpdater(int chunks_x, int chunks_y)
    : chunks_x_(chunks_x), chunks_y_(chunks_y) {
}

std::vector<int> CheckerboardUpdater::get_chunks_for_pass(int pass) const {
    std::vector<int> result;
    
    // 关键修复: 从下往上更新chunk!
    // 这样液体才能正确跨块流动
    for (int cy = chunks_y_ - 1; cy >= 0; --cy) {
        for (int cx = 0; cx < chunks_x_; ++cx) {
            if (should_update_chunk(cx, cy, pass)) {
                result.push_back(cy * chunks_x_ + cx);
            }
        }
    }
    
    return result;
}

bool CheckerboardUpdater::should_update_chunk(int chunk_x, int chunk_y, int pass) const {
    // 4-pass模式:
    // Pass 0: y偶数, x偶数
    // Pass 1: y偶数, x奇数
    // Pass 2: y奇数, x偶数
    // Pass 3: y奇数, x奇数
    return ((chunk_y % 2) * 2 + (chunk_x % 2)) == pass;
}

} // namespace noita
