#include "TextureAtlas.h"

void TextureAtlas::Initialize() {
    // Initialize atlas system
    atlasCount = 0;
    totalAtlasMemory = 0;
    
    // Set up category names for debugging
    categoryNames[AtlasCategory::UI_ELEMENTS] = "UI_ELEMENTS";
    categoryNames[AtlasCategory::PLAYER_SPRITES] = "PLAYER_SPRITES";
    categoryNames[AtlasCategory::ENEMY_SPRITES] = "ENEMY_SPRITES";
    categoryNames[AtlasCategory::ENVIRONMENT] = "ENVIRONMENT";
    categoryNames[AtlasCategory::PARTICLES] = "PARTICLES";
}

void TextureAtlas::Shutdown() {
    // Unload all atlas textures
    for (auto& [category, texture] : atlasTextures) {
        UnloadTexture(texture);
    }
    atlasTextures.clear();
    atlasEntries.clear();
    atlasCount = 0;
    totalAtlasMemory = 0;
}

bool TextureAtlas::BuildAtlas(AtlasCategory category, const std::vector<std::string>& texturePaths, int maxAtlasSize) {
    // For now, just return true - this is a stub implementation
    // TODO: Implement actual atlas building logic
    return true;
}

Rectangle TextureAtlas::GetTextureRegion(const std::string& texturePath) {
    auto it = atlasEntries.find(texturePath);
    if (it != atlasEntries.end()) {
        return it->second.sourceRect;
    }
    // Return empty rectangle if not found
    return {0, 0, 0, 0};
}

Texture2D TextureAtlas::GetAtlasTexture(AtlasCategory category) {
    auto it = atlasTextures.find(category);
    if (it != atlasTextures.end()) {
        return it->second;
    }
    // Return empty texture if not found
    return {0, 0, 0, 0, 0};
}

bool TextureAtlas::IsTextureAtlased(const std::string& texturePath) {
    return atlasEntries.find(texturePath) != atlasEntries.end();
}

int TextureAtlas::GetAtlasCount() const {
    return atlasCount;
}

size_t TextureAtlas::GetTotalAtlasMemory() const {
    return totalAtlasMemory;
}

void TextureAtlas::PrintAtlasStats() const {
    // TODO: Implement stats printing
}

void TextureAtlas::EnableMobileOptimizations(bool enable) {
    mobileOptimizations = enable;
}

void TextureAtlas::SetCompressionQuality(float quality) {
    compressionQuality = quality;
}

bool TextureAtlas::PackTextures(const std::vector<Image>& images, const std::vector<std::string>& paths,
                               int maxSize, Image& atlasImage, std::vector<AtlasEntry>& entries) {
    // TODO: Implement texture packing
    return false;
}

TextureAtlas::PackNode* TextureAtlas::FindNode(PackNode* root, int width, int height) {
    // TODO: Implement node finding
    return nullptr;
}

TextureAtlas::PackNode* TextureAtlas::SplitNode(PackNode* node, int width, int height) {
    // TODO: Implement node splitting
    return nullptr;
}