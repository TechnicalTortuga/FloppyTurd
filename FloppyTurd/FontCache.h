#ifndef FONTCACHE_H
#define FONTCACHE_H

#ifdef __cplusplus
extern "C" {
#endif

// Simple FontCache interface for C compatibility
typedef struct FontCache FontCache;

// C interface functions
FontCache* FontCache_GetInstance(void);
int FontCache_Initialize(FontCache* cache);
void FontCache_Shutdown(FontCache* cache);

#ifdef __cplusplus
}

// C++ class implementation
class FontCache {
public:
    // Singleton access
    static FontCache& GetInstance() {
        static FontCache instance;
        return instance;
    }
    
    // Initialize the font cache
    bool Initialize() {
        // Simple initialization - always succeed for now
        return true;
    }
    
    // Cleanup the font cache
    void Shutdown() {
        // Simple cleanup
    }
    
private:
    FontCache() = default;
    ~FontCache() = default;
    
    // Prevent copying
    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;
};

// C interface implementations (inline for header-only)
inline FontCache* FontCache_GetInstance(void) {
    return &FontCache::GetInstance();
}

inline int FontCache_Initialize(FontCache* cache) {
    return cache ? cache->Initialize() : 0;
}

inline void FontCache_Shutdown(FontCache* cache) {
    if (cache) cache->Shutdown();
}

#endif // __cplusplus

#endif // FONTCACHE_H
