#pragma once
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include "GameSettings.h"
namespace RenderLayout {
inline constexpr float WorldWidth = 400, WorldHeight = 225;
inline constexpr float UiWidth = 320, UiHeight = 180;
inline constexpr float UiX = (WorldWidth - UiWidth) / 2, UiY = (WorldHeight - UiHeight) / 2;
inline constexpr int Density = 2;
// CSS pixels, render-buffer pixels, UI units and gameplay units remain separate.
// An arena is chosen when a run starts, never by browser chrome or presentation changes.
inline bool mobile = false;
inline float surfaceWidth = WorldWidth, surfaceHeight = WorldHeight;
inline float cssScale = 1;
inline float safeTop = 0, safeRight = 0, safeBottom = 0, safeLeft = 0;
inline float TouchHeight() { return std::max(44.0f, 44.0f / cssScale); }
inline void Configure(int width, int height, bool adaptive) {
    mobile = adaptive;
    cssScale = adaptive ? std::max(0.1f, std::min(width / 320.0f, height / 360.0f)) : 1;
    surfaceWidth = adaptive ? width / cssScale : WorldWidth;
    surfaceHeight = adaptive ? height / cssScale : WorldHeight;
}
inline Rectangle SafeRect() {
    if (!mobile) return {0, 0, UiWidth, UiHeight};
    const float left = std::clamp(safeLeft / cssScale, 0.0f, surfaceWidth / 3);
    const float top = std::clamp(safeTop / cssScale, 0.0f, surfaceHeight / 3);
    const float right = std::clamp(safeRight / cssScale, 0.0f, surfaceWidth / 3);
    const float bottom = std::clamp(safeBottom / cssScale, 0.0f, surfaceHeight / 3);
    return {left, top, surfaceWidth - left - right, surfaceHeight - top - bottom};
}
struct HudLayout {
    Rectangle score, coins;
    float heartX, heartY, heartSize, heartStep;
    Rectangle Heart(int index) const { return {heartX,heartY+index*heartStep,heartSize,heartSize}; }
};
inline HudLayout MobileHud(Rectangle safe,int heartCount,float coinTextWidth) {
    const float margin=8, gap=4;
    const float heartY=safe.y+14;
    const int count=std::max(1,heartCount);
    const float heartSize=std::min(20.0f,(safe.y+safe.height-margin-heartY-(count-1)*gap)/count);
    const float coinWidth=std::min(26+coinTextWidth,safe.width*0.45f);
    return {{safe.x+(safe.width-100)/2,safe.y+margin,100,40},
        {safe.x+safe.width-margin-coinWidth,safe.y+safe.height-margin-24,coinWidth,24},
        safe.x+margin,heartY,heartSize,heartSize+gap};
}
inline Camera2D WorldCamera() {
    // Controls and the HUD overlay gameplay. Reserving their height here caused
    // Safari's short landscape viewport to collapse into a postage stamp.
    const float scale = std::min(surfaceWidth / GameSettings::GameWidth, surfaceHeight / GameSettings::GameHeight);
    return {{(surfaceWidth - GameSettings::GameWidth * scale) / 2 * Density,
        (surfaceHeight - GameSettings::GameHeight * scale) / 2 * Density}, {0,0}, 0, Density * scale};
}
inline Rectangle WorldClip() {
    const auto camera=WorldCamera();
    return {camera.offset.x,camera.offset.y,GameSettings::GameWidth*camera.zoom,GameSettings::GameHeight*camera.zoom};
}
inline float UiScale(bool overlay) { return overlay ? 0.9f : 1.0f; }
inline Vector2 UiOrigin(bool overlay) {
    const float scale = UiScale(overlay);
    return {(WorldWidth - UiWidth * scale) / 2, (WorldHeight - UiHeight * scale) / 2};
}
inline Camera2D UiCamera(bool overlay = false) {
    if (mobile) return {{0,0},{0,0},0,Density};
    const auto origin = UiOrigin(overlay);
    return {{origin.x * Density, origin.y * Density},{0,0},0,Density * UiScale(overlay)};
}
inline Vector2 ToUi(Vector2 world, bool overlay) {
    if (mobile) return world;
    const auto origin = UiOrigin(overlay);
    const float scale = UiScale(overlay);
    return {(world.x - origin.x) / scale, (world.y - origin.y) / scale};
}
struct EndCameraOnExit { ~EndCameraOnExit() { EndMode2D(); } };
}
