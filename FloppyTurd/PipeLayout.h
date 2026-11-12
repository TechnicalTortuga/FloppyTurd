#pragma once
#include "GameSettings.h"
#include <raylib.h>
#include <algorithm>
namespace PipeLayout {
struct Pair { float topY,bottomY; Rectangle topHit,bottomHit; };
inline Pair Calculate(float x,float offset,int topHeight,int bottomHeight) {
    const float shift=GameSettings::PortraitArena ? (GameSettings::GameHeight-225)/2 : 0;
    Pair p{(float)(-2*topHeight/3)+offset+shift,(float)(bottomHeight/3+bottomHeight/7)+offset+shift,
        {x+18,-2*topHeight/3.0f+offset+shift,30,6*topHeight/7.0f},
        {x+18,bottomHeight/3.0f+2*bottomHeight/7.0f+offset+shift,30,(float)bottomHeight}};
    if (GameSettings::PortraitArena) {
        const float end=p.topHit.y+p.topHit.height;
        p.topHit.y=std::min(0.0f,p.topHit.y); p.topHit.height=end-p.topHit.y;
        p.bottomHit.height=std::max(p.bottomHit.height,GameSettings::GameHeight-p.bottomHit.y);
    }
    return p;
}
inline void Draw(Texture2D top,Texture2D bottom,float x,float offset) {
    const auto p=Calculate(x,offset,top.height,bottom.height);
    if (GameSettings::PortraitArena && p.topY>0) DrawTexturePro(top,{0,0,(float)top.width,64},{x,0,(float)top.width,p.topY},{0,0},0,WHITE);
    DrawTexturePro(top,{0,0,(float)top.width,(float)top.height},{x,p.topY,(float)top.width,(float)top.height},{0,0},0,WHITE);
    DrawTexturePro(bottom,{0,0,(float)bottom.width,(float)bottom.height},{x,p.bottomY,(float)bottom.width,(float)bottom.height},{0,0},0,WHITE);
    const float end=p.bottomY+bottom.height;
    if (GameSettings::PortraitArena && end<GameSettings::GameHeight) DrawTexturePro(bottom,{0,(float)bottom.height-64,(float)bottom.width,64},{x,end,(float)bottom.width,GameSettings::GameHeight-end},{0,0},0,WHITE);
}
}
