# 块边界传递问题分析和修复方案

## 问题分析

### 当前实现的问题

1. **边界同步只是复制**: `sync_borders()`只是将邻居块的边界像素复制到当前块的边界区域,这会导致:
   - 像素在边界处"卡住"
   - 像素无法真正从一个块移动到另一个块

2. **边界检查不完整**: 当像素移动到边界外时,代码没有正确处理跨块移动

3. **脏矩形扩展不足**: 扩展1像素可能不够,特别是对于液体和气体

## Noita的正确实现方式

### 方案1: 使用边界扩展进行跨块移动(推荐)

在Noita中,每个块有32像素的边界扩展:
- **有效区域**: [BORDER, BORDER+SIZE-1] = [32, 95]
- **边界区域**: [-BORDER, BORDER-1] 和 [SIZE, SIZE+BORDER-1]

**关键点**:
1. 像素可以在边界区域内移动
2. 边界同步在每个pass之前执行
3. 当像素移动到边界区域时,它实际上是在邻居块的副本上移动
4. 下一次边界同步会将变化传回源块

**实现步骤**:
1. 允许像素移动到边界区域(当前已实现)
2. 在每个pass之前同步边界(已修复)
3. 边界同步需要双向同步(需要修复)

### 方案2: 直接跨块移动

当像素移动到边界外时:
1. 计算目标块和目标坐标
2. 在目标块中设置像素
3. 在源块中清除像素

**缺点**: 需要锁机制,影响并行性能

## 推荐修复方案

### 修复1: 双向边界同步

当前的`sync_borders()`只是单向复制(从邻居到当前块)。需要实现双向同步:

```cpp
void Chunk::sync_borders() {
    // 北边界: 双向同步
    if (neighbors_[0]) {
        for (int x = 0; x < SIZE; ++x) {
            // 从邻居复制到我的边界
            get(x, -1) = neighbors_[0]->get(x, SIZE - 1);
            // 从我的边界复制到邻居
            neighbors_[0]->get(x, SIZE) = get(x, 0);
        }
    }
    // ... 其他边界类似
}
```

### 修复2: 边界像素转移

当像素移动到边界区域时,需要将其转移到邻居块:

```cpp
void Chunk::transfer_border_pixels() {
    // 北边界: 将边界区域的像素转移到邻居
    if (neighbors_[0]) {
        for (int x = 0; x < SIZE; ++x) {
            Pixel& border_pixel = get(x, -1);
            if (!border_pixel.is_empty()) {
                // 转移到邻居块
                neighbors_[0]->set_world(world_x_ + x, world_y_ - 1, border_pixel);
                border_pixel.clear();
            }
        }
    }
    // ... 其他边界类似
}
```

### 修复3: 在每个pass之后执行像素转移

```cpp
void World::update_parallel(float dt) {
    for (int pass = 0; pass < 4; ++pass) {
        sync_all_borders();  // 同步边界
        update_chunks(pass);  // 更新块
        transfer_border_pixels();  // 转移边界像素
    }
}
```

## 最简单的修复方案

考虑到当前代码结构,最简单的修复是:

1. **扩大脏矩形范围**: 确保边界区域也被更新
2. **在每个pass之前同步边界**: 已完成
3. **允许像素在边界区域移动**: 已完成

但是,这仍然不能解决像素"卡"在边界的问题。

## 终极解决方案

我建议采用**方案1的简化版本**:

1. 边界区域存储邻居像素的副本
2. 像素可以在边界区域移动
3. **关键**: 当像素移动到边界区域时,标记邻居块为脏
4. 下一次边界同步会将变化传回

这需要修改`mark_dirty`函数:

```cpp
void Chunk::mark_dirty(int local_x, int local_y) {
    dirty_rect_.expand(local_x, local_y);
    is_active_ = true;

    // 如果在边界区域,标记邻居为脏
    if (local_x < 0 && neighbors_[3]) {
        neighbors_[3]->is_active_ = true;
    }
    if (local_x >= SIZE && neighbors_[1]) {
        neighbors_[1]->is_active_ = true;
    }
    if (local_y < 0 && neighbors_[0]) {
        neighbors_[0]->is_active_ = true;
    }
    if (local_y >= SIZE && neighbors_[2]) {
        neighbors_[2]->is_active_ = true;
    }
}
```

这样,当像素移动到边界时,邻居块会在下一帧被更新,边界同步会将像素传过去。
