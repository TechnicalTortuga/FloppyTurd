#include "GameStats.h"
#include "PlatformLayer.h"
#include "Viewport.h"
#include "CoinBalance.h"
#include "EmbedInput.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <iostream>
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    EmbedInput::Key(KEY_SPACE,true,false); EmbedInput::Key(KEY_SPACE,false,false);
    Check(EmbedInput::KeyPressed(KEY_SPACE) && !EmbedInput::KeyDown(KEY_SPACE), "rapid embedded tap survives a frame boundary");
    EmbedInput::EndFrame(); Check(!EmbedInput::state.pressed[KEY_SPACE], "key edge consumed once");
    EmbedInput::Pointer(1,{20,30}); EmbedInput::Pointer(2,{20,30});
    Check(EmbedInput::state.pointerPressed && EmbedInput::state.pointerReleased, "rapid pointer click survives a frame boundary");
    EmbedInput::Reset(); EmbedInput::Pointer(1,{20,30}); EmbedInput::Pointer(3,{20,30});
    Check(!EmbedInput::state.pointerReleased, "pointer cancellation does not click a menu item");
    EmbedInput::Reset();
    int total = 105, session = 5;
    Check(CoinBalance::Spend(total, session, 1, true) && total == 104 && session == 4, "shot spends a coin once");
    Check(CoinBalance::Spend(total, session, 102) && total == 2 && session == 0, "purchase spends session subset then bank");
    Check(!CoinBalance::Spend(total, session, 1, true) && total == 2, "shoot cannot spend banked coins");
    Check(!CoinBalance::Spend(total, session, 3) && total == 2 && session == 0, "insufficient funds preserve balance");
    Check(!CoinBalance::Spend(total, session, -5) && total == 2, "negative cost cannot mint coins");
    const auto path = PlatformLayer::GetInstance().GetSavePath("floppy_turd_stats.bin");
    const auto v3Path = PlatformLayer::GetInstance().GetSavePath("floppy_turd_stats_v3.bin");
    std::filesystem::remove(path);
    std::filesystem::remove(v3Path);
    GameStats s; s.Load();
    Check(s.levelUnlocked[0] && s.totalCoins == 0, "fresh progress");
    s.totalCoins = 123456; s.totalPipes = 42; s.hatUnlocked[14] = true;
    s.skillUnlocked[5] = true; s.levelHighScores[5] = 77; s.levelUnlocked[5] = true; s.Save();
    Check(std::filesystem::file_size(path) == 79, "legacy shadow portable size");
    GameStats roundTrip; roundTrip.Load();
    Check(roundTrip.totalCoins == 123456 && roundTrip.totalPipes == 42 && roundTrip.hatUnlocked[14]
        && roundTrip.skillUnlocked[5] && roundTrip.levelHighScores[5] == 77 && roundTrip.levelUnlocked[5], "round trip progression");
    // Known v1 layout: LE version, twelve LE counters, twenty-seven bools, padding.
    std::filesystem::remove(v3Path);
    unsigned char legacy[80]{}; legacy[0] = 1; legacy[8] = 99; legacy[52 + 14] = 1; legacy[73 + 5] = 1;
    { std::ofstream f(path, std::ios::binary); f.write(reinterpret_cast<char*>(legacy), 80); }
    GameStats migrated; migrated.Load();
    Check(migrated.version == 3 && migrated.totalCoins == 99 && migrated.hatUnlocked[14] && migrated.levelUnlocked[5], "legacy v1 migration");
    legacy[52] = 9;
    { std::ofstream f(path, std::ios::binary); f.write(reinterpret_cast<char*>(legacy), 80); }
    migrated.Load(); Check(migrated.totalCoins == 0 && migrated.levelUnlocked[0], "reject malformed bools atomically");
    { std::ofstream f(path, std::ios::binary); f.put(2); }
    migrated.totalCoins = 123; migrated.Load(); Check(migrated.totalCoins == 0, "reject truncated save");
    for (auto dimensions : {std::pair{1920,1080}, std::pair{1000,1000}, std::pair{2560,1080}, std::pair{200,100}}) {
        auto v = FloppyViewport::Fit(dimensions.first, dimensions.second);
        Check(v.width <= dimensions.first + 0.001f && v.height <= dimensions.second + 0.001f && v.scale > 0, "viewport fits");
        Check(std::abs((dimensions.first / 2.0f - v.x) / v.scale - 200) < 0.01f, "pointer center mapping");
    }
    auto mobileFit = FloppyViewport::Fit(844,390,false);
    Check(std::abs(mobileFit.height-390)<0.001f && mobileFit.width>690, "mobile uses available space without integer-scale gaps");
    auto centered = RenderLayout::ToUi({200,112.5f}, true);
    Check(std::abs(centered.x-160)<0.001f && std::abs(centered.y-90)<0.001f, "smaller overlay pointer mapping");
    auto crisp = FloppyViewport::Fit(1000,700);
    Check(crisp.scale == 2 && crisp.width == 800 && crisp.x == 100, "integer pixel scaling");
    for (auto dimensions : {std::pair{393,696},std::pair{844,390},std::pair{320,480},std::pair{568,320},std::pair{768,1024}}) {
        RenderLayout::safeTop=64; RenderLayout::safeRight=16; RenderLayout::safeBottom=84; RenderLayout::safeLeft=16;
        RenderLayout::Configure(dimensions.first,dimensions.second,true);
        const auto mobile=FloppyViewport::Fit(dimensions.first,dimensions.second,false,RenderLayout::surfaceWidth,RenderLayout::surfaceHeight);
        Check(std::abs(mobile.width-dimensions.first)<0.01f && std::abs(mobile.height-dimensions.second)<0.01f,"adaptive UI fills portrait and landscape");
        const auto safe=RenderLayout::SafeRect();
        const auto camera=RenderLayout::WorldCamera();
        Check(camera.offset.x>=-0.01f && camera.offset.y>=-0.01f,"world fits the full canvas independently of UI safe bounds");
        Check(camera.offset.x+400*camera.zoom<=RenderLayout::surfaceWidth*2+0.01f && camera.offset.y+225*camera.zoom<=RenderLayout::surfaceHeight*2+0.01f,"world preserves the complete gameplay arena");
        const Vector2 center={(dimensions.first/2.0f-mobile.x)/mobile.scale,(dimensions.second/2.0f-mobile.y)/mobile.scale};
        const auto ui=RenderLayout::ToUi(center,true);
        Check(std::abs(ui.x-RenderLayout::surfaceWidth/2)<0.01f && std::abs(ui.y-RenderLayout::surfaceHeight/2)<0.01f,"adaptive pointer and UI camera agree");
        Check(RenderLayout::TouchHeight()*RenderLayout::cssScale>=43.99f,"mobile menu touch targets remain at least 44 CSS pixels");
    }
    RenderLayout::Configure(800,450,false);
    Check(RenderLayout::surfaceWidth==400 && RenderLayout::surfaceHeight==225 && RenderLayout::WorldCamera().zoom==2,"desktop and inline CRT keep legacy geometry");
    std::cout << "Progress migration, corruption, persistence and viewport checks passed\n";
}
