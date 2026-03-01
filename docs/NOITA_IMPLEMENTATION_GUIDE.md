# Noita核心技术实现指南

## 🎯 核心技术要点

基于Noita开发者的技术分享,本文档详细说明关键技术的实现方法。

---

## 1. 从下往上的更新顺序

### 1.1 为什么必须从下往上?

**问题**: 如果从上往下更新
```
Frame 1: 只有最底部的像素下落
Frame 2: 倒数第二行像素下落
Frame 3: 倒数第三行像素下落
... (下落序列不一致,看起来像"波浪")
```

**解决方案**: 从下往上更新
```
Frame 1: 所有像素同时下落
Frame 2: 继续下落
... (下落序列一致,看起来自然)
```

### 1.2 实现代码

```cpp
void World::update() {
    // 关键: 从下往上遍历
    for (int y = HEIGHT - 1; y >= 0; --y) {  // 注意: y--
        for (int x = 0; x < WIDTH; ++x) {
            update_pixel(x, y);
        }
    }
}

void Chunk::update(uint32_t frame, int pass) {
    if (!is_dirty()) return;
    
    // 从下往上更新脏矩形内的像素
    for (int y = dirty_rect_.max_y; y >= dirty_rect_.min_y; --y) {
        for (int x = dirty_rect_.min_x; x <= dirty_rect_.max_x; ++x) {
            if (is_in_update_territory(x, y, pass)) {
                update_pixel_from_bottom(x, y, frame);
            }
        }
    }
}
```

---

## 2. 64×64分块系统

### 2.1 为什么是64×64?

**Noita的选择**: 64×64像素块

**原因**:
1. 足够小,可以高效并行
2. 足够大,减少同步开销
3. 脏矩形优化效果好
4. 适合现代CPU缓存大小

### 2.2 分块实现

```cpp
class World {
    static constexpr int CHUNK_SIZE = 64;
    
    std::vector<std::unique_ptr<Chunk>> chunks_;
    int chunks_x_, chunks_y_;
    
    Chunk* get_chunk(int world_x, int world_y) {
        int cx = world_x / CHUNK_SIZE;
        int cy = world_y / CHUNK_SIZE;
        if (cx < 0 || cx >= chunks_x_ || cy < 0 || cy >= chunks_y_) {
            return nullptr;
        }
        return chunks_[cy * chunks_x_ + cx].get();
    }
    
    Pixel& get_pixel(int world_x, int world_y) {
        Chunk* chunk = get_chunk(world_x, world_y);
        if (!chunk) return empty_pixel_;
        return chunk->get_world(world_x, world_y);
    }
};
```

---

## 3. 脏矩形优化

### 3.1 核心思想

**问题**: 每帧更新所有262,144个像素太慢

**解决方案**: 只更新"脏"的区域

**Noita实现**: 每个块维护自己的脏矩形

### 3.2 脏矩形实现

```cpp
class Chunk {
    struct DirtyRect {
        int min_x, min_y;  // 脏区域左上角
        int max_x, max_y;  // 脏区域右下角
        bool is_dirty = false;
        
        void reset() {
            min_x = CHUNK_SIZE;
            min_y = CHUNK_SIZE;
            max_x = 0;
            max_y = 0;
            is_dirty = false;
        }
        
        void expand(int x, int y) {
            min_x = std::min(min_x, x);
            min_y = std::min(min_y, y);
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
            is_dirty = true;
        }
    } dirty_rect_;
    
    void mark_dirty(int local_x, int local_y) {
        dirty_rect_.expand(local_x, local_y);
    }
    
    void update(uint32_t frame) {
        if (!dirty_rect_.is_dirty) return;  // 跳过干净块
        
        // 只更新脏矩形内的像素
        for (int y = dirty_rect_.max_y; y >= dirty_rect_.min_y; --y) {
            for (int x = dirty_rect_.min_x; x <= dirty_rect_.max_x; ++x) {
                update_pixel(x, y, frame);
            }
        }
        
        dirty_rect_.reset();  // 清空脏矩形
    }
};
```

**性能提升**: 
- 静态区域不消耗CPU
- 只更新活跃区域
- 典型场景下减少90%+的更新量

