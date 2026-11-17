#include "PipeScoring.h"
#include <stdexcept>
#include <iostream>
static void Require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
int main() {
    const float player=PipeScoring::CenterX(10,64);
    bool scored=false;
    Require(!PipeScoring::TryAward(player,PipeScoring::CenterX(10.1f,64),scored),"no point before the sprite centers align");
    Require(PipeScoring::TryAward(player,PipeScoring::CenterX(10,64),scored),"alignment awards the point immediately");
    Require(!PipeScoring::TryAward(player,PipeScoring::CenterX(-50,64),scored),"one obstacle cannot award twice");
    scored=false;
    Require(!PipeScoring::TryAward(player,45,scored),"obstacle ahead stays unscored");
    Require(PipeScoring::TryAward(player,39,scored),"a six-pixel frame crossing cannot skip the pop");
    for (float dt : {1.0f/120,1.0f/60,1.0f/30,0.05f}) {
        bool awarded[3]={false,false,false};
        float x[3]={70,120,170};
        int count=0;
        for (int frame=0;frame<600;++frame) {
            for (int i=0;i<3;++i) {
                x[i]-=120*dt;
                if (PipeScoring::TryAward(player,PipeScoring::CenterX(x[i],64),awarded[i])) ++count;
            }
        }
        Require(count==3,"each toilet scores once at fast speed across frame rates");
    }
    bool pending[2]={false,false};int count=0;
    for (int frame=0;frame<3;++frame) for (auto& flag:pending) {
        if (PipeScoring::TryAward(100,90,flag)) { ++count; break; }
    }
    Require(count==2,"simultaneous crossings remain eligible for the next score update");
    std::cout<<"Sprite-center scoring, overshoot and one-point-per-obstacle checks passed\n";
}
