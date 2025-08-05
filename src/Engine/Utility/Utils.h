#pragma once

#include <string>
#include <cstdint>

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

} // namespace GameCore