---

## 4. 4-Pass棋盘格并行更新

### 4.1 问题: 多线程与更新顺序的冲突

**矛盾**:
- Falling sand需要从下往上更新(串行依赖)
- 多线程需要并行执行(无依赖)

**Naive方案的问题**:
```cpp
// 错误方式: 简单并行
#pragma omp parallel for
for (int chunk_id = 0; chunk_id < chunks.size(); ++chunk_id) {
    chunks[chunk_id]->update();
}

// 问题:
// 1. 像素可能离开其64×64区域进入另一个块
// 2. 同一个像素可能被多个线程更新
// 3. 需要原子操作或锁来防止重复更新
// 4. 锁会严重拖慢速度
```

### 4.2 Noita的解决方案: 棋盘格模式

**核心思想**: 
- 将世界分成4组块
- 每组块之间不相邻
- 每个pass只更新一组块
- 每个块有32像素的"安全边界"

**示意图**:
```
Pass 0:  0 2 0 2 0 2
         2 0 2 0 2 0
         0 2 0 2 0 2
         2 0 2 0 2 0

Pass 1:  1 3 1 3 1 3
         3 1 3 1 3 1
         1 3 1 3 1 3
         3 1 3 1 3 1

Pass 2:  2 0 2 0 2 0
         0 2 0 2 0 2
         2 0 2 0 2 0
         0 2 0 2 0 2

Pass 3:  3 1 3 1 3 1
         1 3 1 3 1 3
         3 1 3 1 3 1
         1 3 1 3 1 3
```

**关键**: 
- 相邻的块永远不会在同一个pass中更新
- 每个块有32像素的"独占区域"
- 无需锁或原子操作

### 4.3 实现代码

```cpp
class World {
    void update_parallel(uint32_t frame) {
        // 4-pass棋盘格更新
        for (int pass = 0; pass < 4; ++pass) {
            // 并行处理当前pass的块
            std::vector<std::future<void>> futures;
            
            for (int cy = 0; cy < chunks_y_; ++cy) {
                for (int cx = 0; cx < chunks_x_; ++cx) {
                    // 棋盘格选择
                    if ((cx + cy) % 4 == pass) {
                        Chunk* chunk = get_chunk(cx, cy);
                        if (chunk && chunk->is_dirty()) {
                            futures.push_back(
                                thread_pool_->enqueue([chunk, frame, pass]() {
                                    chunk->update(frame, pass);
                                })
                            );
                        }
                    }
                }
            }
            
            // 等待当前pass完成
            for (auto& future : futures) {
                future.wait();
            }
        }
    }
};

class Chunk {
    void update(uint32_t frame, int pass) {
        if (!is_dirty()) return;
        
        // 从下往上更新
        for (int y = dirty_rect_.max_y; y >= dirty_rect_.min_y; --y) {
            for (int x = dirty_rect_.min_x; x <= dirty_rect_.max_x; ++x) {
                // 检查是否在当前pass的更新范围内
                if (is_in_update_territory(x, y, pass)) {
                    update_pixel_from_bottom(x, y, frame);
                }
            }
        }
    }
    
    bool is_in_update_territory(int local_x, int local_y, int pass) const {
        // 计算该像素属于哪个pass
        int world_x = world_x_ + local_x;
        int world_y = world_y_ + local_y;
        
        int chunk_x = world_x / SIZE;
        int chunk_y = world_y / SIZE;
        
        // 检查是否在当前块内
        if (chunk_x == chunk_x_ && chunk_y == chunk_y_) {
            return true;
        }
        
        // 检查是否在32像素边界内
        int local_in_neighbor_x = world_x - (chunk_x * SIZE);
        int local_in_neighbor_y = world_y - (chunk_y * SIZE);
        
        // 边界扩展: 32像素
        if (local_in_neighbor_x >= 0 && local_in_neighbor_x < SIZE + BORDER * 2 &&
            local_in_neighbor_y >= 0 && local_in_neighbor_y < SIZE + BORDER * 2) {
            // 检查邻居是否在当前pass更新
            return (chunk_x + chunk_y) % 4 == pass;
        }
        
        return false;
    }
};
```

