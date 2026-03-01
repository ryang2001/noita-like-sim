// Header verification test - reads actual header files
#include <cstdio>
#include <fstream>
#include <string>

bool file_exists(const char* filename) {
    std::ifstream file(filename);
    return file.good();
}

bool contains_string(std::ifstream& file, const std::string& str) {
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(str) != std::string::npos) {
            return true;
        }
    }
    return false;
}

int main() {
    int passed = 0;
    int failed = 0;

    printf("=== Noita-like Prototype - Header Verification ===\n\n");

    // Test 1: Check header files exist
    printf("\n--- Checking Header Files ---\n");

    if (file_exists("../include/Player.hpp")) {
        printf("[PASS] Player.hpp exists\n");
        passed++;
    } else {
        printf("[FAIL] Player.hpp not found\n");
        failed++;
    }

    if (file_exists("../include/Entity.hpp")) {
        printf("[PASS] Entity.hpp exists\n");
        passed++;
    } else {
        printf("[FAIL] Entity.hpp not found\n");
        failed++;
    }

    if (file_exists("../include/Pixel.hpp")) {
        printf("[PASS] Pixel.hpp exists\n");
        passed++;
    } else {
        printf("[FAIL] Pixel.hpp not found\n");
        failed++;
    }

    if (file_exists("../include/World.hpp")) {
        printf("[PASS] World.hpp exists\n");
        passed++;
    } else {
        printf("[FAIL] World.hpp not found\n");
        failed++;
    }

    if (file_exists("../include/Enemy.hpp")) {
        printf("[PASS] Enemy.hpp exists\n");
        passed++;
    } else {
        printf("[FAIL] Enemy.hpp not found\n");
        failed++;
    }

    // Test 2: Check key methods exist in headers
    printf("\n--- Checking Method Declarations ---\n");

    // Check Player.hpp for key methods
    std::ifstream player_hpp("../include/Player.hpp");
    bool has_set_world = contains_string(player_hpp, "set_world(");
    bool has_handle_input = contains_string(player_hpp, "handle_input(");
    bool has_interact = contains_string(player_hpp, "interact_with_world(");

    if (has_set_world && has_handle_input && has_interact) {
        printf("[PASS] Player methods declared (set_world, handle_input, interact_with_world)\n");
        passed += 3;
    } else {
        printf("[FAIL] Player methods missing\n");
        failed += 3;
    }

    // Test 3: Check World.hpp for render buffer method
    std::ifstream world_hpp("../include/World.hpp");
    bool has_update_render = contains_string(world_hpp, "update_render_buffer()");

    if (has_update_render) {
        printf("[PASS] World::update_render_buffer() declared\n");
        passed++;
    } else {
        printf("[FAIL] World::update_render_buffer() not found\n");
        failed++;
    }

    // Test 4: Check Enemy.hpp for new entity types
    std::ifstream enemy_hpp("../include/Enemy.hpp");
    bool has_flower = contains_string(enemy_hpp, "Flower");
    bool has_grass = contains_string(enemy_hpp, "Grass");
    bool has_human = contains_string(enemy_hpp, "Human");
    bool has_tree = contains_string(enemy_hpp, "Tree");
    bool has_bridge = contains_string(enemy_hpp, "Bridge");
    bool has_vehicle = contains_string(enemy_hpp, "Vehicle");

    int entity_count = 0;
    if (has_flower) { printf("[PASS] Flower type declared\n"); entity_count++; }
    else { printf("[FAIL] Flower type missing\n"); failed++; }

    if (has_grass) { printf("[PASS] Grass type declared\n"); entity_count++; }
    else { printf("[FAIL] Grass type missing\n"); failed++; }

    if (has_human) { printf("[PASS] Human type declared\n"); entity_count++; }
    else { printf("[FAIL] Human type missing\n"); failed++; }

    if (has_tree) { printf("[PASS] Tree type declared\n"); entity_count++; }
    else { printf("[FAIL] Tree type missing\n"); failed++; }

    if (has_bridge) { printf("[PASS] Bridge type declared\n"); entity_count++; }
    else { printf("[FAIL] Bridge type missing\n"); failed++; }

    if (has_vehicle) { printf("[PASS] Vehicle type declared\n"); entity_count++; }
    else { printf("[FAIL] Vehicle type missing\n"); failed++; }

    if (entity_count == 6) {
        printf("[PASS] All 6 new entity types declared\n");
        passed += 6;
    }

    // Summary
    printf("\n=== SUMMARY ===\n");
    printf("Total checks: %d\n", passed + failed);
    printf("Passed: %d\n", passed);
    if (failed > 0) printf("Failed: %d\n", failed);

    if (failed == 0) {
        printf("\n[SUCCESS] All header checks passed!\n");
    }

    return (failed > 0) ? 1 : 0;
}
