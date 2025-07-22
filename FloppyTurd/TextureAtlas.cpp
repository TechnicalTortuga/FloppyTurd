#include "TextureAtlas.h"

void TextureAtlas::Initialize() {
    // Initialize atlas system
    atlasCount = 0;
    totalAtlasMemory = 0;
    
    // Set up category names for debugging
    categoryNames[AtlasCategory::UI_ELEMENTS] = "UI_ELEMENTS";
    // categoryNames[AtlasCategory::PLAYER_SPRITES] = "PLAYER_SPRITES"; // Removed - using GAME_OBJECTS instead
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
    if (texturePaths.empty()) {
        GameLog::Log("TextureAtlas: No textures provided for category %d", static_cast<int>(category));
        return false;
    }
    
    // 1. Load and analyze all textures
    std::vector<TextureInfo> textures;
    int totalArea = 0;
    
    for (const auto& path : texturePaths) {
        Image image = LoadImage(path.c_str());
        if (image.data == nullptr) {
            GameLog::Log("TextureAtlas: Failed to load texture: %s", path.c_str());
            continue;
        }
        
        TextureInfo info;
        info.path = path;
        info.image = image;
        info.width = image.width;
        info.height = image.height;
        info.originalWidth = image.width;
        info.originalHeight = image.height;
        info.category = category;
        info.rotated = false;
        info.priority = info.width * info.height; // Larger textures have higher priority
        
        totalArea += info.width * info.height;
        textures.push_back(info);
    }
    
    if (textures.empty()) {
        GameLog::Log("TextureAtlas: No valid textures loaded for category %d", static_cast<int>(category));
        return false;
    }
    
    // 2. Estimate required atlas size with 20% padding for efficiency
    int estimatedSize = static_cast<int>(sqrt(totalArea * 1.2f));
    int atlasSize = NextPowerOfTwo(std::max(estimatedSize, 512));
    atlasSize = std::min(atlasSize, maxAtlasSize);
    
    GameLog::Log("TextureAtlas: Building atlas for category %d, estimated size: %dx%d", 
                static_cast<int>(category), atlasSize, atlasSize);
    
    // 3. Sort textures by area (largest first) for better packing efficiency
    std::sort(textures.begin(), textures.end(), 
              [](const TextureInfo& a, const TextureInfo& b) {
                  return a.priority > b.priority;
              });
    
    // 4. Attempt packing with current size
    std::vector<AtlasEntry> entries;
    bool packingSuccessful = PackTextures(textures, atlasSize, entries);
    
    // 5. If packing failed, try larger size (up to maxAtlasSize)
    while (!packingSuccessful && atlasSize < maxAtlasSize) {
        atlasSize *= 2;
        if (atlasSize > maxAtlasSize) {
            atlasSize = maxAtlasSize;
        }
        
        GameLog::Log("TextureAtlas: Retrying with larger size: %dx%d", atlasSize, atlasSize);
        entries.clear();
        packingSuccessful = PackTextures(textures, atlasSize, entries);
    }
    
    if (!packingSuccessful) {
        GameLog::Log("TextureAtlas: Failed to pack textures for category %d", static_cast<int>(category));
        // Cleanup loaded images
        for (auto& texture : textures) {
            UnloadImage(texture.image);
        }
        return false;
    }
    
    // 6. Create final atlas texture
    bool atlasCreated = CreateAtlasTexture(category, textures, entries, atlasSize);
    
    // 7. Cleanup loaded images
    for (auto& texture : textures) {
        UnloadImage(texture.image);
    }
    
    if (atlasCreated) {
        GameLog::Log("TextureAtlas: Successfully created atlas for category %d (%dx%d, %zu textures)", 
                    static_cast<int>(category), atlasSize, atlasSize, entries.size());
        return true;
    } else {
        GameLog::Log("TextureAtlas: Failed to create atlas texture for category %d", static_cast<int>(category));
        return false;
    }
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
    GameLog::Log("=== TEXTURE ATLAS STATISTICS ===");
    GameLog::Log("Total Atlases: %d", atlasCount);
    GameLog::Log("Total Memory: %.2f MB", totalAtlasMemory / (1024.0f * 1024.0f));
    
    // Category names for better logging
    static const std::unordered_map<AtlasCategory, std::string> categoryNames = {
        {AtlasCategory::UI, "UI"},
        {AtlasCategory::GAME_OBJECTS, "GameObjects"},
        {AtlasCategory::BACKGROUNDS, "Backgrounds"},
        {AtlasCategory::EFFECTS, "Effects"}
    };
    
    for (const auto& [category, texture] : atlasTextures) {
        std::string categoryName = "Unknown";
        auto it = categoryNames.find(category);
        if (it != categoryNames.end()) {
            categoryName = it->second;
        }
        
        int textureCount = 0;
        int usedPixels = 0;
        
        // Count textures and calculate used space in this category
        for (const auto& [path, entry] : atlasEntries) {
            if (entry.category == category) {
                textureCount++;
                usedPixels += static_cast<int>(entry.sourceRect.width * entry.sourceRect.height);
            }
        }
        
        int totalPixels = texture.width * texture.height;
        float efficiency = totalPixels > 0 ? (static_cast<float>(usedPixels) / totalPixels * 100.0f) : 0.0f;
        float memoryMB = (totalPixels * 4) / (1024.0f * 1024.0f); // RGBA = 4 bytes per pixel
        
        GameLog::Log("Atlas [%s]: %dx%d, %d textures, %.1f%% efficiency, %.2f MB", 
                    categoryName.c_str(), texture.width, texture.height, 
                    textureCount, efficiency, memoryMB);
    }
    
    // Calculate overall efficiency
    int totalUsedPixels = 0;
    int totalAtlasPixels = 0;
    
    for (const auto& [category, texture] : atlasTextures) {
        totalAtlasPixels += texture.width * texture.height;
    }
    
    for (const auto& [path, entry] : atlasEntries) {
        totalUsedPixels += static_cast<int>(entry.sourceRect.width * entry.sourceRect.height);
    }
    
    float overallEfficiency = totalAtlasPixels > 0 ? (static_cast<float>(totalUsedPixels) / totalAtlasPixels * 100.0f) : 0.0f;
    
    GameLog::Log("Overall Efficiency: %.1f%% (%d/%d pixels used)", 
                overallEfficiency, totalUsedPixels, totalAtlasPixels);
    GameLog::Log("=== END ATLAS STATISTICS ===");
}

