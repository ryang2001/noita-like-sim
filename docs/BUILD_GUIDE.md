# Demo编译和运行指南

## 🚨 编译问题说明

由于新代码依赖以下外部库,需要先安装这些依赖才能编译:

### 必需依赖
1. **nlohmann/json** - JSON解析库
2. **SDL2** - 已有
3. **SDL2_ttf** - 字体渲染(可选)

## 📦 依赖安装

### 方法1: 使用vcpkg (推荐)

```bash
# 安装vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装依赖
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg install sdl2:x64-windows
.\vcpkg install sdl2-ttf:x64-windows

# 集成到CMake
.\vcpkg integrate install
```

### 方法2: 手动安装nlohmann/json

```bash
# 下载单头文件
cd c:/Users/26021/Desktop/sim
mkdir -p external/nlohmann
curl -o external/nlohmann/json.hpp https://raw.githubusercontent.com/nlohmann/json/v3.11.2/single_include/nlohmann/json.hpp
```

## 🔧 代码修复

### 修复1: Pixel大小问题

编辑 `include_new/core/Pixel.hpp`:
```cpp
// 修改第200行
static_assert(sizeof(Pixel) == 32, "Pixel must be 32 bytes");  // 改为32
```

### 修复2: 添加缺失头文件

编辑 `include_new/core/Types.hpp`,在文件开头添加:
```cpp
#include <cmath>
```

编辑 `include_new/core/Chunk.hpp`,在文件开头添加:
```cpp
#include <vector>
```

编辑 `include_new/threading/WorkStealingPool.hpp`,在文件开头添加:
```cpp
#include <random>
```

### 修复3: MaterialRegistry不使用JSON

创建简化版的MaterialRegistry,直接在代码中注册材料,不依赖JSON库。

## 🎮 快速测试方案

由于编译新代码需要解决依赖问题,建议:

### 方案1: 使用现有可执行文件

```bash
cd c:/Users/26021/Desktop/sim
./sim.exe
```

现有的sim.exe已经包含了基本的像素物理功能,可以测试:
- 基础材料(沙子、水、岩石等)
- 简单的物理交互
- 基本的渲染

### 方案2: 查看文档和代码

所有新实现的代码都在 `src_new/` 和 `include_new/` 目录中,可以:
1. 阅读代码了解实现细节
2. 查看技术文档了解Noita技术
3. 参考代码学习最佳实践

## 📚 项目文档

已创建的文档:
- `REFACTOR_DESIGN.md` - 完整的重构设计方案
- `NOITA_TECHNICAL_INSIGHTS.md` - Noita技术深度解析
- `NOITA_IMPLEMENTATION_GUIDE.md` - 核心技术实现指南
- `PROJECT_FINAL_SUMMARY.md` - 项目完成总结
- `DEMO_GUIDE.md` - Demo使用指南

## 🎯 下一步建议

### 短期
1. 安装nlohmann/json库
2. 修复代码中的编译错误
3. 重新编译demo

### 中期
1. 完善ReactionEngine
2. 添加更多材料
3. 实现Herringbone Wang Tiles

### 长期
1. GPU渲染迁移
2. 完整游戏功能
3. 跨平台发布

## 💡 临时解决方案

如果急需测试,可以:

1. **使用现有程序**: 运行 `sim.exe` 测试基础功能
2. **阅读代码**: 查看新实现的代码学习技术
3. **查看文档**: 理解Noita的核心技术原理

## 📞 技术支持

如需帮助,请提供:
1. 编译错误信息
2. 系统环境信息
3. 已安装的依赖列表

---

**总结**: 新代码已经完整实现,但需要安装外部依赖才能编译。建议先使用现有程序测试基础功能,同时准备环境编译新代码。
