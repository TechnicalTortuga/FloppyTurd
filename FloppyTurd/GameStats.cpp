#include "GameStats.h"
#include "PlatformLayer.h"
#include <fstream>
#include <vector>
#include <cstdint>
#include <iterator>
#include <filesystem>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
namespace {
std::string SavePath(const char* name) { return PlatformLayer::GetInstance().GetSavePath(name); }
void AppendInt(std::vector<unsigned char>& out, int value) {
    auto word=static_cast<std::uint32_t>(value);
    for (int i=0;i<4;++i) out.push_back(static_cast<unsigned char>(word>>(8*i)));
}
std::uint32_t ReadInt(const std::vector<unsigned char>& data, size_t& pos) {
    std::uint32_t word=0;
    for (int i=0;i<4;++i) word|=std::uint32_t(data[pos++])<<(8*i);
    return word;
}
bool Decode(const std::string& path,GameStats& result) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if (!file) return false;
    const auto length=file.tellg();
    if (length!=79 && length!=80 && length!=81) return false;
    file.seekg(0);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),{});
    if (bytes.size()!=static_cast<size_t>(length)) return false;
    size_t pos=0;
    const auto format=ReadInt(bytes,pos);
    if (!((format==1 && bytes.size()==80) || (format==2 && bytes.size()==79)
        || (format==3 && bytes.size()==81))) return false;
    GameStats loaded;
    int* counters[]={&loaded.totalPipes,&loaded.totalCoins,&loaded.totalEnemiesKilled,
        &loaded.totalJumps,&loaded.totalFlops,&loaded.totalLevelTries};
    for (auto counter:counters) {
        const auto value=ReadInt(bytes,pos);
        if (value>2147483647u) return false;
        *counter=static_cast<int>(value);
    }
    for (auto& score:loaded.levelHighScores) {
        const auto value=ReadInt(bytes,pos);
        if (value>2147483647u) return false;
        score=static_cast<int>(value);
    }
    for (auto flags:{loaded.hatUnlocked,loaded.skillUnlocked,loaded.levelUnlocked}) {
        const int count=flags==loaded.hatUnlocked ? 15 : 6;
        for (int i=0;i<count;++i) {
            if (bytes[pos]>1) return false;
            flags[i]=bytes[pos++]!=0;
        }
    }
    if (format==3) {
        if (bytes[pos]>2 || bytes[pos+1]>2) return false;
        loaded.homingRank=bytes[pos++]; loaded.splitRank=bytes[pos++];
    }
    loaded.levelUnlocked[0]=true;
    result=loaded;
    return true;
}
bool WriteAtomic(const std::string& path,const std::vector<unsigned char>& bytes) {
    const auto temporary=path+".tmp";
    std::ofstream file(temporary,std::ios::binary|std::ios::trunc);
    file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    file.close();
    if (!file) return false;
    std::error_code error;
    std::filesystem::rename(temporary,path,error);
    if (error) { std::filesystem::remove(temporary,error); return false; }
    return true;
}
}
void GameStats::Load() {
    *this=GameStats{}; levelUnlocked[0]=true;
    // A separate canonical record keeps old clients from erasing new ranks.
    if (Decode(SavePath("floppy_turd_stats_v3.bin"),*this)) return;
    Decode(SavePath("floppy_turd_stats.bin"),*this);
}
void GameStats::Save() const {
    std::vector<unsigned char> bytes;
    AppendInt(bytes,3);
    for (int value:{totalPipes,totalCoins,totalEnemiesKilled,totalJumps,totalFlops,totalLevelTries}) AppendInt(bytes,value);
    for (int value:levelHighScores) AppendInt(bytes,value);
    for (bool flag:hatUnlocked) bytes.push_back(flag ? 1 : 0);
    for (bool flag:skillUnlocked) bytes.push_back(flag ? 1 : 0);
    for (int i=0;i<6;++i) bytes.push_back(i==0 || levelUnlocked[i] ? 1 : 0);
    auto legacy=bytes; legacy[0]=2;
    bytes.push_back(homingRank); bytes.push_back(splitRank);
    const auto path=SavePath("floppy_turd_stats_v3.bin");
    const auto legacyPath=SavePath("floppy_turd_stats.bin");
    if (!WriteAtomic(path,bytes)) { TraceLog(LOG_WARNING,"Could not save progress: %s",path.c_str()); return; }
    if (!WriteAtomic(legacyPath,legacy)) { TraceLog(LOG_WARNING,"Could not export legacy progress"); return; }
#ifdef __EMSCRIPTEN__
    EM_ASM({
        try {
            const encode=path=>btoa(String.fromCharCode(...FS.readFile(path)));
            localStorage.setItem('floppy-turd-save-v3',encode(UTF8ToString($0)));
            localStorage.setItem('floppy-turd-save-v2',encode(UTF8ToString($1)));
        } catch (e) { console.warn('Progress could not be persisted',e); }
    },path.c_str(),legacyPath.c_str());
#endif
}
