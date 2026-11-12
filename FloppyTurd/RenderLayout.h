#pragma once
#include <raylib.h>
namespace RenderLayout {
inline constexpr float WorldWidth = 400, WorldHeight = 225;
inline constexpr float UiWidth = 320, UiHeight = 180;
inline constexpr float UiX = (WorldWidth - UiWidth) / 2, UiY = (WorldHeight - UiHeight) / 2;
inline constexpr int Density = 2;
inline Camera2D WorldCamera() { return {{0,0},{0,0},0,Density}; }
inline float UiScale(bool overlay) { return overlay ? 0.9f : 1.0f; }
inline Vector2 UiOrigin(bool overlay) {
    const float scale = UiScale(overlay);
    return {(WorldWidth - UiWidth * scale) / 2, (WorldHeight - UiHeight * scale) / 2};
}
inline Camera2D UiCamera(bool overlay = false) {
    const auto origin = UiOrigin(overlay);
    return {{origin.x * Density, origin.y * Density},{0,0},0,Density * UiScale(overlay)};
}
inline Vector2 ToUi(Vector2 world, bool overlay) {
    const auto origin = UiOrigin(overlay);
    const float scale = UiScale(overlay);
    return {(world.x - origin.x) / scale, (world.y - origin.y) / scale};
}
struct EndCameraOnExit { ~EndCameraOnExit() { EndMode2D(); } };
}
