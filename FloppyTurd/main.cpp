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

int main()
{
	// Setup working directory first
	SetupWorkingDirectory();
	
	// Seed the random number generator
	srand(time(NULL));

	// Initialize platform layer
	PlatformLayer::GetInstance().Initialize();

	Game* game = new Game(); // Constructor automatically calls RunGame()
	
	delete game; // Clean up the game object
	
	// Clean up platform layer
	PlatformLayer::GetInstance().Shutdown();
	
	return 0;
}