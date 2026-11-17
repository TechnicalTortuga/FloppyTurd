#pragma once

namespace PipeScoring {
inline float CenterX(float spriteX,float frameWidth) { return spriteX+frameWidth/2; }
// Obstacles scroll left. Once their center reaches the player's center, latch
// the award so an overshooting frame still counts and later frames cannot repeat it.
inline bool TryAward(float playerCenterX,float obstacleCenterX,bool& awarded) {
    if (awarded || playerCenterX<obstacleCenterX) return false;
    awarded=true;
    return true;
}
}
