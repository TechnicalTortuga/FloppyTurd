#include <cstdlib>
#include <ctime>
#include <raylib.h>
#include "Game.h"
#include "PlatformLayer.h"
#include "EmbedInput.h"
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
void floppy_safe_area(float top, float right, float bottom, float left) {
    if (activeGame) activeGame->SetSafeArea(top, right, bottom, left);
}
extern "C"
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_orientation(int waiting) {
    if (activeGame) activeGame->SetOrientationWaiting(waiting!=0);
}
extern "C"
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_touch(int action, int down) {
    if (activeGame) activeGame->TouchAction(action, down != 0);
}
extern "C" {
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_key(int code, int down, int repeat) { EmbedInput::Key(code, down != 0, repeat != 0); }
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_pointer(int phase, float x, float y) { if (activeGame) activeGame->PointerInput(phase, x, y); }
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_reset_input() { EmbedInput::Reset(); if (activeGame && activeGame->playing) activeGame->playing->CancelTouchInput(); }
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_set_presentation(int inlineMode) { if (activeGame) activeGame->SetInlinePresentation(inlineMode != 0); }
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void floppy_activate_audio(int activated) { if (activeGame) activeGame->SetAudioActivated(activated != 0); }
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
