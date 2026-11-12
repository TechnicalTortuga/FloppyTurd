#pragma once
#include <array>
#include <raylib.h>
namespace EmbedInput {
struct State {
    std::array<bool,512> down{}, pressed{}, released{};
    bool hasPointer=false, pointerDown=false, pointerPressed=false, pointerReleased=false;
    Vector2 position{}, delta{};
};
inline State state;
inline void Reset() { state = State{}; }
inline void EndFrame() {
    state.pressed.fill(false); state.released.fill(false);
    state.pointerPressed=state.pointerReleased=false; state.delta={0,0};
}
inline void Key(int code, bool down, bool repeat) {
    if (code<0 || code>=512) return;
    if (down && !state.down[code] && !repeat) state.pressed[code]=true;
    if (!down && state.down[code]) state.released[code]=true;
    state.down[code]=down;
}
inline void Pointer(int phase, Vector2 position) {
    if (state.hasPointer) { state.delta.x += position.x-state.position.x; state.delta.y += position.y-state.position.y; }
    state.position=position; state.hasPointer=true;
    if (phase==1) { state.pointerPressed=true; state.pointerDown=true; }
    if (phase==2) { state.pointerReleased=state.pointerDown; state.pointerDown=false; }
    if (phase==3) { state.pointerDown=state.pointerPressed=state.pointerReleased=false; }
}
inline bool KeyPressed(int key) { return (key>=0 && key<512 && state.pressed[key]) || IsKeyPressed(key); }
inline bool KeyDown(int key) { return (key>=0 && key<512 && state.down[key]) || IsKeyDown(key); }
inline bool MousePressed(int button) { return (button==0 && state.pointerPressed) || IsMouseButtonPressed(button); }
inline bool MouseDown(int button) { return (button==0 && state.pointerDown) || IsMouseButtonDown(button); }
inline bool MouseReleased(int button) { return (button==0 && state.pointerReleased) || IsMouseButtonReleased(button); }
inline Vector2 MousePosition() { return state.hasPointer ? state.position : GetMousePosition(); }
inline Vector2 MouseDelta() { return state.hasPointer ? state.delta : GetMouseDelta(); }
}
