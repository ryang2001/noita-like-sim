# Noita-like Pixel Physics Engine - 快速开始指南

## 🚀 快速开始

### 前置要求

**必需**:
- C++17兼容编译器
- CMake 3.15+
- SDL2(已包含在external/SDL2-2.30.1/)

**可选**:
- Visual Studio 2019/2022 (Windows)
- MinGW-w64 (Windows)
- GCC/Clang (Linux/Mac)

### 构建步骤

#### Windows (Visual Studio)

```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 生成Visual Studio项目
cmake .. -G "Visual Studio 17 2022" -A x64

# 3. 编译
cmake --build . --config Release

# 4. 运行
cd Release
demo.exe
```

#### Windows (MinGW)

```bash
# 1. 创建构建目录
mkdir build_mingw
cd build_mingw

# 2. 生成MinGW项目
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc

# 3. 编译
mingw32-make -j8

# 4. 复制SDL2.dll
cp ../external/SDL2-2.30.1/lib/x64/SDL2.dll .

# 5. 运行
./demo.exe
```

#### Linux/Mac

```bash
# 1. 安装SDL2
sudo apt-get install libsdl2-dev  # Ubuntu/Debian
brew install sdl2                  # Mac

# 2. 创建构建目录
mkdir build
cd build

# 3. 生成项目
cmake ..

# 4. 编译
make -j8

# 5. 运行
./demo
```

## 🎮 基本操作

### 鼠标控制

| 操作 | 功能 |
|------|------|
| 左键 | 绘制材料 |
| 右键 | 擦除 |
| 滚轮 | 缩放 |

### 键盘控制

| 按键 | 功能 |
|------|------|
| 方向键/WASD | 移动相机 |
| [ / ] | 减小/增大画笔大小 |
| 1-0, Q-U | 选择材料 |
| 空格键 | 暂停/恢复 |
| C | 清空世界 |
| G | 生成洞穴 |
| H | 生成地表 |
| Ctrl+S | 保存世界 |
| Ctrl+L | 加载世界 |
| ESC | 退出 |

### Debug功能

| 按键 | 功能 |
|------|------|
| F1 | 切换debug模式 |
| F2 | 显示chunk边界 |
| F3 | 显示FPS |
| F4 | 显示chunk更新状态(绿色=已更新,红色=未更新) |
| F5 | 每个pass渲染(显示4-pass更新过程) |
| F6 | 显示线程活动(黄色=正在更新) |
| F7 | 降低FPS(最小10) |
| F8 | 提高FPS(最大120) |

## 🎨 材料列表

| 按键 | 材料 | 类型 |
|------|------|------|
| 1 | empty | 空 |
| 2 | sand | 粉末 |
| 3 | water | 液体 |
| 4 | stone | 固体 |
| 5 | wood | 固体 |
| 6 | fire | 等离子体 |
| 7 | steam | 气体 |
| 8 | oil | 液体 |
| 9 | lava | 液体 |
| 0 | gunpowder | 粉末 |
| Q | smoke | 气体 |
| W | acid | 液体 |
| E | ice | 固体 |
| R | snow | 粉末 |
| T | plasma | 等离子体 |
| Y | electricity | 能量 |
| U | void | 能量 |

## 🔬 实验示例

### 示例1: 液体流动

1. 选择`water`(按键3)
2. 在屏幕上方绘制
3. 观察液体向下流动
4. 液体会遇到障碍物后向左右扩散

### 示例2: 粉末下落

1. 选择`sand`(按键2)
2. 在屏幕上方绘制
3. 观察粉末向下下落
4. 粉末会堆积成小山

### 示例3: 火焰燃烧

1. 选择`wood`(按键5)
2. 绘制一些木头
3. 选择`fire`(按键6)
4. 在木头附近绘制火焰
5. 观察火焰燃烧木头

### 示例4: 熔岩冷却

1. 选择`lava`(按键9)
2. 绘制熔岩
3. 选择`water`(按键3)
4. 在熔岩附近绘制水
5. 观察熔岩冷却成石头

### 示例5: 酸腐蚀

1. 选择`stone`(按键4)
2. 绘制一些石头
3. 选择`acid`(按键W)
4. 在石头上绘制酸
5. 观察酸腐蚀石头

### 示例6: 棋盘格更新(F5)

1. 按`F1`开启debug模式
2. 按`F2`显示chunk边界
3. 按`F4`显示chunk更新状态
4. 按`F5`开启每个pass渲染
5. 观察绿色chunk逐渐扩散(棋盘格模式)

### 示例7: 线程活动(F6)

1. 按`F1`开启debug模式
2. 按`F2`显示chunk边界
3. 按`F6`显示线程活动
4. 观察黄色chunk快速切换
5. 可以看到多个线程同时工作

