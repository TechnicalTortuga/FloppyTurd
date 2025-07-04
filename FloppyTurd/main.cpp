#include <iostream>

// Only include SDL on iOS if not using Metal renderer
#if defined(PLATFORM_IOS) && !defined(USE_METAL_RENDERER)
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#endif

#include "RaylibCompat.h"
#include "Game.h"
#include "PlatformLayer.h"
#include "AIGUI.h"

// Uncomment this line to enable letterbox debugging
// #define DEBUG_LETTERBOX

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <libgen.h>
#include <unistd.h>
#endif

void SetupWorkingDirectory() {
#ifdef __APPLE__
    // Get the path to the executable
    char pathBuf[PATH_MAX];
    uint32_t bufSize = PATH_MAX;
    if (_NSGetExecutablePath(pathBuf, &bufSize) == 0) {
        // Get the directory containing the executable
        char* execDir = dirname(pathBuf);
        
        // Change to that directory
        if (chdir(execDir) == 0) {
            printf("Changed working directory to: %s\n", execDir);
        } else {
            printf("Failed to change working directory to: %s\n", execDir);
        }
    } else {
        printf("Failed to get executable path\n");
    }
#endif
}

int game_main(int argc, char *argv[])
{
	// Setup working directory first
	SetupWorkingDirectory();
	
	// Seed the random number generator
	srand(static_cast<unsigned int>(time(NULL)));

	// Initialize platform layer
	PlatformLayer::GetInstance().Initialize();

	Game* game = new Game(); // Constructor automatically calls RunGame()
	
	delete game; // Clean up the game object
	
	// Clean up platform layer
	PlatformLayer::GetInstance().Shutdown();
	
	return 0;
}

// Regular main function for non-iOS platforms
#if !defined(PLATFORM_IOS)
int main()
{
	return game_main(0, nullptr);
}
#endif

// iOS entry point
#if defined(PLATFORM_IOS)
extern "C" int main(int argc, char *argv[]) {
#if !defined(USE_METAL_RENDERER)
    // Initialize SDL2 if not using Metal
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
#endif

    // Call our game logic
    int result = game_main(argc, argv);

#if !defined(USE_METAL_RENDERER)
    // Clean up SDL if used
    SDL_Quit();
#endif
    return result;
}
#endif