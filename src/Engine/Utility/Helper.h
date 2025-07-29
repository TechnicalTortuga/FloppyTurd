#pragma once

#include <string>

namespace GameCore {

// Returns absolute path to asset located under Assets_New or fallback Assets directory.
// This is platform-agnostic; platform-specific bundle logic can wrap this later.
std::string GetAssetFullPath(const std::string& relativePath);

// Simple cross-platform file existence check using std::filesystem.
bool FileExists(const std::string& relativePath);

} // namespace GameCore
