#include <iostream>
#include <raylib.h>
#include "Game.h"
#define AIGUI_IMPLEMENTATION
#include "AIGUI.h"

AIGUI_Context g_AIGUI = {};

int main()
{
	// Seed the random number generator
	srand(time(NULL));

	Game* game = new Game();
}