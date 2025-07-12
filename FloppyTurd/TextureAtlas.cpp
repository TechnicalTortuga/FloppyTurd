#include "TextureAtlas.h"
#include <algorithm>
#include <memory>
#include <numeric>

void TextureAtlas::Initialize() {
    // Initialize category names for debugging
    categoryNames[AtlasCategory::UI_ELEMENTS] = "UI_ELEMENTS";
    categoryNames[AtlasCategory::PLAYER_SPRITES] = "PLAYER_SPRITES";
    categoryNames[AtlasCategory::ENEMY_SPRITES] = "ENEMY_SPRITES";
    categoryNames[AtlasCategory::ENVIRONMENT] = "ENVIRONMENT";
    categoryNames[AtlasCategory::PARTICLES] = "PARTICLES";
    
#ifdef PLATFORM_MOBILE
    EnableMobileOptimizations(true);
#endif
    
    TraceLog(LOG_INFO, "TextureAtlas: Initialized with mobile optimizations: %s", 
             mobileOptimizations ? "ON" : "OFF");
}

void TextureAtlas::Shutdown() {
    // Unload all atlas textures
    for (auto& [category, texture] : atlasTextures) {
        if (texture.id != 0) {
            UnloadTexture(texture);
        }
    }
    
    atlasTextures.clear();
    atlasEntries.clear();
    totalAtlasMemory = 0;
    atlasCount = 0;
    
    TraceLog(LOG_INFO, "TextureAtlas: Shutdown complete");
}

bool TextureAtlas::BuildAtlas(AtlasCategory category, const std::vector<std::string>& texturePaths, int maxAtlasSize) {
    // Adjust atlas size for mobile
    if (mobileOptimizations) {
        maxAtlasSize = std::min(maxAtlasSize, maxMobileAtlasSize);
    }
    
    std::vector<Image> images;
    std::vector<std::string> validPaths;
    
    // Load all images
    for (const auto& path : texturePaths) {
        std::string fullPath = PlatformAPI::GetPlatformImpl()->GetResourcePath(path);
        Image img = LoadImage(fullPath.c_str());
        
        if (img.data != nullptr) {
            images.push_back(img);
            validPaths.push_back(path);
        } else {
            TraceLog(LOG_WARNING, "TextureAtlas: Failed to load image: %s", path.c_str());
        }
    }
    
    if (images.empty()) {
        TraceLog(LOG_ERROR, "TextureAtlas: No valid images found for category %s", 
                 categoryNames[category].c_str());
        return false;
    }
    
    // Pack textures into atlas
    Image atlasImage;
    std::vector<AtlasEntry> entries;
    
    if (!PackTextures(images, validPaths, maxAtlasSize, atlasImage, entries)) {
        // Cleanup loaded images
        for (auto& img : images) {
            UnloadImage(img);
        }
        return false;
    }
    
    // Apply mobile compression if enabled
    if (mobileOptimizations && compressionQuality < 1.0f) {
        // Simple quality reduction by downscaling
        int newWidth = (int)(atlasImage.width * compressionQuality);
        int newHeight = (int)(atlasImage.height * compressionQuality);
        ImageResize(&atlasImage, newWidth, newHeight);
        
        // Update entry rectangles
        for (auto& entry : entries) {
            entry.sourceRect.x *= compressionQuality;
            entry.sourceRect.y *= compressionQuality;
            entry.sourceRect.width *= compressionQuality;
            entry.sourceRect.height *= compressionQuality;
        }
    }
    
    // Create texture from atlas image
    Texture2D atlasTexture = LoadTextureFromImage(atlasImage);
    UnloadImage(atlasImage);
    
    // Cleanup individual images
    for (auto& img : images) {
        UnloadImage(img);
    }
    
    if (atlasTexture.id == 0) {
        TraceLog(LOG_ERROR, "TextureAtlas: Failed to create texture for category %s", 
                 categoryNames[category].c_str());
        return false;
    }
    
    // Store atlas texture and entries
    atlasTextures[category] = atlasTexture;
    
    for (const auto& entry : entries) {
        atlasEntries[entry.originalPath] = entry;
    }
    
    // Update statistics
    size_t atlasMemory = atlasTexture.width * atlasTexture.height * 4; // RGBA
    totalAtlasMemory += atlasMemory;
    atlasCount++;
    
    TraceLog(LOG_INFO, "TextureAtlas: Built %s atlas (%dx%d, %d textures, %.1fKB)", 
             categoryNames[category].c_str(), atlasTexture.width, atlasTexture.height, 
             (int)entries.size(), atlasMemory / 1024.0f);
    
    return true;
}

