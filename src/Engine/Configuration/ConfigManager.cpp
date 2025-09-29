#include "ConfigManager.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace GameCore {

    ConfigManager& ConfigManager::Instance() {
        static ConfigManager instance;
        return instance;
    }

    void ConfigManager::Initialize(const PlatformDelegates& delegates) {
        m_delegates = delegates;
        DetectPlatform();
        UpdateScreenInfo();
        LoadDefaultConfiguration();
        CalculateScaleFactors();
        
        GN_LOG_INFO("ConfigManager initialized for platform: " + std::string(m_delegates.GetPlatformName()));
        GN_LOG_INFO("Screen info: " + std::to_string(m_screenInfo.pixelWidth) + "x" + 
                   std::to_string(m_screenInfo.pixelHeight) + " pixels, " +
                   std::to_string(m_screenInfo.logicalWidth) + "x" + 
                   std::to_string(m_screenInfo.logicalHeight) + " logical, scale=" + 
                   std::to_string(m_screenInfo.scaleFactor));
    }

    void ConfigManager::LoadConfiguration() {
        // TODO: Load from configuration file in the future
        // For now, use dynamic detection
        LoadDefaultConfiguration();
    }

    void ConfigManager::SaveConfiguration() {
        // TODO: Save configuration to file in the future
        GN_LOG_DEBUG("ConfigManager: Configuration saved (placeholder)");
    }

    void ConfigManager::UpdateScreenInfo() {
        // Prefer enhanced info; do not clobber cached valid info with legacy fallbacks
        if (m_delegates.renderer.getScreenInfo) {
            m_delegates.renderer.getScreenInfo(&m_screenInfo);
            CalculateScaleFactors(); // Recalculate when screen changes
            GN_LOG_INFO("Screen info updated: " + std::to_string(m_screenInfo.pixelWidth) + "x" +
                       std::to_string(m_screenInfo.pixelHeight) + " pixels");

            // Trigger callback if set (for automatic updates)
            if (m_screenInfoUpdateCallback) {
                m_screenInfoUpdateCallback();
                GN_LOG_DEBUG("Screen info update callback triggered");
            }

            return;
        }
        if (m_screenInfo.pixelWidth > 0.0f && m_screenInfo.pixelHeight > 0.0f) {
            // Keep existing cached values
            return;
        }
        if (m_delegates.renderer.getScreenSize) {
            // Legacy once if nothing cached
            m_delegates.renderer.getScreenSize(&m_screenInfo.pixelWidth, &m_screenInfo.pixelHeight);
            m_screenInfo.logicalWidth = m_screenInfo.pixelWidth;
            m_screenInfo.logicalHeight = m_screenInfo.pixelHeight;
            m_screenInfo.scaleFactor = 1.0f;
            m_screenInfo.isPortrait = m_screenInfo.pixelHeight > m_screenInfo.pixelWidth;
            m_screenInfo.deviceModel = "Unknown";
            CalculateScaleFactors();
            GN_LOG_WARN("Using legacy screen size detection once: " + std::to_string(m_screenInfo.pixelWidth) + "x" +
                       std::to_string(m_screenInfo.pixelHeight));
            return;
        }
        // Absolute last resort defaults (iPhone 16 portrait) to avoid 800x600 behavior
        m_screenInfo.pixelWidth = 1179.0f;
        m_screenInfo.pixelHeight = 2556.0f;
        m_screenInfo.logicalWidth = m_screenInfo.pixelWidth;
        m_screenInfo.logicalHeight = m_screenInfo.pixelHeight;
        m_screenInfo.scaleFactor = 1.0f;
        m_screenInfo.isPortrait = true;
        m_screenInfo.deviceModel = "Unknown";
        CalculateScaleFactors();
        GN_LOG_WARN("Using hardcoded pixel defaults for screen info (no delegates available)");
    }

    void ConfigManager::SetScreenInfoUpdateCallback(ScreenInfoUpdateCallback callback) {
        m_screenInfoUpdateCallback = callback;
        GN_LOG_DEBUG("Screen info update callback set");
    }

    float ConfigManager::GetUIScale() const {
        auto it = m_scaleFactors.find("ui_scale");
        return it != m_scaleFactors.end() ? it->second : 1.0f;
    }

    float ConfigManager::GetTextScale() const {
        auto it = m_scaleFactors.find("text_scale");
        return it != m_scaleFactors.end() ? it->second : 1.0f;
    }

    float ConfigManager::GetSpriteScale() const {
        auto it = m_scaleFactors.find("sprite_scale");
        return it != m_scaleFactors.end() ? it->second : 1.0f;
    }

    float ConfigManager::GetBackgroundScale() const {
        auto it = m_scaleFactors.find("background_scale");
        return it != m_scaleFactors.end() ? it->second : 1.0f;
    }

    float ConfigManager::PixelsToLogical(float pixels) const {
        return pixels / m_screenInfo.scaleFactor;
    }

    float ConfigManager::LogicalToPixels(float logical) const {
        return logical * m_screenInfo.scaleFactor;
    }

    float ConfigManager::GetScreenWidthPercent(float percent) const {
        return m_screenInfo.logicalWidth * (percent / 100.0f);
    }

    float ConfigManager::GetScreenHeightPercent(float percent) const {
        return m_screenInfo.logicalHeight * (percent / 100.0f);
    }

    bool ConfigManager::ShouldUseLowQualityAssets() const {
        // Use lower quality on older devices or smaller screens
        return m_screenInfo.pixelWidth < 1000.0f || 
               m_screenInfo.deviceModel.find("SE") != std::string::npos;
    }

    int ConfigManager::GetRecommendedTextureSize() const {
        if (m_screenInfo.pixelWidth >= 2000.0f) {
            return 2048; // High resolution devices
        } else if (m_screenInfo.pixelWidth >= 1500.0f) {
            return 1024; // Medium resolution devices
        } else {
            return 512;  // Lower resolution devices
        }
    }

    int ConfigManager::GetMaxParticleCount() const {
        if (IsIOS() && m_screenInfo.pixelWidth >= 2000.0f) {
            return 500; // High-end iOS devices
        } else if (IsIOS()) {
            return 250; // Standard iOS devices
        } else {
            return 1000; // Desktop can handle more
        }
    }

    void ConfigManager::DetectPlatform() {
        switch (m_delegates.platformType) {
            case PlatformDelegates::PLATFORM_TYPE_IOS:
                m_platform = Platform::iOS;
                break;
            case PlatformDelegates::PLATFORM_TYPE_MACOS:
            case PlatformDelegates::PLATFORM_TYPE_WINDOWS:
            case PlatformDelegates::PLATFORM_TYPE_LINUX:
                m_platform = Platform::Desktop;
                break;
            default:
                m_platform = Platform::Unknown;
                GN_LOG_WARN("Unknown platform type detected");
                break;
        }
    }

    void ConfigManager::CalculateScaleFactors() {
        // Calculate UI scale based on screen size
        float uiScale = CalculateUIScaleFromScreenSize();
        float textScale = CalculateTextScaleFromDPI();
        
        // Background scale should fill the screen height
        float backgroundScale = 1.0f;
        
        // Store calculated scales
        m_scaleFactors["ui_scale"] = uiScale;
        m_scaleFactors["text_scale"] = textScale;
        m_scaleFactors["sprite_scale"] = uiScale; // Sprites use same scale as UI
        m_scaleFactors["background_scale"] = backgroundScale;
        
        GN_LOG_DEBUG("Calculated scale factors - UI: " + std::to_string(uiScale) + 
                    ", Text: " + std::to_string(textScale) + 
                    ", Background: " + std::to_string(backgroundScale));
    }

    void ConfigManager::LoadDefaultConfiguration() {
        // Platform-specific default configurations
        if (IsIOS()) {
            // iOS defaults - scale based on device size
            if (m_screenInfo.pixelWidth >= 2000.0f) {
                // iPhone Pro Max, iPad Pro
                m_scaleFactors["default_font_size"] = 96.0f;
                m_scaleFactors["button_scale"] = 1.2f;
            } else if (m_screenInfo.pixelWidth >= 1500.0f) {
                // iPhone Pro, standard iPhones
                m_scaleFactors["default_font_size"] = 64.0f;
                m_scaleFactors["button_scale"] = 1.0f;
            } else {
                // iPhone SE, smaller devices
                m_scaleFactors["default_font_size"] = 48.0f;
                m_scaleFactors["button_scale"] = 0.8f;
            }
        } else {
            // Desktop defaults
            m_scaleFactors["default_font_size"] = 32.0f;
            m_scaleFactors["button_scale"] = 1.0f;
        }
    }

    float ConfigManager::CalculateUIScaleFromScreenSize() const {
        if (IsIOS()) {
            // iOS: Scale based on logical width
            // iPhone SE (375pt) = 0.8x, iPhone (393pt) = 1.0x, iPhone Pro Max (430pt) = 1.2x
            float baseWidth = 393.0f; // iPhone 14/15 standard width
            return std::max(0.5f, std::min(2.0f, m_screenInfo.logicalWidth / baseWidth));
        } else {
            // Desktop: Scale based on screen resolution
            float baseWidth = 1920.0f; // 1080p width
            return std::max(0.5f, std::min(3.0f, m_screenInfo.pixelWidth / baseWidth));
        }
    }

    float ConfigManager::CalculateTextScaleFromDPI() const {
        if (IsIOS()) {
            // iOS handles DPI automatically, use UI scale
            return GetUIScale();
        } else {
            // Desktop: Estimate DPI and scale accordingly
            // Assume 24-inch 1080p monitor = ~92 DPI, 27-inch 1440p = ~109 DPI
            float estimatedDPI = 96.0f; // Standard DPI assumption
            if (m_screenInfo.pixelWidth >= 2560.0f) {
                estimatedDPI = 120.0f; // High DPI display
            }
            return estimatedDPI / 96.0f; // Scale relative to 96 DPI
        }
    }

    void ConfigManager::SetScreenInfoDirect(const ScreenInfo& screenInfo) {
        m_screenInfo = screenInfo;
        CalculateScaleFactors();

        GN_LOG_INFO("ConfigManager: Screen info set directly - " +
                   std::to_string((int)m_screenInfo.pixelWidth) + "x" +
                   std::to_string((int)m_screenInfo.pixelHeight) + " pixels, " +
                   std::to_string(m_screenInfo.logicalWidth) + "x" +
                   std::to_string(m_screenInfo.logicalHeight) + " logical, " +
                   "scale: " + std::to_string(m_screenInfo.scaleFactor) +
                   ", portrait: " + (m_screenInfo.isPortrait ? "true" : "false"));

        // CRITICAL FIX: Trigger callback for orientation changes
        // This ensures GameplayState gets notified of screen info changes
        if (m_screenInfoUpdateCallback) {
            m_screenInfoUpdateCallback();
            GN_LOG_INFO("Screen info update callback triggered for orientation change");
        }
    }

} // namespace GameCore