void TextureAtlas::EnableMobileOptimizations(bool enable) {
    mobileOptimizations = enable;
}

void TextureAtlas::SetCompressionQuality(float quality) {
    compressionQuality = quality;
}

bool TextureAtlas::PackTextures(const std::vector<TextureInfo>& textures, int atlasSize, std::vector<AtlasEntry>& entries) {
    if (textures.empty()) return true;
    
    // Create root node for the atlas
    PackNode* root = new PackNode(0, 0, atlasSize, atlasSize);
    entries.clear();
    entries.reserve(textures.size());
    
    bool allPacked = true;
    
    // Pack each texture using binary tree algorithm
    for (const auto& texture : textures) {
        PackNode* node = FindNode(root, texture.width, texture.height);
        
        if (node) {
            // Split the node to fit this texture
            PackNode* fit = SplitNode(node, texture.width, texture.height);
            
            if (fit) {
                // Create atlas entry for this texture
                AtlasEntry entry;
                entry.texturePath = texture.path;
                entry.sourceRect = { (float)fit->x, (float)fit->y, (float)texture.width, (float)texture.height };
                entry.originalSize = { (float)texture.originalWidth, (float)texture.originalHeight };
                entry.rotated = texture.rotated;
                entry.category = texture.category;
                entry.isLoaded = true;
                
                entries.push_back(entry);
                
                // Mark node as occupied
                fit->occupied = true;
                fit->texturePath = texture.path;
            } else {
                allPacked = false;
                break;
            }
        } else {
            allPacked = false;
            break;
        }
    }
    
    // Clean up the tree
    delete root;
    
    return allPacked;
}

