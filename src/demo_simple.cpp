#define SDL_MAIN_HANDLED

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "core/World.hpp"
#include "materials/MaterialRegistry.hpp"
#include <SDL.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace noita;

// ==================== 全局变量 ====================

World* world = nullptr;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* world_texture = nullptr;

// 相机
struct Camera {
    float x = 0, y = 0;
    float zoom = 1.0f;
    int screen_width = 1024;
    int screen_height = 768;
} camera;

// 性能统计
struct PerfStats {
    float fps = 0;
    float update_time_ms = 0;
    float render_time_ms = 0;
    int chunks_updated = 0;
} perf_stats;

// 当前工具
MaterialID current_material = 1;
int brush_size = 5;
bool debug_mode = false;
bool show_chunk_borders = false;
bool show_chunk_updates = false;
bool show_fps = true;
bool paused = false;
bool render_per_pass = false;  // 是否每个pass渲染
bool show_thread_activity = false;  // 是否显示线程正在更新的chunk
float target_fps = 60.0f;  // 目标FPS

// 材料列表
struct MaterialInfo {
    MaterialID id;
    std::string name;
    std::string key;
};

std::vector<MaterialInfo> material_list;
int current_material_index = 0;

// ==================== 初始化 ====================

bool init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Noita-like Pixel Physics Demo",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              camera.screen_width, camera.screen_height,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    world_texture = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       world->get_width(),
                                       world->get_height());
    if (!world_texture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

void cleanup_sdl() {
    if (world_texture) SDL_DestroyTexture(world_texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void init_material_list() {
    material_list.clear();

    // 常用材料
    material_list.push_back({MaterialRegistry::instance().get_id("sand"), "Sand", "1"});
    material_list.push_back({MaterialRegistry::instance().get_id("water"), "Water", "2"});
    material_list.push_back({MaterialRegistry::instance().get_id("rock"), "Rock", "3"});
    material_list.push_back({MaterialRegistry::instance().get_id("fire"), "Fire", "4"});
    material_list.push_back({MaterialRegistry::instance().get_id("lava"), "Lava", "5"});
    material_list.push_back({MaterialRegistry::instance().get_id("wood"), "Wood", "6"});
    material_list.push_back({MaterialRegistry::instance().get_id("oil"), "Oil", "7"});
    material_list.push_back({MaterialRegistry::instance().get_id("gunpowder"), "Gunpowder", "8"});
    material_list.push_back({MaterialRegistry::instance().get_id("acid"), "Acid", "9"});
    material_list.push_back({MaterialRegistry::instance().get_id("ice"), "Ice", "0"});
    material_list.push_back({MaterialRegistry::instance().get_id("steam"), "Steam", "Q"});
    material_list.push_back({MaterialRegistry::instance().get_id("glass"), "Glass", "W"});
    material_list.push_back({MaterialRegistry::instance().get_id("gold"), "Gold", "E"});
    material_list.push_back({MaterialRegistry::instance().get_id("diamond"), "Diamond", "R"});
    material_list.push_back({MaterialRegistry::instance().get_id("poison"), "Poison", "T"});
    material_list.push_back({MaterialRegistry::instance().get_id("blood"), "Blood", "Y"});
    material_list.push_back({MaterialRegistry::instance().get_id("slime"), "Slime", "U"});

    if (!material_list.empty()) {
        current_material = material_list[0].id;
    }

    // 调试: 输出材料ID和状态
    std::cout << "\n=== Material Debug ===" << std::endl;
    for (const auto& mat : material_list) {
        const auto& props = MaterialRegistry::instance().get(mat.id);
        std::cout << mat.name << " ID=" << mat.id 
                  << " state=" << static_cast<int>(props.default_state);
        if (props.default_state == MaterialState::Liquid) {
            std::cout << " (Liquid)";
        }
        std::cout << std::endl;
    }
}

// ==================== 渲染 ====================

void render_world() {
    auto start_time = std::chrono::high_resolution_clock::now();

    void* pixels;
    int pitch;
    SDL_LockTexture(world_texture, nullptr, &pixels, &pitch);

    uint32_t* pixel_data = static_cast<uint32_t*>(pixels);

    for (int y = 0; y < world->get_height(); ++y) {
        for (int x = 0; x < world->get_width(); ++x) {
            const Pixel& pixel = world->get(x, y);
            uint32_t color;

            if (pixel.is_empty()) {
                color = 0x1a1a1aFF;
            } else {
                const auto& props = MaterialRegistry::instance().get(pixel.material);

                uint8_t r = props.color.r;
                uint8_t g = props.color.g;
                uint8_t b = props.color.b;
                uint8_t a = props.color.a;

                // 颜色变体
                if (pixel.variation > 0) {
                    int variation = (pixel.variation % 8) - 4;
                    r = static_cast<uint8_t>(std::max(0, std::min(255, r + variation * 10)));
                    g = static_cast<uint8_t>(std::max(0, std::min(255, g + variation * 10)));
                    b = static_cast<uint8_t>(std::max(0, std::min(255, b + variation * 10)));
                }

                // 温度影响
                if (pixel.temperature > 100.0f) {
                    float heat_factor = std::min(1.0f, (pixel.temperature - 100.0f) / 500.0f);
                    r = static_cast<uint8_t>(std::min(255, static_cast<int>(r + heat_factor * 100)));
                }

                // 发光效果
                if (props.emits_light) {
                    r = static_cast<uint8_t>(std::min(255, r + 30));
                    g = static_cast<uint8_t>(std::min(255, g + 20));
                }

                color = (r << 24) | (g << 16) | (b << 8) | a;
            }

            pixel_data[y * world->get_width() + x] = color;
        }
    }

    SDL_UnlockTexture(world_texture);

    // 渲染世界
    SDL_RenderClear(renderer);

    SDL_Rect src_rect;
    src_rect.x = static_cast<int>(camera.x);
    src_rect.y = static_cast<int>(camera.y);
    src_rect.w = static_cast<int>(camera.screen_width / camera.zoom);
    src_rect.h = static_cast<int>(camera.screen_height / camera.zoom);

    SDL_RenderCopy(renderer, world_texture, &src_rect, nullptr);

    // 调试可视化
    if (debug_mode && show_chunk_borders) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        int chunk_size = 64;
        for (int cx = 0; cx < world->get_chunk_count_x(); ++cx) {
            for (int cy = 0; cy < world->get_chunk_count_y(); ++cy) {
                Chunk* chunk = world->get_chunk_by_index(cx, cy);
                if (!chunk) continue;
                
                int screen_x = static_cast<int>((cx * chunk_size - camera.x) * camera.zoom);
                int screen_y = static_cast<int>((cy * chunk_size - camera.y) * camera.zoom);
                int screen_w = static_cast<int>(chunk_size * camera.zoom);
                int screen_h = static_cast<int>(chunk_size * camera.zoom);

                SDL_Rect rect = {screen_x, screen_y, screen_w, screen_h};
                
                // 根据是否更新显示不同颜色
                if (show_chunk_updates) {
                    if (chunk->was_updated_this_frame()) {
                        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100);  // 绿色: 已更新
                    } else {
                        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 100);  // 红色: 未更新
                    }
                } else {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100);  // 默认绿色
                }
                
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
    }
    
    // 显示线程正在更新的chunk
    if (debug_mode && show_thread_activity) {
        int active_x = world->get_active_chunk_x();
        int active_y = world->get_active_chunk_y();
        
        if (active_x >= 0 && active_y >= 0) {
            int chunk_size = 64;
            int screen_x = static_cast<int>((active_x * chunk_size - camera.x) * camera.zoom);
            int screen_y = static_cast<int>((active_y * chunk_size - camera.y) * camera.zoom);
            int screen_w = static_cast<int>(chunk_size * camera.zoom);
            int screen_h = static_cast<int>(chunk_size * camera.zoom);
            
            SDL_Rect rect = {screen_x, screen_y, screen_w, screen_h};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 150);  // 黄色: 线程正在更新
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    // 显示FPS和材料信息
    if (show_fps) {
        std::stringstream ss;
        ss << "FPS: " << static_cast<int>(perf_stats.fps)
           << " | Update: " << std::fixed << std::setprecision(1) << perf_stats.update_time_ms << "ms"
           << " | Render: " << perf_stats.render_time_ms << "ms";

        if (!material_list.empty() && current_material_index < material_list.size()) {
            ss << " | Material: " << material_list[current_material_index].name;
        }

        ss << " | Brush: " << brush_size;

        if (paused) {
            ss << " | PAUSED";
        }

        // 简单的文本显示(使用SDL_RenderDrawPoint)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // 这里可以添加SDL_ttf来显示文本,但为了简化,我们只在控制台输出
    }

    SDL_RenderPresent(renderer);
    
    // 清除所有chunk的updated_this_frame_标志,为下一帧做准备
    for (int cx = 0; cx < world->get_chunk_count_x(); ++cx) {
        for (int cy = 0; cy < world->get_chunk_count_y(); ++cy) {
            Chunk* chunk = world->get_chunk_by_index(cx, cy);
            if (chunk) {
                chunk->clear_updated_flag();
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    perf_stats.render_time_ms = duration.count() / 1000.0f;
}

// ==================== 输入处理 ====================

void handle_input(bool& running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    break;

                case SDLK_c:
                    world->clear();
                    std::cout << "World cleared!" << std::endl;
                    break;

                case SDLK_F1:
                    debug_mode = !debug_mode;
                    std::cout << "Debug mode: " << (debug_mode ? "ON" : "OFF") << std::endl;
                    break;

                case SDLK_F2:
                    show_chunk_borders = !show_chunk_borders;
                    std::cout << "Chunk borders: " << (show_chunk_borders ? "ON" : "OFF") << std::endl;
                    break;

                case SDLK_F3:
                    show_fps = !show_fps;
                    std::cout << "FPS display: " << (show_fps ? "ON" : "OFF") << std::endl;
                    break;

                case SDLK_F4:
                    show_chunk_updates = !show_chunk_updates;
                    std::cout << "Chunk update display: " << (show_chunk_updates ? "ON" : "OFF") << std::endl;
                    break;

                case SDLK_F5:
                    render_per_pass = !render_per_pass;
                    if (render_per_pass) {
                        world->set_pass_callback([](int pass) {
                            render_world();
                            SDL_Delay(50);  // 延迟50ms,让用户看清
                        });
                    } else {
                        world->set_pass_callback([](int pass) {
                            render_world();
                        });
                    }
                    std::cout << "Render per pass: " << (render_per_pass ? "ON (4x renders with delay)" : "OFF (normal render)") << std::endl;
                    break;

                case SDLK_F6:
                    show_thread_activity = !show_thread_activity;
                    std::cout << "Thread activity display: " << (show_thread_activity ? "ON (Yellow=Active)" : "OFF") << std::endl;
                    break;

                case SDLK_F7:
                    target_fps = std::max(10.0f, target_fps - 10.0f);
                    std::cout << "Target FPS: " << static_cast<int>(target_fps) << std::endl;
                    break;

                case SDLK_F8:
                    target_fps = std::min(120.0f, target_fps + 10.0f);
                    std::cout << "Target FPS: " << static_cast<int>(target_fps) << std::endl;
                    break;

                case SDLK_SPACE:
                    paused = !paused;
                    std::cout << (paused ? "PAUSED" : "RESUMED") << std::endl;
                    break;

                case SDLK_LEFTBRACKET:
                    brush_size = std::max(1, brush_size - 1);
                    std::cout << "Brush size: " << brush_size << std::endl;
                    break;

                case SDLK_RIGHTBRACKET:
                    brush_size = std::min(50, brush_size + 1);
                    std::cout << "Brush size: " << brush_size << std::endl;
                    break;

                // 数字键选择材料
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                case SDLK_5:
                case SDLK_6:
                case SDLK_7:
                case SDLK_8:
                case SDLK_9:
                case SDLK_0:
                    {
                        int index = (event.key.keysym.sym - SDLK_1 + 9) % 10;
                        if (index < material_list.size()) {
                            current_material_index = index;
                            current_material = material_list[index].id;
                            std::cout << "Selected: " << material_list[index].name << std::endl;
                        }
                    }
                    break;

                // 字母键选择材料
                case SDLK_q:
                case SDLK_w:
                case SDLK_e:
                case SDLK_r:
                case SDLK_t:
                case SDLK_y:
                case SDLK_u:
                    {
                        int index = 10 + (event.key.keysym.sym - SDLK_q);
                        if (index < material_list.size()) {
                            current_material_index = index;
                            current_material = material_list[index].id;
                            std::cout << "Selected: " << material_list[index].name << std::endl;
                        }
                    }
                    break;

                // 生成地形
                case SDLK_g:
                    std::cout << "Generating caves..." << std::endl;
                    world->generate_caves();
                    std::cout << "Caves generated!" << std::endl;
                    break;

                case SDLK_h:
                    std::cout << "Generating surface..." << std::endl;
                    world->generate_surface();
                    std::cout << "Surface generated!" << std::endl;
                    break;

                // 保存/加载
                case SDLK_s:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                        world->save("world_save.bin");
                        std::cout << "World saved!" << std::endl;
                    }
                    break;

                case SDLK_l:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                        world->load("world_save.bin");
                        std::cout << "World loaded!" << std::endl;
                    }
                    break;
            }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEMOTION) {
            if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);

                int world_x = static_cast<int>(mouse_x / camera.zoom + camera.x);
                int world_y = static_cast<int>(mouse_y / camera.zoom + camera.y);

                for (int dy = -brush_size; dy <= brush_size; ++dy) {
                    for (int dx = -brush_size; dx <= brush_size; ++dx) {
                        if (dx * dx + dy * dy <= brush_size * brush_size) {
                            int px = world_x + dx;
                            int py = world_y + dy;
                            if (world->is_valid(px, py)) {
                                world->set_material(px, py, current_material);
                            }
                        }
                    }
                }
            }
            else if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);

                int world_x = static_cast<int>(mouse_x / camera.zoom + camera.x);
                int world_y = static_cast<int>(mouse_y / camera.zoom + camera.y);

                // 右键擦除
                for (int dy = -brush_size; dy <= brush_size; ++dy) {
                    for (int dx = -brush_size; dx <= brush_size; ++dx) {
                        if (dx * dx + dy * dy <= brush_size * brush_size) {
                            int px = world_x + dx;
                            int py = world_y + dy;
                            if (world->is_valid(px, py)) {
                                Pixel empty;
                                world->set(px, py, empty);
                            }
                        }
                    }
                }
            }
        }
        else if (event.type == SDL_MOUSEWHEEL) {
            if (event.wheel.y > 0) {
                camera.zoom *= 1.1f;
            } else {
                camera.zoom /= 1.1f;
            }
            camera.zoom = std::max(0.1f, std::min(10.0f, camera.zoom));
        }
        else if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                camera.screen_width = event.window.data1;
                camera.screen_height = event.window.data2;
            }
        }
    }

    // 键盘移动相机
    const Uint8* keystate = SDL_GetKeyboardState(nullptr);
    float move_speed = 10.0f / camera.zoom;
    if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A]) camera.x -= move_speed;
    if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D]) camera.x += move_speed;
    if (keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W]) camera.y -= move_speed;
    if (keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S]) camera.y += move_speed;

    // 限制相机范围
    camera.x = std::max(0.0f, std::min(static_cast<float>(world->get_width() - camera.screen_width / camera.zoom), camera.x));
    camera.y = std::max(0.0f, std::min(static_cast<float>(world->get_height() - camera.screen_height / camera.zoom), camera.y));
}

