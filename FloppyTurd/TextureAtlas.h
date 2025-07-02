#pragma once
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

// Structure to store atlas entry information
struct AtlasEntry {
    Rectangle sourceRect;       // Position and size within the atlas texture
    Vector2 originalSize;       // Original texture dimensions
    std::string originalPath;   // Original file path for reference
    bool isLoaded = false;
};

// Atlas configuration for different sprite categories
enum class AtlasCategory {
    UI_ELEMENTS,      // Buttons, frames, icons
    PLAYER_SPRITES,   // Player animations and hats
    ENEMY_SPRITES,    // Enemy animations and projectiles
    ENVIRONMENT,      // Backgrounds, tiles, decorations
    PARTICLES         // Effects, explosions, pickups
};

class TextureAtlas {
public:
    static TextureAtlas& GetInstance() {
        static TextureAtlas instance;
        return instance;
    }

    // Initialize atlas system
    void Initialize();
    void Shutdown();

    // Build atlases from resource definitions
    bool BuildAtlas(AtlasCategory category, const std::vector<std::string>& texturePaths, 
                   int maxAtlasSize = 2048);
    
    // Get texture region from atlas
    Rectangle GetTextureRegion(const std::string& texturePath);
    Texture2D GetAtlasTexture(AtlasCategory category);
    
    // Check if texture is atlased
    bool IsTextureAtlased(const std::string& texturePath);
    
    // Performance utilities
    int GetAtlasCount() const;
    size_t GetTotalAtlasMemory() const;
    void PrintAtlasStats() const;
    
    // Mobile-specific optimizations
    void EnableMobileOptimizations(bool enable);
    void SetCompressionQuality(float quality); // 0.0 - 1.0

private:
    TextureAtlas() = default;
    ~TextureAtlas() = default;
    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;

    // Atlas building helpers
    bool PackTextures(const std::vector<Image>& images, const std::vector<std::string>& paths,
                     int maxSize, Image& atlasImage, std::vector<AtlasEntry>& entries);
    
    // Simple rect packing algorithm
    struct PackNode {
        int x, y, width, height;
        bool used = false;
        std::unique_ptr<PackNode> right, down;
        
        PackNode(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
    };
    
    PackNode* FindNode(PackNode* root, int width, int height);
    PackNode* SplitNode(PackNode* node, int width, int height);

    // Atlas storage
    std::unordered_map<AtlasCategory, Texture2D> atlasTextures;
    std::unordered_map<std::string, AtlasEntry> atlasEntries;
    std::unordered_map<AtlasCategory, std::string> categoryNames;
    
    // Performance tracking
    size_t totalAtlasMemory = 0;
    int atlasCount = 0;
    
    // Mobile optimization settings
    bool mobileOptimizations = false;
    float compressionQuality = 0.8f;
    int maxMobileAtlasSize = 1024;
}; 