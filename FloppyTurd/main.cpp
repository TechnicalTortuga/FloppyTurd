#include <cstdlib>
#include <ctime>
#include <raylib.h>
#include "Game.h"
#include "PlatformLayer.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
static Game* activeGame = nullptr;
extern "C" {
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_set_paused(int paused) {
    if (activeGame) activeGame->SetHostPaused(paused != 0);
}
}
extern "C"
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_resize(int width, int height) {
    if (activeGame && width > 0 && height > 0) SetWindowSize(width, height);
}
extern "C"
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_touch(int action, int down) {
    if (activeGame) activeGame->TouchAction(action, down != 0);
}
int main() {
#ifndef __EMSCRIPTEN__
    // Installed builds resolve resources relative to the executable on every desktop OS.
    ChangeDirectory(GetApplicationDirectory());
#endif
    srand(static_cast<unsigned>(time(nullptr)));
    activeGame = new Game();
    activeGame->RunGame();
    delete activeGame;
    activeGame = nullptr;
    PlatformLayer::GetInstance().Shutdown();
    return 0;
}
