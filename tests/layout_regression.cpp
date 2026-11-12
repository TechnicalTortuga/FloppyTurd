#include "RenderLayout.h"
#include "PipeLayout.h"
#include <stdexcept>
#include <iostream>
static void Require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
int main() {
    GameSettings::SelectArena(true);
    Require(GameSettings::GameWidth==200 && GameSettings::GameHeight==356,"normal touch runs use the closer portrait arena");
    for (float offset : {-30.0f,-20.0f,0.0f,60.0f}) {
        const auto pipe=PipeLayout::Calculate(225,offset,192,192);
        Require(pipe.topHit.y<=0 && pipe.bottomHit.y+pipe.bottomHit.height>=GameSettings::GameHeight,"portrait pipe shafts cannot be bypassed above or below");
        Require(pipe.bottomHit.y-(pipe.topHit.y+pipe.topHit.height)>80,"portrait pipe clearance retains the legacy gap");
    }
    RenderLayout::Configure(393,696,true);
    RenderLayout::safeTop=64; RenderLayout::safeBottom=86;
    const auto portrait=RenderLayout::WorldClip();
    Require(portrait.height*RenderLayout::cssScale/2>695,"portrait gameplay fills the available height");
    Require(portrait.height/GameSettings::GameHeight>696*2/400.0f/RenderLayout::cssScale*1.12f,"player and toilets render at least12percent larger than the previous portrait framing");
    const float width=GameSettings::GameWidth,height=GameSettings::GameHeight;
    RenderLayout::Configure(800,450,false);
    const auto inlineClip=RenderLayout::WorldClip();
    Require(GameSettings::GameWidth==width && GameSettings::GameHeight==height,"presentation changes cannot remap the live arena");
    Require(std::abs(inlineClip.height-450)<0.01f && inlineClip.width<450,"inline output fits the same portrait run at800x450");
    GameSettings::SelectArena(false);
    RenderLayout::Configure(844,280,true);
    RenderLayout::safeTop=64; RenderLayout::safeBottom=90;
    const auto boss=RenderLayout::WorldClip();
    Require(boss.height*RenderLayout::cssScale/2>279,"short Safari landscape uses the entire height");
    const auto camera=RenderLayout::WorldCamera();
    RenderLayout::safeTop=100; RenderLayout::safeBottom=120;
    Require(RenderLayout::WorldCamera().zoom==camera.zoom,"control dock and safe-area changes cannot shrink gameplay");
    for (const auto area : {Rectangle{0,64,320,440},Rectangle{60,80,965,160},Rectangle{0,0,320,180}}) {
        const auto hud=RenderLayout::MobileHud(area,9,75);
        Require(std::abs(hud.score.x+hud.score.width/2-(area.x+area.width/2))<0.01f,"pipe count stays centered at the top");
        Require(hud.Heart(1).x==hud.Heart(0).x && hud.Heart(1).y>hud.Heart(0).y,"hearts descend along the left edge");
        const auto last=hud.Heart(8);
        Require(last.y+last.height<=area.y+area.height && last.width>0,"nine boss hearts fit even in short landscape and inline views");
        Require(hud.coins.x>area.x+area.width/2 && hud.coins.y>area.y+area.height/2,"coin counter anchors to the lower right safe area");
        Require(hud.coins.x+hud.coins.width<=area.x+area.width && hud.coins.y+hud.coins.height<=area.y+area.height,"coin counter stays clear of unsafe edges and the shoot pad");
    }
    RenderLayout::Configure(800,450,false);
    Require(RenderLayout::WorldCamera().zoom==2 && RenderLayout::WorldCamera().offset.x==0,"desktop/boss legacy geometry remains400x225");
    std::cout<<"Portrait arena, inline retention and Safari landscape camera checks passed\n";
}