bool TextureAtlas::PackTextures(const std::vector<Image>& images, const std::vector<std::string>& paths,
                               int maxSize, Image& atlasImage, std::vector<AtlasEntry>& entries) {
    
    if (images.empty()) return false;
    
    // Sort images by height (descending) for better packing
    std::vector<size_t> indices(images.size());
    std::iota(indices.begin(), indices.end(), 0);
    
    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        return images[a].height > images[b].height;
    });
    
    // Start with a reasonable size and grow if needed
    int atlasWidth = 512;
    int atlasHeight = 512;
    
    while (atlasWidth <= maxSize && atlasHeight <= maxSize) {
        // Try to pack with current size
        auto root = std::make_unique<PackNode>(0, 0, atlasWidth, atlasHeight);
        entries.clear();
        bool allFit = true;
        
        for (size_t idx : indices) {
            const Image& img = images[idx];
            
            PackNode* node = FindNode(root.get(), img.width, img.height);
            if (node) {
                PackNode* fit = SplitNode(node, img.width, img.height);
                
                AtlasEntry entry;
                entry.sourceRect = { (float)fit->x, (float)fit->y, (float)img.width, (float)img.height };
                entry.originalSize = { (float)img.width, (float)img.height };
                entry.originalPath = paths[idx];
                entry.isLoaded = true;
                
                entries.push_back(entry);
            } else {
                allFit = false;
                break;
            }
        }
        
        if (allFit) {
            // Create atlas image
            atlasImage = GenImageColor(atlasWidth, atlasHeight, BLANK);
            
            // Copy images to atlas
            for (size_t i = 0; i < entries.size(); i++) {
                size_t idx = indices[i];
                const AtlasEntry& entry = entries[i];
                
                Rectangle sourceRect = { 0, 0, (float)images[idx].width, (float)images[idx].height };
                ImageDraw(&atlasImage, images[idx], sourceRect, entry.sourceRect, WHITE);
            }
            
            return true;
        }
        
        // Grow atlas size
        if (atlasWidth <= atlasHeight) {
            atlasWidth *= 2;
        } else {
            atlasHeight *= 2;
        }
    }
    
    TraceLog(LOG_ERROR, "TextureAtlas: Could not fit all textures in atlas (max size: %d)", maxSize);
    return false;
}

TextureAtlas::PackNode* TextureAtlas::FindNode(PackNode* root, int width, int height) {
    if (root->used) {
        PackNode* result = FindNode(root->right.get(), width, height);
        if (result) return result;
        return FindNode(root->down.get(), width, height);
    } else if (width <= root->width && height <= root->height) {
        return root;
    }
    return nullptr;
}

TextureAtlas::PackNode* TextureAtlas::SplitNode(PackNode* node, int width, int height) {
    node->used = true;
    
    node->down = std::make_unique<PackNode>(node->x, node->y + height, node->width, node->height - height);
    node->right = std::make_unique<PackNode>(node->x + width, node->y, node->width - width, height);
    
    return node;
}

Rectangle TextureAtlas::GetTextureRegion(const std::string& texturePath) {
    auto it = atlasEntries.find(texturePath);
    if (it != atlasEntries.end() && it->second.isLoaded) {
        return it->second.sourceRect;
    }
    
    // Return empty rectangle if not found
    return Rectangle{ 0, 0, 0, 0 };
}

Texture2D TextureAtlas::GetAtlasTexture(AtlasCategory category) {
    auto it = atlasTextures.find(category);
    if (it != atlasTextures.end()) {
        return it->second;
    }
    
    // Return empty texture if not found
    return Texture2D();
}

bool TextureAtlas::IsTextureAtlased(const std::string& texturePath) {
    auto it = atlasEntries.find(texturePath);
    return it != atlasEntries.end() && it->second.isLoaded;
}

int TextureAtlas::GetAtlasCount() const {
    return atlasCount;
}

size_t TextureAtlas::GetTotalAtlasMemory() const {
    return totalAtlasMemory;
}

void TextureAtlas::PrintAtlasStats() const {
    TraceLog(LOG_INFO, "=== TextureAtlas Statistics ===");
    TraceLog(LOG_INFO, "Atlas count: %d", atlasCount);
    TraceLog(LOG_INFO, "Total memory: %.2fMB", totalAtlasMemory / (1024.0f * 1024.0f));
    TraceLog(LOG_INFO, "Atlased textures: %d", (int)atlasEntries.size());
    
    for (const auto& [category, texture] : atlasTextures) {
        size_t memory = texture.width * texture.height * 4;
        TraceLog(LOG_INFO, "  %s: %dx%d (%.1fKB)", 
                 categoryNames.at(category).c_str(), 
                 texture.width, texture.height, 
                 memory / 1024.0f);
    }
}

void TextureAtlas::EnableMobileOptimizations(bool enable) {
    mobileOptimizations = enable;
    if (enable) {
        maxMobileAtlasSize = 1024;  // Conservative size for mobile GPUs
        compressionQuality = 0.9f;  // Slight quality reduction
    } else {
        compressionQuality = 1.0f;  // Full quality
    }
}

void TextureAtlas::SetCompressionQuality(float quality) {
    compressionQuality = std::clamp(quality, 0.1f, 1.0f);
}