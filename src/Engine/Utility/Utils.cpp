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

// UI Positioning utilities
Gnosis::GNVector2 CenterObjectAtPosition(float centerX, float centerY, float objectWidth, float objectHeight) {
    return Gnosis::GNVector2(centerX - (objectWidth / 2.0f), centerY - (objectHeight / 2.0f));
}

Gnosis::GNVector2 CenterObjectHorizontally(float screenWidth, float centerY, float objectWidth, float objectHeight) {
    float centerX = screenWidth / 2.0f;
    return CenterObjectAtPosition(centerX, centerY, objectWidth, objectHeight);
}

std::pair<float, float> GetScaledDimensions(float textureWidth, float textureHeight, float scale) {
    return std::make_pair(textureWidth * scale, textureHeight * scale);
}

Gnosis::GNVector2 GetPercentagePosition(float screenWidth, float screenHeight, float xPercent, float yPercent, 
                                       float objectWidth, float objectHeight, bool centerObject) {
    float targetX = screenWidth * xPercent;
    float targetY = screenHeight * yPercent;
    
    if (centerObject) {
        return CenterObjectAtPosition(targetX, targetY, objectWidth, objectHeight);
    } else {
        return Gnosis::GNVector2(targetX, targetY);
    }
}

} // namespace GameCore
