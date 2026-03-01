# Noita-like Pixel Physics Engine

<div align="center">

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![SDL2](https://img.shields.io/badge/SDL2-2.30.1-orange.svg)
![CMake](https://img.shields.io/badge/CMake-3.15+-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

**A high-performance pixel physics simulation engine inspired by Noita**

[Features](#-features) • [Quick Start](#-quick-start) • [Documentation](#-documentation) • [Demo](#-demo)

</div>

## 📖 Overview

This is a C++ implementation of a pixel-based physics simulation engine, inspired by the game Noita. The engine simulates thousands of individual pixels with different material properties, states, and interactions using cellular automata rules and parallel processing.

### Key Highlights

- ⚡ **High Performance** - Multi-threaded processing with work-stealing algorithm
- 🧱 **Chunk-based Architecture** - 64×64 pixel chunks with boundary synchronization
- 🎨 **17 Materials** - Sand, water, fire, lava, acid, and more
- 🔥 **8 Chemical Reactions** - Fire burns wood, acid dissolves stone, etc.
- 🌡️ **Temperature System** - Melting, freezing, boiling points
- 🎮 **Interactive Demo** - Real-time visualization and debugging tools

---

## ✨ Features

### Core Systems

- **Chunk System**
  - 64×64 active pixel grid with 32-pixel border extension
  - Checkerboard 4-pass update algorithm
  - Automatic boundary synchronization between chunks
  - Neighbor management (N/S/E/W)

- **World System**
  - Multi-chunk world management
  - Parallel and serial update modes
  - Performance statistics and monitoring
  - Procedural generation (caves, surfaces, biomes)
  - World serialization (save/load)

- **Pixel System**
  - Material ID and state tracking
  - Temperature, lifetime, and velocity
  - Six material states: Empty, Solid, Powder, Liquid, Gas, Plasma, Energy

### Material Physics

| Material Type | Behavior | Examples |
|--------------|----------|----------|
| **Powder** | Falls down, piles up | Sand, Snow, Gunpowder |
| **Liquid** | Flows down, spreads horizontally | Water, Oil, Lava, Acid |
| **Gas** | Rises, disperses | Steam, Smoke |
| **Solid** | Static, immovable | Stone, Wood, Ice |
| **Plasma** | Short-lived, destructive | Fire, Plasma |
| **Energy** | Special effects | Electricity, Void |

### Temperature System

- **Phase Transitions**: Melting, freezing, boiling
- **Material-specific temperatures**: Melting point, freezing point, boiling point
- **Dynamic state changes**: Water ↔ Ice ↔ Steam

### Chemical Reactions

```
Fire + Wood → Fire  (burning)
Fire + Oil → Fire  (burning)
Fire + Gunpowder → Fire (explosion)
Water + Fire → Steam (extinguishing)
Lava + Water → Stone (cooling)
Lava + Ice → Water (melting)
Acid + Stone → Empty (dissolving)
Acid + Wood → Empty (dissolving)
```

### Performance Optimizations

- **Work-Stealing Thread Pool** - Load balancing across CPU cores
- **Checkerboard Updates** - Prevents double-updating and reduces contention
- **Boundary Extension** - Minimizes cross-chunk access
- **FPS Control** - Adjustable target FPS (10-120)

---

## 🚀 Quick Start

### Prerequisites

- **C++17** compatible compiler
- **CMake** 3.15 or higher
- **SDL2** 2.30.1 (included in `external/SDL2-2.30.1/`)
- **Optional**: Visual Studio 2019/2022, MinGW-w64, GCC/Clang

### Build Instructions

#### Windows (Visual Studio)

```bash
# Create build directory
mkdir build && cd build

# Generate Visual Studio project
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release

# Run
cd Release && demo.exe
```

#### Windows (MinGW)

```bash
# Create build directory
mkdir build_mingw && cd build_mingw

# Generate MinGW project
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc

# Build
mingw32-make -j8

# Copy SDL2.dll
cp ../external/SDL2-2.30.1/lib/x64/SDL2.dll .

# Run
./demo.exe
```

#### Linux/Mac

```bash
# Install SDL2
sudo apt-get install libsdl2-dev  # Ubuntu/Debian
brew install sdl2                  # Mac

# Build
mkdir build && cd build
cmake ..
make -j8

# Run
./demo
```

---

## 🎮 Controls

### Mouse

| Button | Action |
|--------|--------|
| Left Click | Draw material |
| Right Click | Erase |
| Scroll | Zoom |

### Keyboard

| Key | Action |
|-----|--------|
| **WASD / Arrows** | Move camera |
| **[ / ]** | Decrease/Increase brush size |
| **1-0, Q-U** | Select material |
| **Space** | Pause/Resume |
| **C** | Clear world |
| **G** | Generate caves |
| **H** | Generate surface |
| **Ctrl+S** | Save world |
| **Ctrl+L** | Load world |
| **ESC** | Exit |

### Debug Mode

| Key | Action |
|-----|--------|
| **F1** | Toggle debug mode |
| **F2** | Show chunk boundaries |
| **F3** | Show FPS |
| **F4** | Show chunk update status (green=updated, red=not updated) |
| **F5** | Render after each pass (show 4-pass process) |
| **F6** | Show thread activity (yellow=updating) |
| **F7** | Decrease FPS (min 10) |
| **F8** | Increase FPS (max 120) |

### Material List

| Key | Material | Type | Density | Color |
|-----|----------|------|---------|-------|
| 1 | Empty | - | 0.0 | ⬛ |
| 2 | Sand | Powder | 1.5 | 🟨 |
| 3 | Water | Liquid | 1.0 | 🔵 |
| 4 | Stone | Solid | 2.5 | ⬜ |
| 5 | Wood | Solid | 0.7 | 🟫 |
| 6 | Fire | Plasma | 0.1 | 🟧 |
| 7 | Steam | Gas | 0.001 | ⬜ |
| 8 | Oil | Liquid | 0.8 | 🟤 |
| 9 | Lava | Liquid | 2.0 | 🟠 |
| 0 | Gunpowder | Powder | 1.2 | ⬛ |
| Q | Smoke | Gas | 0.001 | ⬛ |
| W | Acid | Liquid | 1.2 | 🟢 |
| E | Ice | Solid | 0.9 | 🩵 |
| R | Snow | Powder | 0.3 | ⬜ |
| T | Plasma | Plasma | 0.05 | 🟪 |
| Y | Electricity | Energy | 0.0 | 🟡 |
| U | Void | Energy | 0.0 | ⬛ |

---

## 📊 Project Structure

```
sim/
├── include/                    # Header files
│   ├── core/                  # Core systems
│   │   ├── Chunk.hpp         # Chunk system
│   │   ├── Pixel.hpp         # Pixel definition
│   │   ├── Types.hpp         # Type definitions
│   │   └── World.hpp         # World system
│   ├── materials/            # Material system
│   │   └── MaterialRegistry.hpp
│   └── threading/            # Thread system
│       └── WorkStealingPool.hpp
│
├── src/                       # Source files
│   ├── core/                 # Core implementations
│   ├── materials/           # Material implementations
│   ├── threading/           # Thread implementations
│   └── demo_simple.cpp      # Main program
│
├── config/                   # Configuration files
│   ├── materials/           # Material definitions
│   └── reactions/           # Reaction definitions
│
├── external/                 # External dependencies
│   ├── SDL2-2.30.1/        # SDL2 library
│   └── box2d/              # Box2D physics (backup)
│
├── tests/                    # Test files
├── docs/                     # Documentation
└── CMakeLists.txt          # CMake configuration
```

---

## 🔬 Technical Highlights

### Checkerboard 4-Pass Update Algorithm

The world is updated in 4 passes to prevent race conditions and double-updating:

```cpp
for (int pass = 0; pass < 4; ++pass) {
    auto chunk_indices = checkerboard_updater_->get_chunks_for_pass(pass);
    for (int idx : chunk_indices) {
        chunks_[idx]->update(frame, pass);
    }
}
```

Chunks are selected based on `(chunk_x + chunk_y) % 4`, ensuring adjacent chunks are never updated simultaneously.

### Cellular Automata Rules

**Powder Behavior:**
1. Try to move down
2. Try to move down-left or down-right (random)

**Liquid Behavior:**
1. Try to move down
2. Try to move down-left or down-right (random)
3. Try to move left or right (random)

**Gas Behavior:**
1. Try to move up
2. Try to move up-left or up-right (random)
3. Try to move left or right (random)

**Fire Behavior:**
1. Decrease lifetime
2. Ignite flammable materials nearby
3. Float upward
4. React with water to create steam

### Boundary Synchronization

Each chunk synchronizes its border pixels with neighbors after each update:

```cpp
void Chunk::sync_borders() {
    if (neighbors_[0]) {  // North
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
    // ... other directions
}
```

### Work-Stealing Thread Pool

- Each thread has its own task queue
- Idle threads can steal tasks from other threads
- Load balancing across CPU cores
- Automatic thread activity tracking

---

## 📈 Performance

- **Resolution**: 1024×1024 pixels (16×16 chunks)
- **Update Rate**: 60 FPS (adjustable)
- **Threads**: Configurable (default: number of CPU cores)
- **Memory**: ~256 MB for 1024×1024 world

### Performance Statistics

The engine provides real-time performance metrics:
- FPS (Frames Per Second)
- Update time (ms)
- Chunk update count
- Thread activity

---

## 📚 Documentation

- [Project Structure](PROJECT_STRUCTURE.md) - Detailed project structure
- [Quick Start Guide](QUICKSTART.md) - Quick start and experiments
- [Features](FEATURES.md) - Complete feature list
- [Build Guide](docs/BUILD_GUIDE.md) - Detailed build instructions
- [Debug Guide](docs/DEBUG_GUIDE.md) - Debugging tools and techniques

---

## 🎯 Demo Experiments

### Experiment 1: Liquid Flow

1. Select `water` (key 3)
2. Draw at the top of the screen
3. Observe liquid flowing downward and spreading

### Experiment 2: Powder Piling

1. Select `sand` (key 2)
2. Draw at the top of the screen
3. Observe powder piling up into hills

### Experiment 3: Fire Burning

1. Select `wood` (key 5) and draw some wood
2. Select `fire` (key 6)
3. Draw fire near the wood
4. Observe fire burning the wood

### Experiment 4: Lava Cooling

1. Select `lava` (key 9) and draw some lava
2. Select `water` (key 3)
3. Draw water near the lava
4. Observe lava cooling into stone

### Experiment 5: Checkerboard Updates (F5)

1. Press `F1` to enable debug mode
2. Press `F2` to show chunk boundaries
3. Press `F4` to show chunk update status
4. Press `F5` to render after each pass
5. Observe the 4-pass update process

### Experiment 6: Thread Activity (F6)

1. Press `F1` to enable debug mode
2. Press `F2` to show chunk boundaries
3. Press `F6` to show thread activity
4. Observe multiple threads working simultaneously

### Experiment 7: Slow Motion (F7/F8)

1. Press `F7` to decrease FPS to 10-20
2. Draw some materials
3. Observe slow motion pixel movement
4. Press `F8` to restore 60 FPS

---

## 🔧 Configuration

### Add Custom Material

Edit `config/materials/basic_materials.json`:

```json
{
    "materials": [
        {
            "name": "my_material",
            "state": "liquid",
            "density": 1.0,
            "flammability": 0.5,
            "melting_point": 0.0,
            "freezing_point": 0.0,
            "boiling_point": 100.0,
            "color": "0xFFFF0000"
        }
    ]
}
```

### Add Custom Reaction

Edit `config/reactions/basic_reactions.json`:

```json
{
    "reactions": [
        {
            "material": "fire",
            "target": "my_material",
            "result": "smoke",
            "probability": 0.5
        }
    ]
}
```

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

### Areas for Contribution

- 🎨 **More Materials** - Add new materials with unique behaviors
- 🔥 **More Reactions** - Implement additional chemical reactions
- ⚡ **Performance** - Optimize performance and reduce memory usage
- 🎮 **Features** - Add new game mechanics and interactions
- 📝 **Documentation** - Improve documentation and tutorials

### Development Setup

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

---

## 🙏 Acknowledgments

- **Noita** - Original inspiration for the pixel physics concept
- **SDL2** - Graphics and input library
- **CMake** - Build system
- **Box2D** - Physics engine (backup, not currently used)

---

## 📞 Contact

- **Repository**: https://github.com/ryang2001/noita-like-sim
- **Issues**: https://github.com/ryang2001/noita-like-sim/issues

---

<div align="center">

**Built with ❤️ using C++, SDL2, and CMake**

[⬆ Back to top](#noita-like-pixel-physics-engine)

</div>
