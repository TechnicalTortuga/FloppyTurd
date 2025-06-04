#include "Window.h"

Window::Window(int width, int height)
{
	InitWindow(width, height, "Floppy Turd");
	SetExitKey(KEY_NULL);
}

Window::~Window()
{
	CloseWindow();
}