### 4.4 边界扩展

**问题**: 像素可能移动到相邻块

**解决方案**: 每个块存储额外的32像素边界

```
块大小: 64×64
实际存储: (64 + 32*2) × (64 + 32*2) = 128×128

布局:
+------------------+
|    32px border   |
|  +------------+  |
|  |            |  |
|32|   64×64    |32|
|px|   core     |px|
|  |            |  |
|  +------------+  |
|    32px border   |
+------------------+
```

**实现**:
```cpp
class Chunk {
    // 存储包含边界的像素
    std::array<Pixel, (SIZE + BORDER * 2) * (SIZE + BORDER * 2)> pixels_;
    
    Pixel& get(int local_x, int local_y) {
        // 转换为带边界的坐标
        int storage_x = local_x + BORDER;
        int storage_y = local_y + BORDER;
        
        return pixels_[storage_y * (SIZE + BORDER * 2) + storage_x];
    }
    
    void sync_borders() {
        // 从邻居复制边界像素
        if (neighbors_[NORTH]) {
            for (int x = 0; x < SIZE; ++x) {
                // 复制邻居的底部边界到自己的顶部边界
                pixels_[x + BORDER] = neighbors_[NORTH]->get(x, SIZE - 1);
            }
        }
        // ... 其他方向类似
    }
};
```

---

## 5. 密度置换系统

### 5.1 核心思想

**Noita实现**: 不同密度的液体会自然分层

**示例**:
- 油的密度 < 水的密度
- 油会浮在水面上
- 沙子的密度 > 水的密度
- 沙子会沉入水中

### 5.2 实现代码

```cpp
void Chunk::update_liquid(int local_x, int local_y) {
    Pixel& current = get(local_x, local_y);
    
    // 1. 尝试向下移动
    if (local_y + 1 < SIZE + BORDER * 2) {
        Pixel& below = get(local_x, local_y + 1);
        
        if (below.is_empty()) {
            // 直接下落
            swap(current, below);
            mark_dirty(local_x, local_y + 1);
            return;
        }
        
        if (below.is_liquid()) {
            // 密度置换(Noita核心!)
            float current_density = MaterialRegistry::get(current.material).density;
            float below_density = MaterialRegistry::get(below.material).density;
            
            if (current_density > below_density) {
                // 密度大的下沉
                swap(current, below);
                mark_dirty(local_x, local_y + 1);
                return;
            }
        }
    }
    
    // 2. 尝试对角线移动
    int diag_dirs[2] = {-1, 1};
    int diag_dir = diag_dirs[rand() % 2];
    
    if (local_x + diag_dir >= 0 && local_x + diag_dir < SIZE + BORDER * 2 &&
        local_y + 1 < SIZE + BORDER * 2) {
        Pixel& diag = get(local_x + diag_dir, local_y + 1);
        
        if (diag.is_empty()) {
            swap(current, diag);
            mark_dirty(local_x + diag_dir, local_y + 1);
            return;
        }
    }
    
    // 3. 尝试横向移动
    int side_dirs[2] = {-1, 1};
    int side_dir = side_dirs[rand() % 2];
    
    if (local_x + side_dir >= 0 && local_x + side_dir < SIZE + BORDER * 2) {
        Pixel& side = get(local_x + side_dir, local_y);
        
        if (side.is_empty()) {
            swap(current, side);
            mark_dirty(local_x + side_dir, local_y);
            return;
        }
    }
}

void Chunk::try_density_swap(int x1, int y1, int x2, int y2) {
    Pixel& p1 = get(x1, y1);
    Pixel& p2 = get(x2, y2);
    
    if (p1.is_empty() || p2.is_empty()) return;
    
    float density1 = MaterialRegistry::get(p1.material).density;
    float density2 = MaterialRegistry::get(p2.material).density;
    
    // 密度大的下沉
    if (density1 > density2 && y1 < y2) {
        swap(p1, p2);
        mark_dirty(x1, y1);
        mark_dirty(x2, y2);
    }
}
```

---

## 6. 火焰传播系统

### 6.1 Noita的火焰规则

