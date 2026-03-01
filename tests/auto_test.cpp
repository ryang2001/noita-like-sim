// Simple automated test program for Noita-like prototype
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

// Simple function to check if file exists
bool file_exists(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

// Check if string exists in file
bool contains_file_content(const char* filename, const std::string& search) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(search) != std::string::npos) {
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

int main() {
    printf("=== Noita-like Prototype - Automated Tests ===\n\n");

    int total_checks = 0;
    int passed_checks = 0;

    // ===== TEST 1: Source Files Existence =====
    printf("\n--- Test 1: Source Files Existence ---\n");

    const char* source_files[] = {
        "src/main.cpp",
        "src/Player.cpp",
        "src/World.cpp",
        "src/Enemy.cpp",
        "src/Renderer.cpp",
        "src/UI.cpp",
        nullptr
    };

    for (int i = 0; source_files[i] != nullptr; i++) {
        total_checks++;
        if (file_exists(source_files[i])) {
            printf("[PASS] %s exists\n", source_files[i]);
            passed_checks++;
        } else {
            printf("[FAIL] %s missing\n", source_files[i]);
        }
    }

    // ===== TEST 2: Include Files Existence =====
    printf("\n--- Test 2: Include Files Existence ---\n");

    const char* header_files[] = {
        "include/Player.hpp",
        "include/World.hpp",
        "include/Enemy.hpp",
        "include/Entity.hpp",
        "include/Pixel.hpp",
        "include/Input.hpp",
        "include/Renderer.hpp",
        "include/Wand.hpp",
        "include/Projectile.hpp",
        "include/Spell.hpp",
        "include/UI.hpp",
        nullptr
    };

    for (int i = 0; header_files[i] != nullptr; i++) {
        total_checks++;
        if (file_exists(header_files[i])) {
            printf("[PASS] %s exists\n", header_files[i]);
            passed_checks++;
        } else {
            printf("[FAIL] %s missing\n", header_files[i]);
        }
    }

    // ===== TEST 3: Key Methods Declared =====
    printf("\n--- Test 3: Key Methods in Headers ---\n");

    total_checks += 3;  // set_world, handle_input, interact_with_world

    if (file_exists("include/Player.hpp")) {
        // Check for key method declarations
        if (contains_file_content("include/Player.hpp", "set_world(")) {
            printf("[PASS] Player::set_world() declared\n");
            passed_checks++;
        } else {
            printf("[FAIL] Player::set_world() NOT found\n");
        }

        if (contains_file_content("include/Player.hpp", "handle_input(")) {
            printf("[PASS] Player::handle_input() declared\n");
            passed_checks++;
        } else {
            printf("[FAIL] Player::handle_input() NOT found\n");
        }

        if (contains_file_content("include/Player.hpp", "interact_with_world(")) {
            printf("[PASS] Player::interact_with_world() declared\n");
            passed_checks++;
        } else {
            printf("[FAIL] Player::interact_with_world() NOT found\n");
        }
    } else {
        printf("[SKIP] Player.hpp not found\n");
    }

    // ===== TEST 4: Render Buffer Method =====
    printf("\n--- Test 4: World Render Buffer Method ---\n");

    total_checks++;  // update_render_buffer check

    if (file_exists("include/World.hpp")) {
        if (contains_file_content("include/World.hpp", "update_render_buffer()")) {
            printf("[PASS] World::update_render_buffer() declared\n");
            passed_checks++;
        } else {
            printf("[FAIL] World::update_render_buffer() NOT found\n");
        }
    } else {
        printf("[SKIP] World.hpp not found\n");
    }

    // ===== TEST 5: New Entity Types =====
    printf("\n--- Test 5: New Entity Types ---\n");

    if (file_exists("include/Enemy.hpp")) {
        const char* entity_types[] = {"Flower", "Grass", "Human", "Tree", "Bridge", "Vehicle"};
        int found_count = 0;

        for (int i = 0; i < 5; i++) {
            total_checks++;
            if (contains_file_content("include/Enemy.hpp", entity_types[i])) {
                printf("[PASS] EnemyType::%s declared\n", entity_types[i]);
                passed_checks++;
                found_count++;
            } else {
                printf("[FAIL] EnemyType::%s NOT found\n", entity_types[i]);
            }
        }

        total_checks++;  // Check if we found at least one
        if (found_count > 0) {
            printf("[INFO] Found %d/5 new entity types\n", found_count);
            passed_checks++;
        } else {
            printf("[FAIL] No new entity types found\n");
        }
    } else {
        printf("[SKIP] Enemy.hpp not found\n");
    }

    // ===== TEST 6: Compile Main Program =====
    printf("\n--- Test 6: Main Program Compilation ---\n");

    total_checks++;

    // Try to compile main program
    printf("Attempting to compile main program...\n");

    int compile_result = system("g++ -o sim.exe.tmp src/*.cpp -Iinclude -I\"C:/mingw64/include/SDL2\" -L\"C:/mingw64/lib\" -m64 -lmingw32 -lSDL2main -lSDL2 -std=c++20 -DSDL_MAIN_HANDLED=1 2>nul");

    if (compile_result == 0) {
        printf("[PASS] Main program compiles successfully\n");
        passed_checks++;

        // Clean up temp file if exists
        system("del sim.exe.tmp 2>nul");
    } else {
        printf("[FAIL] Main program compilation failed (error code: %d)\n", compile_result);
    }

    // ===== SUMMARY =====
    printf("\n=== SUMMARY ===\n");
    printf("Total checks: %d\n", total_checks);
    printf("Passed: %d\n", passed_checks);
    printf("Failed: %d\n", total_checks - passed_checks);

    if (passed_checks == total_checks) {
        printf("\n[SUCCESS] All tests passed!\n");
        return 0;
    } else {
        printf("\n[WARNING] Some tests failed. Review results above.\n");
        return 1;
    }
}
