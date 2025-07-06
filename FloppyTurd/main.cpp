#include <iostream>

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

// Global game instance for iOS to access
Game* g_gameInstance = nullptr;

Game* GetGameInstance()
{
	return g_gameInstance;
}

void SetGameInstance(Game* instance)
{
	g_gameInstance = instance;
}

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
	// Suppress unused parameter warnings
	(void)argc;
	(void)argv;
	
	// Setup working directory first
	SetupWorkingDirectory();
	
	// Seed the random number generator
	srand(static_cast<unsigned int>(time(NULL)));

	// Initialize platform layer with a placeholder value for nativeView
	PlatformLayer::GetInstance().Initialize(nullptr);

#if defined(PLATFORM_IOS)
	// On iOS, we only create the game instance
	// The actual game loop is driven by the iOS display system
	g_gameInstance = new Game();
	return 0; // iOS will keep the app running via UIApplicationMain
#else
	// On desktop, we create the game and run the traditional loop
	Game* game = new Game();
	game->RunGameDesktop();
	
	delete game;
	
	// Clean up platform layer
	PlatformLayer::GetInstance().Shutdown();
	
	return 0;
#endif
}

// Regular main function for non-iOS platforms
#if !defined(PLATFORM_IOS)
int main()
{
	return game_main(0, nullptr);
}
#endif

// iOS entry point is handled by main_ios.mm