
#include <iostream>
#include <raylib.h>
#include "Game.h"
#include "PlatformLayer.h"
#include "AIGUI.h"

int main()
{
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