#include "RenderLayout.h"
#include "PipeLayout.h"
#include <stdexcept>
#include <iostream>
static void Require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
int main() {
    GameSettings::SelectArena(true);
    Require(GameSettings::GameWidth==225 && GameSettings::GameHeight==400,"normal touch runs use a portrait arena");
    for (float offset : {-30.0f,-20.0f,0.0f,60.0f}) {
        const auto pipe=PipeLayout::Calculate(225,offset,192,192);
        Require(pipe.topHit.y<=0 && pipe.bottomHit.y+pipe.bottomHit.height>=400,"portrait pipe shafts cannot be bypassed above or below");
        Require(pipe.bottomHit.y-(pipe.topHit.y+pipe.topHit.height)>80,"portrait pipe clearance retains the legacy gap");
    }
    RenderLayout::Configure(393,696,true);
    RenderLayout::safeTop=64; RenderLayout::safeBottom=86;
    const auto portrait=RenderLayout::WorldClip();
    Require(portrait.height*RenderLayout::cssScale/2>695,"portrait gameplay fills the available height");
    const float width=GameSettings::GameWidth,height=GameSettings::GameHeight;
    RenderLayout::Configure(800,450,false);
    const auto inlineClip=RenderLayout::WorldClip();
    Require(GameSettings::GameWidth==width && GameSettings::GameHeight==height,"presentation changes cannot remap the live arena");
    Require(inlineClip.height==450 && inlineClip.width<450,"inline output fits the same portrait run at800x450");
    GameSettings::SelectArena(false);
    RenderLayout::Configure(844,280,true);
    RenderLayout::safeTop=64; RenderLayout::safeBottom=90;
    const auto boss=RenderLayout::WorldClip();
    Require(boss.height*RenderLayout::cssScale/2>279,"short Safari landscape uses the entire height");
    const auto camera=RenderLayout::WorldCamera();
    RenderLayout::safeTop=100; RenderLayout::safeBottom=120;
    Require(RenderLayout::WorldCamera().zoom==camera.zoom,"control dock and safe-area changes cannot shrink gameplay");
    RenderLayout::Configure(800,450,false);
    Require(RenderLayout::WorldCamera().zoom==2 && RenderLayout::WorldCamera().offset.x==0,"desktop/boss legacy geometry remains400x225");
    std::cout<<"Portrait arena, inline retention and Safari landscape camera checks passed\n";
}
