# 液体穿过其他元素问题修复总结

## 📋 问题分析

### 问题描述

**液体直接穿过其他元素**,包括固体材料(如石头、木头等)。

### 问题根源

**`try_move`函数缺少对固体材料的检查**:

```cpp
bool Chunk::try_move(int from_x, int from_y, int to_x, int to_y) {
    // ...
    
    // 检查目标位置
    if (to.is_empty()) {
        // 直接移动
        to = from;
        from = Pixel();
        return true;
    }
    
    // ❌ 缺少固体材料检查!
    // 液体会直接穿过固体材料
    
    // 同种液体/气体不交换
    if (from.material == to.material) {
        return false;
    }
    
    // ...
}
```

**影响**:
- ❌ 液体可以穿过石头
- ❌ 液体可以穿过木头
- ❌ 液体可以穿过任何非空材料

## 🔧 修复方案

### 修复内容

**位置**: `src/core/Chunk.cpp` - `Chunk::try_move()`

**修复代码**:
```cpp
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
    
    // ✅ 关键修复: 固体材料不能被穿透!
    if (to.is_solid()) {
        return false;
    }
    
    // 同种液体/气体不交换(避免无限交换)
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
```

## 📊 修复效果对比

### 修复前

**问题**:
- ❌ 液体可以直接穿过石头
- ❌ 液体可以直接穿过木头
- ❌ 液体可以直接穿过任何非空材料
- ❌ 违反物理规律

### 修复后

**效果**:
- ✅ 液体不能穿过石头
- ✅ 液体不能穿过木头
- ✅ 液体不能穿过固体材料
- ✅ 符合物理规律

## 🎯 验证测试

### 测试1: 液体不能穿过石头

**步骤**:
1. 选择`stone`
2. 绘制一堵墙
3. 选择`water`
4. 在墙上方绘制水

**预期结果**:
- ✅ 水不能穿过石头
- ✅ 水会堆积在石头上方
- ✅ 水会向左右扩散

### 测试2: 液体不能穿过木头

**步骤**:
1. 选择`wood`
2. 绘制一堵墙
3. 选择`water`
4. 在墙上方绘制水

**预期结果**:
- ✅ 水不能穿过木头
- ✅ 水会堆积在木头上方
- ✅ 水会向左右扩散

### 测试3: 液体可以置换气体

**步骤**:
1. 选择`steam`
2. 绘制一些蒸汽
3. 选择`water`
4. 在蒸汽下方绘制水

**预期结果**:
- ✅ 水可以置换蒸汽
- ✅ 符合物理规律

### 测试4: 液体可以置换密度更小的液体

**步骤**:
1. 选择`oil`(密度0.8)
2. 绘制一些油
3. 选择`water`(密度1.0)
4. 在油下方绘制水

**预期结果**:
- ✅ 水可以置换油
- ✅ 符合物理规律

## 📝 总结

### 问题根源

**`try_move`函数缺少对固体材料的检查**,导致液体可以穿过任何非空材料。

### 修复方案

**添加固体材料检查**:
```cpp
if (to.is_solid()) {
    return false;
}
```

### 修复效果

**正确性保证**:
- ✅ 液体不能穿过固体材料
- ✅ 液体可以置换气体
- ✅ 液体可以置换密度更小的液体
- ✅ 符合物理规律

### 符合Noita规范

**Noita的核心设计**:
- 简单的规则,复杂的结果
- 固体材料不能被穿透
- 液体可以置换气体
- 液体可以置换密度更小的液体

**修复后的实现**:
- ✅ 固体材料不能被穿透
- ✅ 液体可以置换气体
- ✅ 液体可以置换密度更小的液体
- ✅ 符合Noita的设计理念

## 🎉 结论

液体穿过其他元素问题已完全修复!

**修复内容**:
- ✅ 在`try_move`函数中添加固体材料检查
- ✅ 液体不能穿过固体材料
- ✅ 液体可以置换气体
- ✅ 液体可以置换密度更小的液体

**修复效果**:
- ✅ 液体不能穿过石头
- ✅ 液体不能穿过木头
- ✅ 液体不能穿过固体材料
- ✅ 符合物理规律

**验证结果**:
- ✅ 编译成功
- ✅ 程序运行正常
- ✅ 液体不能穿过固体材料
- ✅ 液体可以置换气体和密度更小的液体

液体穿过其他元素问题已完全修复,完全符合Noita的设计规范! 🎯
