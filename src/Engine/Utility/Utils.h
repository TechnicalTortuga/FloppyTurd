#pragma once

#include <string>
#include <cstdint>
#include <utility>
#include "../Core/GnosisTypes.h"

namespace GameCore {

// File and Asset utilities
// Returns absolute path to asset located under Assets_New or fallback Assets directory.
// This is platform-agnostic; platform-specific bundle logic can wrap this later.
std::string GetAssetFullPath(const std::string& relativePath);

// Simple cross-platform file existence check using std::filesystem.
bool FileExists(const std::string& relativePath);

// Timing utilities
// Get current timestamp in milliseconds using high-resolution clock
uint64_t GetCurrentTimestamp();

// UI Positioning utilities
// These functions provide consistent, predictable positioning calculations
// All positions are calculated as top-left coordinates for rendering

// Calculate top-left position to center an object at given screen position
Gnosis::GNVector2 CenterObjectAtPosition(float centerX, float centerY, float objectWidth, float objectHeight);

// Calculate top-left position to center an object horizontally on screen
Gnosis::GNVector2 CenterObjectHorizontally(float screenWidth, float centerY, float objectWidth, float objectHeight);

// Calculate scaled object dimensions
std::pair<float, float> GetScaledDimensions(float textureWidth, float textureHeight, float scale);

// Calculate position for UI element with percentage-based positioning
Gnosis::GNVector2 GetPercentagePosition(float screenWidth, float screenHeight, float xPercent, float yPercent, 
                                       float objectWidth, float objectHeight, bool centerObject = true);

} // namespace GameCore
