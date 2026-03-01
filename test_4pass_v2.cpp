#include <iostream>
#include <vector>

void test_4pass(int width, int height) {
    std::cout << "Testing 4-pass pattern for " << width << "x" << height << " grid:" << std::endl;
    
    for (int pass = 0; pass < 4; ++pass) {
        std::cout << "Pass " << pass << ":" << std::endl;
        std::vector<std::vector<char>> grid(height, std::vector<char>(width, 'o'));
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // 当前的算法
                if (((y % 2) * 2 + (x % 2)) == pass) {
                    grid[y][x] = 'x';
                }
            }
        }
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                std::cout << grid[y][x];
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
}

int main() {
    test_4pass(12, 6);
    return 0;
}
