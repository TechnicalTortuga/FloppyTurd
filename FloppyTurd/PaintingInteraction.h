#pragma once
#include <raylib.h>
#include <algorithm>
#include <cmath>

struct PaintingInteraction {
    float scale=1;
    int pressedLevel=-1;
    bool Update(int level,bool hovered,bool pressed,bool down,bool released,bool unlocked,float dt) {
        if (pressed) pressedLevel=hovered ? level : -1;
        const bool activate=released && hovered && unlocked && pressedLevel==level;
        const bool held=down && pressedLevel==level && hovered;
        const float target=held ? 0.95f : hovered ? 1.04f : 1.0f;
        scale+=(target-scale)*(1-std::exp(-14*std::clamp(dt,0.0f,0.1f)));
        if (released || (!down && !pressed)) pressedLevel=-1;
        return activate;
    }
    Rectangle Bounds(Rectangle base) const {
        const float w=base.width*scale,h=base.height*scale;
        return {base.x+(base.width-w)/2,base.y+(base.height-h)/2,w,h};
    }
};
