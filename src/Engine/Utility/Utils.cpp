#include "Utils.h"
#include <filesystem>
#include <chrono>

namespace GameCore {

static const std::filesystem::path kNewAssetRoot = std::filesystem::path("src/Assets_New");
static const std::filesystem::path kLegacyAssetRoot = std::filesystem::path("src/Assets");

std::string GetAssetFullPath(const std::string& relativePath) {
    std::filesystem::path candidate = kNewAssetRoot / relativePath;
    if (std::filesystem::exists(candidate)) {
        return candidate.string();
    }
    candidate = kLegacyAssetRoot / relativePath;
    return candidate.string();  // regardless of exist; caller may check
}

bool FileExists(const std::string& relativePath) {
    std::filesystem::path candidateNew = kNewAssetRoot / relativePath;
    if (std::filesystem::exists(candidateNew)) {
        return true;
    }
    std::filesystem::path candidateLegacy = kLegacyAssetRoot / relativePath;
    return std::filesystem::exists(candidateLegacy);
}

uint64_t GetCurrentTimestamp() {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace GameCore