TextureAtlas::PackNode* TextureAtlas::FindNode(PackNode* root, int width, int height) {
    if (!root) return nullptr;
    
    // If this node is occupied, check children
    if (root->occupied) {
        // Try left child first, then right
        PackNode* node = FindNode(root->left, width, height);
        if (node) return node;
        return FindNode(root->right, width, height);
    }
    
    // Check if this node is big enough
    if (width <= root->width && height <= root->height) {
        // Perfect fit
        if (width == root->width && height == root->height) {
            return root;
        }
        
        // Node is bigger than needed, we can use it
        return root;
    }
    
    // Node is too small
    return nullptr;
}

TextureAtlas::PackNode* TextureAtlas::SplitNode(PackNode* node, int width, int height) {
    if (!node || node->occupied) return nullptr;
    
    // If perfect fit, no need to split
    if (width == node->width && height == node->height) {
        return node;
    }
    
    // If texture doesn't fit, return null
    if (width > node->width || height > node->height) {
        return nullptr;
    }
    
    // Calculate remaining space after placing texture
    int remainingHorizontal = node->width - width;
    int remainingVertical = node->height - height;
    
    // Choose split direction based on which leaves more usable space
    if (remainingHorizontal > remainingVertical) {
        // Split horizontally (left/right)
        node->left = new PackNode(node->x, node->y, width, node->height);
        node->right = new PackNode(node->x + width, node->y, remainingHorizontal, node->height);
        
        // Now split the left node vertically if needed
        if (height < node->height) {
            PackNode* leftNode = node->left;
            leftNode->left = new PackNode(leftNode->x, leftNode->y, width, height);
            leftNode->right = new PackNode(leftNode->x, leftNode->y + height, width, leftNode->height - height);
            return leftNode->left;
        }
        return node->left;
    } else {
        // Split vertically (top/bottom)
        node->left = new PackNode(node->x, node->y, node->width, height);
        node->right = new PackNode(node->x, node->y + height, node->width, remainingVertical);
        
        // Now split the left node horizontally if needed
        if (width < node->width) {
            PackNode* leftNode = node->left;
            leftNode->left = new PackNode(leftNode->x, leftNode->y, width, height);
            leftNode->right = new PackNode(leftNode->x + width, leftNode->y, leftNode->width - width, height);
            return leftNode->left;
        }
        return node->left;
    }
}

bool TextureAtlas::CreateAtlasTexture(AtlasCategory category, const std::vector<TextureInfo>& textures, 
                                     const std::vector<AtlasEntry>& entries, int atlasSize) {
    // Create blank atlas image
    Image atlasImage = GenImageColor(atlasSize, atlasSize, BLANK);
    
    // Draw each texture onto the atlas at its designated position
    for (size_t i = 0; i < textures.size() && i < entries.size(); i++) {
        const TextureInfo& texture = textures[i];
        const AtlasEntry& entry = entries[i];
        
        // Copy texture to atlas position
        ImageDraw(&atlasImage, texture.image, 
                 {0, 0, (float)texture.width, (float)texture.height},
                 {entry.sourceRect.x, entry.sourceRect.y, entry.sourceRect.width, entry.sourceRect.height},
                 WHITE);
    }
    
    // Convert to texture and store
    Texture2D atlasTexture = LoadTextureFromImage(atlasImage);
    UnloadImage(atlasImage);
    
    if (atlasTexture.id == 0) {
        TraceLog(LOG_ERROR, "ATLAS: Failed to create texture from atlas image");
        return false;
    }
    
    // Store the atlas texture
    atlasTextures[category] = atlasTexture;
    
    // Update memory tracking
    size_t textureMemory = atlasSize * atlasSize * 4; // Assuming RGBA format
    totalAtlasMemory += textureMemory;
    atlasCount++;
    
    TraceLog(LOG_INFO, "ATLAS: Created atlas for category %d, size: %dx%d, memory: %zu bytes", 
             (int)category, atlasSize, atlasSize, textureMemory);
    
    return true;
}

int TextureAtlas::NextPowerOfTwo(int value) {
    if (value <= 0) return 1;
    
    // Check if already power of two
    if ((value & (value - 1)) == 0) {
        return value;
    }
    
    // Find next power of two
    int power = 1;
    while (power < value) {
        power <<= 1;
    }
    
    return power;
}