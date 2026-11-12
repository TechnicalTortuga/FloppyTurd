#include "GameStats.h"
#include "PlatformLayer.h"
#include <fstream>
#include <vector>
#include <cstdint>
#include <iterator>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
namespace {
std::string SavePath() { return PlatformLayer::GetInstance().GetSavePath("floppy_turd_stats.bin"); }
void AppendInt(std::vector<unsigned char>& out, int value) {
    auto word = static_cast<std::uint32_t>(value);
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<unsigned char>(word >> (8 * i)));
}
std::uint32_t ReadInt(const std::vector<unsigned char>& data, size_t& pos) {
    std::uint32_t word = 0;
    for (int i = 0; i < 4; ++i) word |= std::uint32_t(data[pos++]) << (8 * i);
    return word;
}
}
void GameStats::Load() {
    // Decode into a temporary value: truncated/corrupt files cannot partially replace progress.
    *this = GameStats{};
    levelUnlocked[0] = true;
    std::ifstream file(SavePath(), std::ios::binary);
    if (!file) return;
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), {});
    if (bytes.size() != 79 && bytes.size() != 80) return;
    size_t pos = 0;
    auto format = ReadInt(bytes, pos);
    // v1 was a Windows/macOS little-endian struct with one padding byte.
    if (!((format == 1 && bytes.size() == 80) || (format == 2 && bytes.size() == 79))) return;
    GameStats loaded;
    int* counters[] = {&loaded.totalPipes, &loaded.totalCoins, &loaded.totalEnemiesKilled,
        &loaded.totalJumps, &loaded.totalFlops, &loaded.totalLevelTries};
    for (auto counter : counters) {
        auto value = ReadInt(bytes, pos);
        if (value > 2147483647u) return;
        *counter = static_cast<int>(value);
    }
    for (auto& score : loaded.levelHighScores) {
        auto value = ReadInt(bytes, pos);
        if (value > 2147483647u) return;
        score = static_cast<int>(value);
    }
    for (auto flags : {loaded.hatUnlocked, loaded.skillUnlocked, loaded.levelUnlocked}) {
        int count = flags == loaded.hatUnlocked ? 15 : 6;
        for (int i = 0; i < count; ++i) {
            if (bytes[pos] > 1) return;
            flags[i] = bytes[pos++] != 0;
        }
    }
    loaded.levelUnlocked[0] = true;
    *this = loaded;
}
void GameStats::Save() const {
    std::vector<unsigned char> bytes;
    AppendInt(bytes, 2);
    for (int value : {totalPipes, totalCoins, totalEnemiesKilled, totalJumps, totalFlops, totalLevelTries}) AppendInt(bytes, value);
    for (int value : levelHighScores) AppendInt(bytes, value);
    for (bool flag : hatUnlocked) bytes.push_back(flag ? 1 : 0);
    for (bool flag : skillUnlocked) bytes.push_back(flag ? 1 : 0);
    for (int i = 0; i < 6; ++i) bytes.push_back(i == 0 || levelUnlocked[i] ? 1 : 0);
    const auto path = SavePath();
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) { TraceLog(LOG_WARNING, "Could not save progress: %s", path.c_str()); return; }
    file.close();
#ifdef __EMSCRIPTEN__
    EM_ASM({
        try {
            const bytes = FS.readFile(UTF8ToString($0));
            localStorage.setItem('floppy-turd-save-v2', btoa(String.fromCharCode(...bytes)));
        } catch (e) { console.warn('Progress could not be persisted', e); }
    }, path.c_str());
#endif
}
