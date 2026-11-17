#include "EnemyEntry.h"
#include <stdexcept>
#include <iostream>
static void Require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
int main() {
    using namespace EnemyEntry;
    Require(OffscreenX(400,340)==420,"the old sewer spawn must be moved completely beyond the desktop edge");
    Require(OffscreenX(200)==220 && OffscreenX(200,350)==350,"portrait spawns honor the arena while retaining future offscreen positions");
    Require(!OffscreenGap(400,100,300,32).Valid(),"birds cannot spawn in a visible gap");
    const auto gap=OffscreenGap(400,390,480,32);
    Require(gap.Valid() && gap.min>400 && gap.max+32<=480,"a straddling gap only offers fully offscreen bird positions");
    for (int fps : {20,30,60,120}) for (float arena : {200.0f,400.0f})
        for (float speed : {80.0f,100.0f,140.0f}) for (float phase : {0.0f,1.5f,4.5f}) {
            Vector2 normal{OffscreenX(arena),80},inverted=normal;
            float distance=0,invertedDistance=0;
            for (int frame=0;frame<fps*3;++frame) {
                const float previousX=normal.x;
                normal=AdvanceRoll(normal,80,distance,speed,phase,false,1.0f/fps);
                inverted=AdvanceRoll(inverted,80,invertedDistance,speed,phase,true,1.0f/fps);
                Require(normal.x<previousX,"rolls move on the first update and never idle at the screen edge");
                Require(std::abs(normal.y-80)<=50.001f,"the original wave amplitude remains bounded");
                Require(std::abs(normal.y+inverted.y-160)<0.001f,"inverted rolls retain mirrored wave attacks");
            }
            Require(std::abs(normal.x-(OffscreenX(arena)-speed*3))<0.03f,"entry travel is consistent across difficulty speeds and frame rates");
            const auto paused=AdvanceRoll(normal,80,distance,speed,phase,false,0);
            Require(paused.x==normal.x && paused.y==normal.y,"pause cannot move an enemy");
        }
    float distance=0;
    const auto first=AdvanceRoll({420,110},110,distance,100,1.5f,true,0.000001f);
    Require(std::abs(first.y-110)<0.001f,"wave onset does not teleport away from the supplied spawn height");
    std::cout<<"Offscreen enemy entry, continuous wave motion and frame-rate checks passed\n";
}
