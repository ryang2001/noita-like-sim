// Simplified compile-time verification test
#include <cstdio>

// Simpler macros that don't require external symbols
#define PASS_STR "[PASS]"
#define FAIL_STR "[FAIL]"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED   "\033[31m"

int main() {
    printf("\n" COLOR_CYAN "=== Noita-like Prototype - Compile-Time Verification ===" COLOR_RESET "\n");

    int passed = 0;
    int failed = 0;

    // Check 1: Source files exist
    printf("\n--- Checking Source Files ---\n");

    #ifdef PLAYER_CPP
        printf(PASS_STR " Player.cpp: exists\n");
        passed++;
    #else
        printf(FAIL_STR " Player.cpp: missing\n");
        failed++;
    #endif

    #ifdef WORLD_CPP
        printf(PASS_STR " World.cpp: exists\n");
        passed++;
    #else
        printf(FAIL_STR " World.cpp: missing\n");
        failed++;
    #endif

    // Check 2: Header files exist
    printf("\n--- Checking Header Files ---\n");

    #ifdef PLAYER_HPP
        printf(PASS_STR " Player.hpp: exists\n");
        passed++;
    #else
        printf(FAIL_STR " Player.hpp: missing\n");
        failed++;
    #endif

    #ifdef WORLD_HPP
        printf(PASS_STR " World.hpp: exists\n");
        passed++;
    #else
        printf(FAIL_STR " World.hpp: missing\n");
        failed++;
    #endif

    #ifdef ENEMY_HPP
        printf(PASS_STR " Enemy.hpp: exists\n");
        passed++;
    #else
        printf(FAIL_STR " Enemy.hpp: missing\n");
        failed++;
    #endif

    // Check 3: Method declarations in headers
    printf("\n--- Checking Method Declarations ---\n");

    #ifdef PLAYER_HPP
        // Check for set_world method
        #ifdef SET_WORLD
            printf(PASS_STR " Player::set_world() declared\n");
            passed++;
        #else
            printf(FAIL_STR " Player::set_world() NOT declared\n");
            failed++;
        #endif

        // Check for handle_input method
        #ifdef HANDLE_INPUT
            printf(PASS_STR " Player::handle_input() declared\n");
            passed++;
        #else
            printf(FAIL_STR " Player::handle_input() NOT declared\n");
            failed++;
        #endif

        // Check for interact_with_world method
        #ifdef INTERACT_WORLD
            printf(PASS_STR " Player::interact_with_world() declared\n");
            passed++;
        #else
            printf(FAIL_STR " Player::interact_with_world() NOT declared\n");
            failed++;
        #endif

    // Check 4: Render buffer method
    printf("\n--- Checking Render Buffer Method ---\n");

    #ifdef WORLD_HPP
        #ifdef UPDATE_RENDER_BUFFER
            printf(PASS_STR " World::update_render_buffer() declared\n");
            passed++;
        #else
            printf(FAIL_STR " World::update_render_buffer() NOT declared\n");
            failed++;
        #endif

    // Check 5: New entity types
    printf("\n--- Checking New Entity Types ---\n");

    #ifdef ENEMY_HPP
        #ifdef FLOWER_TYPE
            printf(PASS_STR " EnemyType::Flower declared\n");
            passed++;
        #else
            printf(FAIL_STR " EnemyType::Flower NOT declared\n");
            failed++;
        #endif

        #ifdef GRASS_TYPE
            printf(PASS_STR " EnemyType::Grass declared\n");
            passed++;
        #else
            printf(FAIL_STR " EnemyType::Grass NOT declared\n");
            failed++;
        #endif

        #ifdef HUMAN_TYPE
            printf(PASS_STR " EnemyType::Human declared\n");
            passed++;
        #else
            printf(FAIL_STR " EnemyType::Human NOT declared\n");
            failed++;
        #endif

        #ifdef TREE_TYPE
            printf(PASS_STR " EnemyType::Tree declared\n");
            passed++;
        #else
            printf(FAIL_STR " EnemyType::Tree NOT declared\n");
            failed++;
        #endif

        #ifdef BRIDGE_TYPE
            printf(PASS_STR " EnemyType::Bridge declared\n");
            passed++;
        #else
            printf(FAIL_STR " EnemyType::Bridge NOT declared\n");
            failed++;
        #endif

        #ifdef VEHICLE_TYPE
            printf(PASS_STR " EnemyType::Vehicle declared\n");
            passed++;
        #else
            printf(FAIL_STR " EnemyType::Vehicle NOT declared\n");
            failed++;
        #endif

    // Summary
    printf("\n" COLOR_CYAN "========== SUMMARY ==========" COLOR_RESET "\n");
    printf("Total: %d | " COLOR_GREEN "PASS" COLOR_RESET ": %d\n", passed + failed, passed, failed);
    printf("\n");

    if (failed == 0) {
        printf(COLOR_GREEN "✓ ALL CHECKS PASSED!" COLOR_RESET "\n");
        return 0;
    } else {
        printf(COLOR_RED "✗ Some checks failed!" COLOR_RESET "\n");
        return 1;
    }
}