### 示例8: 慢动作(F7/F8)

1. 按`F7`降低FPS到10-20
2. 观察慢动作效果
3. 可以清楚看到像素移动过程
4. 按`F8`恢复到60FPS

## 🐛 调试技巧

### 1. 查看更新模式

**步骤**:
1. 按`F1`开启debug模式
2. 按`F2`显示chunk边界
3. 按`F4`显示chunk更新状态

**效果**:
- 绿色: chunk在当前帧被更新
- 红色: chunk在当前帧未被更新
- 可以看到棋盘格更新模式

### 2. 查看4-pass更新过程

**步骤**:
1. 按`F1`开启debug模式
2. 按`F2`显示chunk边界
3. 按`F5`开启每个pass渲染

**效果**:
- 每个pass渲染1次
- 可以清楚看到4-pass的更新顺序
- 每个pass之间有50ms延迟

### 3. 查看线程活动

**步骤**:
1. 按`F1`开启debug模式
2. 按`F2`显示chunk边界
3. 按`F6`显示线程活动

**效果**:
- 黄色: 线程正在更新的chunk
- 可以看到多个线程同时工作
- 可以观察线程调度

### 4. 慢动作观察

**步骤**:
1. 按`F7`降低FPS到10-20
2. 绘制一些材料
3. 观察慢动作效果

**效果**:
- 可以清楚看到像素移动过程
- 可以观察液体流动
- 可以观察气体上升

### 5. 快速恢复

**步骤**:
1. 按`F8`提高FPS到60
2. 按`F5`关闭每个pass渲染
3. 按`F6`关闭线程活动显示

**效果**:
- 恢复正常速度
- 恢复正常渲染

## 📊 性能监控

### FPS显示

1. 按`F3`显示FPS
2. 观察FPS值
3. 调整画笔大小
4. 观察FPS变化

### 更新时间

1. 按`F3`显示FPS
2. 观察Update时间
3. 调整画笔大小
4. 观察Update时间变化

### 块更新统计

1. 按`F1`开启debug模式
2. 按`F4`显示chunk更新状态
3. 观察绿色chunk数量
4. 可以看到每帧更新了多少chunk

## 🎯 最佳实践

### 1. 性能优化

- 使用较小的画笔大小
- 避免绘制过多材料
- 使用适当的FPS
- 使用并行更新

### 2. 调试技巧

- 使用慢动作观察细节
- 使用chunk更新状态查看更新模式
- 使用线程活动查看线程调度
- 使用4-pass渲染观察更新过程

### 3. 实验建议

- 尝试不同的材料组合
- 观察材料之间的反应
- 测试不同的画笔大小
- 测试不同的FPS

## ❓ 常见问题

### Q: 程序启动失败

**A**: 检查SDL2.dll是否在可执行文件同一目录

### Q: 材料不移动

**A**: 检查是否按了空格键暂停

### Q: FPS太低

**A**: 减小画笔大小,减少绘制材料数量

### Q: 看不到chunk边界

**A**: 按`F1`开启debug模式,再按`F2`显示chunk边界

### Q: 看不到更新过程

**A**: 按`F5`开启每个pass渲染

### Q: 看不到线程活动

**A**: 按`F6`显示线程活动

### Q: 速度太快看不清

**A**: 按`F7`降低FPS到10-20

## 📚 进阶学习

### 1. 查看源代码

- `include/core/Chunk.hpp`: Chunk系统定义
- `src/core/Chunk.cpp`: Chunk系统实现
- `include/core/World.hpp`: World系统定义
- `src/core/World.cpp`: World系统实现
- `src/demo_simple.cpp`: 主程序

### 2. 添加新材料

编辑`config/materials/basic_materials.json`:

```json
{
    "name": "my_material",
    "state": "liquid",
    "density": 1.0,
    "flammability": 0.0,
    "melting_point": 0.0,
    "freezing_point": 0.0,
    "boiling_point": 100.0,
    "color": "0xFFFF0000"
}
```

### 3. 添加新反应

编辑`config/reactions/basic_reactions.json`:

```json
{
    "material": "fire",
    "target": "my_material",
    "result": "smoke",
    "probability": 0.5
}
```

### 4. 修改世界大小

编辑`src/demo_simple.cpp`:

```cpp
WorldConfig config;
config.width = 1024;  // 修改宽度
config.height = 1024; // 修改高度
world = new World(config);
```

## 🎉 开始探索!

现在你已经掌握了基本操作,开始探索Noita-like像素物理引擎吧!

- 尝试不同的材料组合
- 观察材料之间的反应
- 测试不同的画笔大小
- 使用Debug功能了解内部机制

祝你玩得开心! 🎯