```
火焰行为:
1. 随机选择一个方向
2. 检查该方向的像素是否可燃
3. 如果可燃(如油),点燃它
4. 如果不可燃(如水),火焰转化为蒸汽
5. 木材燃烧: 等待足够时间后销毁像素
```

### 6.2 实现代码

```cpp
void Chunk::update_fire(int local_x, int local_y) {
    Pixel& fire = get(local_x, local_y);
    
    // 火焰生命周期
    fire.lifetime--;
    if (fire.lifetime <= 0) {
        fire.clear();
        mark_dirty(local_x, local_y);
        return;
    }
    
    // 随机选择方向(8方向)
    int dirs[8][2] = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0},          {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
    };
    
    int dir = rand() % 8;
    int nx = local_x + dirs[dir][0];
    int ny = local_y + dirs[dir][1];
    
    if (nx < 0 || nx >= SIZE + BORDER * 2 || 
        ny < 0 || ny >= SIZE + BORDER * 2) {
        return;
    }
    
    Pixel& neighbor = get(nx, ny);
    
    if (neighbor.is_empty()) {
        // 火焰向空处扩散(概率)
        if (rand() % 100 < 20) {
            neighbor = Pixel(MaterialRegistry::get_id("fire"), MaterialState::Plasma);
            neighbor.lifetime = 50 + rand() % 50;
            mark_dirty(nx, ny);
        }
    } else {
        const auto& neighbor_props = MaterialRegistry::get(neighbor.material);
        
        if (neighbor_props.flammability > 0.0f) {
            // 可燃材料: 点燃
            if (rand() % 100 < static_cast<int>(neighbor_props.flammability * 100)) {
                neighbor.material = MaterialRegistry::get_id("fire");
                neighbor.state = MaterialState::Plasma;
                neighbor.lifetime = 50 + rand() % 50;
                mark_dirty(nx, ny);
            }
        } else if (neighbor.material == MaterialRegistry::get_id("water")) {
            // 水扑灭火焰,转化为蒸汽
            fire.material = MaterialRegistry::get_id("steam");
            fire.state = MaterialState::Gas;
            fire.lifetime = 100;
            
            neighbor.material = MaterialRegistry::get_id("steam");
            neighbor.state = MaterialState::Gas;
            neighbor.lifetime = 100;
            
            mark_dirty(local_x, local_y);
            mark_dirty(nx, ny);
        }
    }
    
    // 火焰向上飘动
    if (local_y > 0) {
        Pixel& above = get(local_x, local_y - 1);
        if (above.is_empty() && rand() % 100 < 30) {
            swap(fire, above);
            mark_dirty(local_x, local_y - 1);
        }
    }
}
```

---

## 7. 气体物理系统

### 7.1 Noita的气体规则

```
气体行为(反向液体):
1. 优先检查是否可以向上移动
2. 如果不能,检查左右是否可以移动
```

### 7.2 实现代码

```cpp
void Chunk::update_gas(int local_x, int local_y) {
    Pixel& gas = get(local_x, local_y);
    
    // 气体生命周期
    gas.lifetime--;
    if (gas.lifetime <= 0) {
        gas.clear();
        mark_dirty(local_x, local_y);
        return;
    }
    
    // 1. 优先向上移动
    if (local_y > 0) {
        Pixel& above = get(local_x, local_y - 1);
        
        if (above.is_empty()) {
            swap(gas, above);
            mark_dirty(local_x, local_y - 1);
            return;
        }
        
        if (above.is_liquid() || above.is_powder()) {
            // 气体穿过液体/粉末上升
            float gas_density = MaterialRegistry::get(gas.material).density;
            float above_density = MaterialRegistry::get(above.material).density;
            
            if (gas_density < above_density) {
                swap(gas, above);
                mark_dirty(local_x, local_y - 1);
                return;
            }
        }
    }
    
    // 2. 尝试对角线向上移动
    int diag_dirs[2] = {-1, 1};
    int diag_dir = diag_dirs[rand() % 2];
    
    if (local_x + diag_dir >= 0 && local_x + diag_dir < SIZE + BORDER * 2 &&
        local_y > 0) {
        Pixel& diag = get(local_x + diag_dir, local_y - 1);
        
        if (diag.is_empty()) {
            swap(gas, diag);
            mark_dirty(local_x + diag_dir, local_y - 1);
            return;
        }
    }
    
    // 3. 尝试横向移动(扩散)
    int side_dirs[2] = {-1, 1};
    int side_dir = side_dirs[rand() % 2];
    
    if (local_x + side_dir >= 0 && local_x + side_dir < SIZE + BORDER * 2) {
        Pixel& side = get(local_x + side_dir, local_y);
        
        if (side.is_empty()) {
            swap(gas, side);
            mark_dirty(local_x + side_dir, local_y);
            return;
        }
    }
}
```

