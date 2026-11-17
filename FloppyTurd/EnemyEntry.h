#pragma once
#include <raylib.h>
#include <algorithm>
#include <cmath>

namespace EnemyEntry {
inline constexpr float Margin=20, RollAmplitude=50;
inline float OffscreenX(float arenaWidth,float requested=0) { return std::max(arenaWidth+Margin,requested); }
struct SpawnRange {
    float min,max;
    bool Valid() const { return min<=max; }
};
inline SpawnRange OffscreenGap(float arenaWidth,float left,float right,float spriteWidth) {
    return {std::ceil(OffscreenX(arenaWidth,left)),std::floor(right-spriteWidth)};
}
inline Vector2 AdvanceRoll(Vector2 pos,float baseY,float& distance,float speed,float phase,bool inverted,float dt) {
    const float step=std::max(0.0f,dt)*speed;
    pos.x-=step;
    distance+=step;
    // Enter the wave continuously from the requested spawn height. Random phase
    // and inversion cannot teleport the roll on its first update.
    const float t=std::clamp(distance/32.0f,0.0f,1.0f);
    const float amplitude=RollAmplitude*t*t*(3-2*t);
    pos.y=baseY+amplitude*std::sin(phase-distance*0.015f)*(inverted ? -1 : 1);
    return pos;
}
}
