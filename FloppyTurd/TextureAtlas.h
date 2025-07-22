#pragma once
#include "PlatformAPI.h"
#include <string>
#include <unordered_map>
#include <vector>

// Structure to store atlas entry information
struct AtlasEntry {
    std::string texturePath;    // Original texture file path
    Rectangle sourceRect;       // Position and size within the atlas texture
    Vector2 originalSize;       // Original texture dimensions
    bool rotated = false;       // Whether texture was rotated in atlas
    AtlasCategory category;     // Which category this texture belongs to
    bool isLoaded = false;
};

// Structure for texture information during atlas building
struct TextureInfo {
    std::string path;
    Image image;
    int width, height;
    int originalWidth, originalHeight;
    bool rotated = false;
    AtlasCategory category;
    int priority;               // Packing priority (larger textures first)
};

// Atlas configuration for different sprite categories
enum class AtlasCategory {
    UI,               // UI elements, buttons, frames, icons
    GAME_OBJECTS,     // Player, enemies, game sprites
    BACKGROUNDS,      // Background images and tiles
    EFFECTS           // Particles, effects, explosions
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
    bool PackTextures(const std::vector<TextureInfo>& textures, int atlasSize, std::vector<AtlasEntry>& entries);
    bool CreateAtlasTexture(AtlasCategory category, const std::vector<TextureInfo>& textures, 
                           const std::vector<AtlasEntry>& entries, int atlasSize);
    int NextPowerOfTwo(int value);
    
    // Binary tree packing algorithm
    struct PackNode {
        int x, y, width, height;
        bool occupied = false;
        PackNode* left = nullptr;
        PackNode* right = nullptr;
        std::string texturePath;    // Path of texture in this node (if occupied)
        
        PackNode(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
        
        ~PackNode() {
            delete left;
            delete right;
        }
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