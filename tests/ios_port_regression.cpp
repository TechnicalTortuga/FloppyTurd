#include "ProjectileSkills.h"
#include "CoinBalance.h"
#include "GameStats.h"
#include "PlatformLayer.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
namespace {
void Check(bool value,const char* message) {
    if (!value) { std::cerr<<message<<'\n'; std::exit(1); }
}
std::vector<unsigned char> Read(const std::string& path) {
    std::ifstream file(path,std::ios::binary);
    return {(std::istreambuf_iterator<char>(file)),{}};
}
void Write(const std::string& path,const std::vector<unsigned char>& data) {
    std::ofstream file(path,std::ios::binary|std::ios::trunc);
    file.write(reinterpret_cast<const char*>(data.data()),data.size());
}
}
int main() {
    Check(std::getenv("FLOPPY_TURD_SAVE_DIR")!=nullptr,"Run through CTest with an isolated save directory");
    using namespace ProjectileSkills;
    int coins=1200,session=25;
    std::uint8_t rank=0;
    auto spend=[&](int cost){return CoinBalance::Spend(coins,session,cost);};
    Check(!Upgrade(Kind::Homing,rank,false,spend) && coins==1200 && rank==0,"shooting prerequisite cannot charge currency");
    Check(Upgrade(Kind::Homing,rank,true,spend) && coins==900 && session==0 && rank==1,"first homing rank spends300 once");
    Check(Upgrade(Kind::Homing,rank,true,spend) && coins==200 && rank==2,"second homing rank costs700");
    Check(!Upgrade(Kind::Homing,rank,true,spend) && coins==200,"max rank never charges again");
    rank=0;
    Check(Upgrade(Kind::Split,rank,true,spend) && coins==0 && rank==1,"first split rank costs200");
    Check(!Upgrade(Kind::Split,rank,true,spend) && rank==1 && coins==0,"failed upgrade preserves rank and balance");
    Check(ShotCount(0)==1 && ShotCount(1)==2 && ShotCount(2)==3,"split counts match iOS ranks");
    Check(ShotOffset(0)==0 && ShotOffset(1)==15 && ShotOffset(2)==-15,"parallel split offsets use normalized iOS spacing");
    Check(!CanAcquire({0,0},{1,0},{-10,0},2),"homing never acquires behind the shot");
    Check(!CanAcquire({0,0},{1,0},{100,50},1) && CanAcquire({0,0},{1,0},{100,50},2),"rank acquisition cones20/40degrees");
    const auto weak=Steer({1,0},{0,0},{60,20},1,1.0f/60);
    const auto strong=Steer({1,0},{0,0},{60,20},2,1.0f/60);
    Check(weak.y>0 && strong.y>weak.y && std::abs(std::hypot(strong.x,strong.y)-1)<0.0001f,"stronger homing turns faster without changing speed");
    const auto zero=Steer({1,0},{0,0},{60,20},2,0);
    Check(zero.x==1 && zero.y==0,"paused guidance cannot turn");

    const auto legacyPath=PlatformLayer::GetInstance().GetSavePath("floppy_turd_stats.bin");
    const auto canonicalPath=PlatformLayer::GetInstance().GetSavePath("floppy_turd_stats_v3.bin");
    std::filesystem::remove(legacyPath);std::filesystem::remove(canonicalPath);
    GameStats source;source.totalCoins=321;source.homingRank=2;source.splitRank=1;source.skillUnlocked[0]=true;source.hatUnlocked[14]=true;source.Save();
    Check(std::filesystem::file_size(canonicalPath)==81 && std::filesystem::file_size(legacyPath)==79,"v3 canonical and legacy shadow have independent formats");
    const auto shadow=Read(legacyPath);
    Check(shadow[0]==2,"legacy rollback receives a readable v2 record");
    GameStats restored;restored.Load();
    Check(restored.totalCoins==321 && restored.homingRank==2 && restored.splitRank==1 && restored.hatUnlocked[14],"new ranks and previous unlocks round trip");
    source.totalCoins=555;source.Save();restored.Load();
    Check(restored.totalCoins==555,"atomic save replaces an existing file on this platform");
    auto invalid=Read(canonicalPath);invalid[79]=3;Write(canonicalPath,invalid);restored.Load();
    Check(restored.totalCoins==555 && restored.homingRank==0 && restored.skillUnlocked[0],"invalid new ranks recover legacy progress atomically");
    std::filesystem::remove(canonicalPath);restored.Load();
    Check(restored.version==3 && restored.homingRank==0 && restored.totalCoins==555,"v2 migrates without unlocking free projectile skills");
    auto old=shadow;old[0]=1;old.push_back(0);Write(legacyPath,old);restored.Load();
    Check(restored.version==3 && restored.totalCoins==321 && restored.hatUnlocked[14],"original v1 still migrates");
    std::cout<<"iOS skill economy, guidance and compatible-save checks passed\n";
}