---

## 8. 性能优化总结

### 8.1 关键优化技术

| 技术 | 性能提升 | 实现难度 |
|------|---------|---------|
| 从下往上更新 | 确保正确性 | 低 |
| 64×64分块 | 并行基础 | 中 |
| 脏矩形优化 | 90%+减少更新量 | 中 |
| 4-pass棋盘格 | 完全并行化 | 高 |
| 密度置换 | 真实物理 | 低 |
| 边界扩展 | 避免锁 | 中 |

### 8.2 性能分析

**典型场景**: 512×512世界, 50%活跃像素

**无优化**:
- 更新像素: 262,144
- 时间: ~20ms (50 FPS)

**脏矩形优化**:
- 更新像素: ~26,000 (10%)
- 时间: ~2ms (500 FPS)

**棋盘格并行(4核)**:
- 更新像素: ~26,000
- 时间: ~0.5ms (2000 FPS)

**实际瓶颈**: 渲染、反应检查、温度模拟

---

## 9. 实施检查清单

### ✅ 必须实现
- [ ] 从下往上的更新顺序
- [ ] 64×64分块系统
- [ ] 脏矩形优化
- [ ] 4-pass棋盘格并行
- [ ] 32像素边界扩展
- [ ] 密度置换系统

### ✅ 推荐实现
- [ ] 火焰传播系统
- [ ] 气体物理系统
- [ ] 材料反应系统
- [ ] 温度模拟

### ✅ 可选实现
- [ ] 刚体系统
- [ ] 爆炸坍塌
- [ ] 自然现象(草生长等)

---

## 10. 常见陷阱

### ❌ 错误1: 从上往下更新
```cpp
// 错误
for (int y = 0; y < HEIGHT; ++y) {
    for (int x = 0; x < WIDTH; ++x) {
        update_pixel(x, y);
    }
}

// 正确
for (int y = HEIGHT - 1; y >= 0; --y) {
    for (int x = 0; x < WIDTH; ++x) {
        update_pixel(x, y);
    }
}
```

### ❌ 错误2: 简单并行化
```cpp
// 错误: 会导致竞态条件
#pragma omp parallel for
for (int i = 0; i < pixels.size(); ++i) {
    update_pixel(i);
}

// 正确: 使用棋盘格模式
for (int pass = 0; pass < 4; ++pass) {
    #pragma omp parallel for
    for (int chunk_id : get_chunks_for_pass(pass)) {
        chunks[chunk_id]->update(pass);
    }
}
```

### ❌ 错误3: 忽略密度
```cpp
// 错误: 所有液体行为相同
if (below.is_liquid()) {
    swap(current, below);
}

// 正确: 考虑密度
if (below.is_liquid()) {
    if (current.density > below.density) {
        swap(current, below);
    }
}
```

### ❌ 错误4: 过度使用锁
```cpp
// 错误: 每个像素加锁
std::mutex pixel_mutex;
void update_pixel(int x, int y) {
    std::lock_guard<std::mutex> lock(pixel_mutex);
    // ...
}

// 正确: 使用棋盘格避免锁
// 无需任何锁!
```

---

## 📚 总结

Noita的核心技术在于:
1. **简单规则,复杂涌现** - 每个像素遵循简单规则
2. **性能优先设计** - 从架构层面考虑性能
3. **创造性解决冲突** - 棋盘格模式解决多线程冲突
4. **细节决定成败** - 更新顺序、密度、边界处理等细节至关重要

通过实现这些技术,我们可以构建一个真正Noita级别的像素物理引擎。
