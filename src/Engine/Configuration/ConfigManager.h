#pragma once

#include "../Platform/PlatformDelegates.h"
#include "../Core/GNLog.h"
#include <unordered_map>
#include <string>

namespace GameCore {

    /**
     * @class ConfigManager
     * @brief Centralized dynamic configuration management system
     * 
     * Eliminates all hardcoded values by providing dynamic screen and device information,
     * scaling factors, and platform-specific configurations. All systems should query
     * this manager instead of using hardcoded values.
     */
    class ConfigManager {
    public:
        static ConfigManager& Instance();
        
        // Initialization
        void Initialize(const PlatformDelegates& delegates);
        void LoadConfiguration();
        void SaveConfiguration();
        
        // Screen configuration - NO MORE HARDCODED DIMENSIONS!
        const ScreenInfo& GetCurrentScreenInfo() const { return m_screenInfo; }
        void UpdateScreenInfo(); // Call when orientation changes or resolution changes
        void InitializeScreenInfoDirect(); // Direct initialization using Swift CXX interop
        void SetScreenInfoDirect(const ScreenInfo& screenInfo); // Set screen info directly from Swift
        
        // Dynamic scaling factors based on screen size and device type
        float GetUIScale() const;           // UI element scaling
        float GetTextScale() const;         // Text scaling factor
        float GetSpriteScale() const;       // Sprite scaling factor
        float GetBackgroundScale() const;   // Background scaling factor
        
        // Platform detection
        bool IsIOS() const { return m_platform == Platform::iOS; }
        bool IsDesktop() const { return m_platform == Platform::Desktop; }
        bool IsMobile() const { return IsIOS(); } // Add other mobile platforms here
        
        // Dynamic coordinate conversions
        float PixelsToLogical(float pixels) const;
        float LogicalToPixels(float logical) const;
        
        // Responsive layout helpers
        float GetScreenWidthPercent(float percent) const;
        float GetScreenHeightPercent(float percent) const;
        
        // Device-specific optimizations
        bool ShouldUseLowQualityAssets() const;
        int GetRecommendedTextureSize() const;
        int GetMaxParticleCount() const;
        
    private:
        enum class Platform { iOS, Desktop, Unknown };
        
        ConfigManager() = default;
        ~ConfigManager() = default;
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;
        
        // Core data
        ScreenInfo m_screenInfo;
        Platform m_platform = Platform::Unknown;
        PlatformDelegates m_delegates;
        
        // Dynamic scaling factors
        std::unordered_map<std::string, float> m_scaleFactors;
        
        // Helper methods
        void DetectPlatform();
        void CalculateScaleFactors();
        void LoadDefaultConfiguration();
        float CalculateUIScaleFromScreenSize() const;
        float CalculateTextScaleFromDPI() const;
    };

} // namespace GameCore