// ==================== 主函数 ====================

int main(int argc, char* argv[]) {
    std::cout << "\n=== Noita-like Pixel Physics Demo ===\n" << std::endl;

    // 初始化材料注册表
    MaterialRegistry::instance().load_from_json("config/materials/basic_materials.json");

    // 创建世界
    WorldConfig config;
    config.width = 512;
    config.height = 512;
    world = new World(config);

    std::cout << "World created: " << world->get_width() << "x" << world->get_height() << std::endl;

    // 初始化材料列表
    init_material_list();

    // 初始化SDL
    if (!init_sdl()) {
        cleanup_sdl();
        delete world;
        return 1;
    }

    // 显示控制说明
    std::cout << "\n=== Controls ===" << std::endl;
    std::cout << "Mouse Left: Draw material" << std::endl;
    std::cout << "Mouse Right: Erase" << std::endl;
    std::cout << "Mouse Wheel: Zoom" << std::endl;
    std::cout << "Arrow Keys / WASD: Move camera" << std::endl;
    std::cout << "[ / ]: Decrease / Increase brush size" << std::endl;
    std::cout << "1-0, Q-U: Select material" << std::endl;
    std::cout << "Space: Pause / Resume" << std::endl;
    std::cout << "C: Clear world" << std::endl;
    std::cout << "G: Generate caves" << std::endl;
    std::cout << "H: Generate surface" << std::endl;
    std::cout << "Ctrl+S: Save world" << std::endl;
    std::cout << "Ctrl+L: Load world" << std::endl;
    std::cout << "F1: Toggle debug mode" << std::endl;
    std::cout << "F2: Toggle chunk borders" << std::endl;
    std::cout << "F3: Toggle FPS display" << std::endl;
    std::cout << "F4: Toggle chunk update display (Green=Updated, Red=Not Updated)" << std::endl;
    std::cout << "F5: Toggle render per pass (Show 4-pass update process)" << std::endl;
    std::cout << "F6: Toggle thread activity display (Yellow=Active)" << std::endl;
    std::cout << "F7: Decrease target FPS" << std::endl;
    std::cout << "F8: Increase target FPS" << std::endl;
    std::cout << "ESC: Exit\n" << std::endl;

    std::cout << "=== Materials ===" << std::endl;
    for (size_t i = 0; i < material_list.size(); ++i) {
        std::cout << material_list[i].key << ": " << material_list[i].name;
        if (i % 4 == 3) std::cout << std::endl;
        else std::cout << "\t";
    }
    std::cout << "\n" << std::endl;
    
    // 设置默认的渲染回调
    world->set_pass_callback([](int pass) {
        render_world();
    });

    // 主循环
    bool running = true;
    auto last_time = std::chrono::high_resolution_clock::now();
    auto last_frame_time = last_time;
    int frame_count = 0;
    const float frame_time = 1000.0f / target_fps;  // 每帧的时间(ms)

    while (running) {
        handle_input(running);
        
        // FPS控制
        auto frame_start_time = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(frame_start_time - last_frame_time).count() / 1000.0f;
        
        if (elapsed_ms < frame_time) {
            SDL_Delay(static_cast<Uint32>(frame_time - elapsed_ms));
        }
        last_frame_time = std::chrono::high_resolution_clock::now();

        // 更新世界
        if (!paused) {
            auto update_start = std::chrono::high_resolution_clock::now();
            world->update(1.0f / target_fps);
            auto update_end = std::chrono::high_resolution_clock::now();
            perf_stats.update_time_ms = std::chrono::duration_cast<std::chrono::microseconds>(update_end - update_start).count() / 1000.0f;
        } else {
            // 暂停时也要渲染
            render_world();
        }

        // 计算FPS
        frame_count++;
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - last_time);
        if (elapsed.count() >= 1) {
            perf_stats.fps = frame_count;
            frame_count = 0;
            last_time = current_time;

            if (show_fps) {
                std::cout << "FPS: " << static_cast<int>(perf_stats.fps)
                          << " | Update: " << std::fixed << std::setprecision(1) << perf_stats.update_time_ms << "ms"
                          << " | Render: " << perf_stats.render_time_ms << "ms" << std::endl;
            }
        }
    }

    // 清理
    cleanup_sdl();
    delete world;

    std::cout << "\n=== Demo Closed ===" << std::endl;
    return 0;
}
