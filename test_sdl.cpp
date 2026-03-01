#include "SDL.h"
#include <iostream>

int main() {
    std::cout << "Starting SDL test..." << std::endl;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    std::cout << "SDL initialized successfully!" << std::endl;
    
    SDL_Window* window = SDL_CreateWindow("Test",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          640, 480,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    std::cout << "Window created successfully!" << std::endl;
    
    SDL_Delay(2000);
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Test completed!" << std::endl;
    return 0;
}
