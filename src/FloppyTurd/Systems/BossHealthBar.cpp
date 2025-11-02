#include "BossHealthBar.h"
#include "BossSystem.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <cmath>
#include <chrono>
#include <algorithm>

namespace GameCore {

BossHealthBar::BossHealthBar(BossSystem* bossSystem, const char* bossName, Gnosis::ECS* ecsSystem)
    : m_bossSystem(bossSystem)
    , m_bossName(bossName)
    , m_ecsSystem(ecsSystem)
    , m_currentHealthPercent(1.0f)
    , m_shadowHealthPercent(1.0f)
    , m_hurtFadeTimer(0.0f)
    , m_displayedHealthPercent(1.0f)
    , m_whiteTrimDelay(0.0f)
{
    GN_LOG_INFO("BossHealthBar created for: " + std::string(bossName));
    CreateUIEntities();
}

BossHealthBar::~BossHealthBar() {
    // Clean up UI entities
    if (m_ecsSystem) {
        if (m_backgroundEntity != 0) m_ecsSystem->DestroyEntity(m_backgroundEntity);
        if (m_healthFillEntity != 0) m_ecsSystem->DestroyEntity(m_healthFillEntity);
        if (m_hurtEffectEntity != 0) m_ecsSystem->DestroyEntity(m_hurtEffectEntity);
        if (m_bossNameEntity != 0) m_ecsSystem->DestroyEntity(m_bossNameEntity);
        // Health text entity removed as requested
    }
    GN_LOG_INFO("BossHealthBar destroyed");
}

void BossHealthBar::CreateUIEntities() {
    if (!m_ecsSystem) return;

    // Get current screen dimensions for dynamic positioning
    float screenWidth = 1179.0f;  // Default iPhone 16 width
    float screenHeight = 2556.0f; // Default iPhone 16 height

    // Calculate dynamic positions - adjust for landscape mode
    bool isLandscape = (screenWidth > screenHeight);
    float barX;
    if (isLandscape) {
        // Landscape mode: move boss bar to the right to avoid UI overlap
        barX = screenWidth * 0.50f;  // 50% from left in landscape
    } else {
        // Portrait mode: original positioning
        barX = screenWidth * 0.25f;  // 25% from left for better positioning
    }
    float barY = screenHeight * BAR_TOP_OFFSET;
    float nameY = screenHeight * NAME_OFFSET;
    float scale = 8.0f;  // 8x scale for proper visibility
    
    // Store barX for use in UpdateUIEntities
    m_barX = barX;
    m_barY = barY;

    GN_LOG_INFO("BossHealthBar positioning: screen(" + std::to_string((int)screenWidth) + "x" + std::to_string((int)screenHeight) +
               "), isLandscape=" + std::to_string(isLandscape) + ", barX=" + std::to_string(barX));

    // Create frame background entity using BossBarFrame sprite
    m_backgroundEntity = m_ecsSystem->CreateEntity();
    Transform frameTransform(Gnosis::GNVector2(barX, barY), 0.0f, Gnosis::GNVector2(scale, scale));
    m_ecsSystem->AddComponent<Transform>(m_backgroundEntity, frameTransform);

    Sprite frameSprite("BossBarFrame", ORIGINAL_WIDTH * scale, ORIGINAL_HEIGHT * scale);
    frameSprite.visible = true;
    frameSprite.layer = 15;
    m_ecsSystem->AddComponent<Sprite>(m_backgroundEntity, frameSprite);

    // Create health sprite with FIXED destination rendering
    m_healthFillEntity = m_ecsSystem->CreateEntity();
    Sprite healthSprite("BossBarHealth", ORIGINAL_WIDTH * scale, ORIGINAL_HEIGHT * scale);
    healthSprite.visible = true;
    healthSprite.isAnimated = false;
    healthSprite.frameCount = 1;
    healthSprite.frameWidth = ORIGINAL_WIDTH;
    healthSprite.frameHeight = ORIGINAL_HEIGHT;
    healthSprite.layer = 13; // Behind frame (layer 15) and hurt effect (layer 14)
    
    // Enable fixed-destination rendering to prevent position shifting when clipping
    healthSprite.useFixedDestination = true;
    healthSprite.fixedWidth = ORIGINAL_WIDTH * scale;   // Always render at full bar width (1280 pixels)
    healthSprite.fixedHeight = ORIGINAL_HEIGHT * scale; // Always render at full bar height (256 pixels)
    
    // Source rect will be changed each frame for clipping effect
    healthSprite.sourceX = 0;
    healthSprite.sourceY = 0;
    healthSprite.sourceWidth = ORIGINAL_WIDTH;  // Full width initially
    healthSprite.sourceHeight = ORIGINAL_HEIGHT;
    
    m_ecsSystem->AddComponent<Sprite>(m_healthFillEntity, healthSprite);
    Transform healthTrans(Gnosis::GNVector2(barX, barY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f)); // Scale is baked into sprite size
    m_ecsSystem->AddComponent<Transform>(m_healthFillEntity, healthTrans);

    // Create hurt effect entity (white bar for damage flash) with FIXED destination rendering
    // This renders BEHIND the red health bar, so when health clips, white shows through
    m_hurtEffectEntity = m_ecsSystem->CreateEntity();
    Sprite hurtSprite("BossBarHurt", ORIGINAL_WIDTH * scale, ORIGINAL_HEIGHT * scale);
    hurtSprite.visible = true; // Always visible, alpha controls fade
    hurtSprite.isAnimated = false;
    hurtSprite.frameCount = 1;
    hurtSprite.frameWidth = ORIGINAL_WIDTH;
    hurtSprite.frameHeight = ORIGINAL_HEIGHT;
    hurtSprite.sourceX = 0;
    hurtSprite.sourceY = 0;
    hurtSprite.sourceWidth = ORIGINAL_WIDTH;
    hurtSprite.sourceHeight = ORIGINAL_HEIGHT;
    hurtSprite.layer = 12; // BEHIND health bar (13) so it shows through when health clips
    hurtSprite.color = Gnosis::GNColor(255, 255, 255, 0); // Start transparent
    
    // Enable fixed-destination rendering to prevent position shifting when clipping
    hurtSprite.useFixedDestination = true;
    hurtSprite.fixedWidth = ORIGINAL_WIDTH * scale;   // Always render at full bar width (1280 pixels)
    hurtSprite.fixedHeight = ORIGINAL_HEIGHT * scale; // Always render at full bar height (256 pixels)
    
    m_ecsSystem->AddComponent<Sprite>(m_hurtEffectEntity, hurtSprite);
    // Position hurt effect exactly the same as health bar
    Transform hurtTrans(Gnosis::GNVector2(barX, barY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_hurtEffectEntity, hurtTrans);

    // Create boss name text entity (positioned relative to scaled bar)
    m_bossNameEntity = m_ecsSystem->CreateEntity();
    Transform nameTransform(Gnosis::GNVector2(barX + (ORIGINAL_WIDTH * scale * 0.1f), nameY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_bossNameEntity, nameTransform);

    UIElement nameElement(m_bossName, "");
    nameElement.fontSize = 88.0f; // Same as Main Menu global UI font size for mobile
    nameElement.textColor = Gnosis::GNColor(255, 215, 0, 255); // Gold color like old implementation
    nameElement.centerTextHorizontally = false; // Left-aligned like old implementation
    nameElement.centerTextVertically = false;
    nameElement.textLayer = 18;
    nameElement.visible = false; // Initially hidden
    m_ecsSystem->AddComponent<UIElement>(m_bossNameEntity, nameElement);

    // Health percentage text removed as requested

    GN_LOG_INFO("BossHealthBar UI entities created with texture-based design (matching old implementation)");
}

void BossHealthBar::Update(float deltaTime) {
    if (!IsVisible()) return;

    UpdateHealthValues();

    // Update hurt effect fade and trim delay
    if (m_hurtFadeTimer > 0.0f) {
        m_hurtFadeTimer -= deltaTime;
        if (m_hurtFadeTimer < 0.0f) {
            m_hurtFadeTimer = 0.0f;
            m_shadowHealthPercent = m_currentHealthPercent;  // Reset shadow to match current
        }
    }
    
    // Update white bar trim delay - starts after a brief pause, then interpolates to current health
    if (m_whiteTrimDelay > 0.0f) {
        m_whiteTrimDelay -= deltaTime;
        if (m_whiteTrimDelay < 0.0f) {
            m_whiteTrimDelay = 0.0f;
        }
    } else if (m_shadowHealthPercent > m_currentHealthPercent) {
        // Linearly interpolate shadow health toward current health
        float trimSpeed = 0.8f; // Units per second
        m_shadowHealthPercent -= trimSpeed * deltaTime;
        if (m_shadowHealthPercent < m_currentHealthPercent) {
            m_shadowHealthPercent = m_currentHealthPercent;
        }
    }

    // Update UI entities based on current health
    UpdateUIEntities();
}

void BossHealthBar::SetVisible(bool visible) {
    if (!m_ecsSystem) return;

    // Track manual visibility changes (for pause menu)
    m_manuallyHidden = !visible;

    // Set visibility for frame sprite
    if (m_backgroundEntity != 0) {
        Sprite* frameSprite = m_ecsSystem->GetComponent<Sprite>(m_backgroundEntity);
        if (frameSprite) frameSprite->visible = visible;
    }

    if (m_healthFillEntity != 0) {
        Sprite* healthSprite = m_ecsSystem->GetComponent<Sprite>(m_healthFillEntity);
        if (healthSprite) healthSprite->visible = visible;
    }

    if (m_hurtEffectEntity != 0) {
        Sprite* hurtSprite = m_ecsSystem->GetComponent<Sprite>(m_hurtEffectEntity);
        if (hurtSprite) hurtSprite->visible = visible;
    }

    // Set visibility for text element
    if (m_bossNameEntity != 0) {
        UIElement* nameElement = m_ecsSystem->GetComponent<UIElement>(m_bossNameEntity);
        if (nameElement) nameElement->visible = visible;
    }
}

void BossHealthBar::UpdateUIEntities() {
    if (!m_ecsSystem) return;

    bool shouldBeVisible = IsVisible();

    // Update background visibility (always visible when boss bar is shown)
    if (m_backgroundEntity != 0) {
        Sprite* frameSprite = m_ecsSystem->GetComponent<Sprite>(m_backgroundEntity);
        if (frameSprite) frameSprite->visible = shouldBeVisible;
    }

    // Update health sprite - FIXED DESTINATION rendering with UV clipping
    if (m_healthFillEntity != 0) {
        Sprite* health = m_ecsSystem->GetComponent<Sprite>(m_healthFillEntity);
        if (health) {
            health->visible = shouldBeVisible && (m_displayedHealthPercent > 0.0f);
            
            // ONLY change sourceWidth for UV clipping - destination size stays constant via fixedWidth/Height
            health->sourceX = 0;
            health->sourceY = 0;
            health->sourceWidth = ORIGINAL_WIDTH * m_displayedHealthPercent;  // Clip texture from right
            health->sourceHeight = ORIGINAL_HEIGHT;
            
            // Position stays locked - no need to adjust since sprite renders at fixed size
            // The Metal renderer will keep the quad at constant size, only UV coords change
        }
    }

    // Update hurt effect (white bar) - FIXED DESTINATION rendering with UV clipping
    if (m_hurtEffectEntity != 0) {
        Sprite* hurt = m_ecsSystem->GetComponent<Sprite>(m_hurtEffectEntity);
        if (hurt && shouldBeVisible) {
            // Hurt bar is visible when there's damage to show
            hurt->visible = true;
            
            // Trim the white bar via SOURCE rectangle (UV clipping) - destination stays constant
            hurt->sourceX = 0;
            hurt->sourceY = 0;
            hurt->sourceWidth = ORIGINAL_WIDTH * m_shadowHealthPercent;  // Clip texture from right
            hurt->sourceHeight = ORIGINAL_HEIGHT;
            
            // Alpha controls the fade: visible when damaged, fades out over time
            if (m_shadowHealthPercent > m_currentHealthPercent && m_hurtFadeTimer > 0.0f) {
                // Calculate fade based on timer
                float fadeProgress = m_hurtFadeTimer / HURT_FADE_DURATION;
                if (fadeProgress < 0.0f) fadeProgress = 0.0f;
                if (fadeProgress > 1.0f) fadeProgress = 1.0f;
                uint8_t alpha = static_cast<uint8_t>(fadeProgress * 255.0f);
                hurt->color = Gnosis::GNColor(255, 255, 255, alpha);
            } else {
                // No damage or timer expired, fully transparent
                hurt->color = Gnosis::GNColor(255, 255, 255, 0);
            }
            
            // Position stays locked - no adjustment needed with fixed-destination rendering
        } else if (hurt) {
            hurt->visible = false;
        }
    }

    // Update boss name visibility
    if (m_bossNameEntity != 0) {
        UIElement* nameElement = m_ecsSystem->GetComponent<UIElement>(m_bossNameEntity);
        if (nameElement) nameElement->visible = shouldBeVisible;
    }

    // Health percentage text removed as requested
}

bool BossHealthBar::IsVisible() const {
    // Respect manual hiding (for pause menu) even if boss is active
    if (m_manuallyHidden) {
        return false;
    }
    return m_bossSystem && m_bossSystem->IsActive() && m_bossSystem->GetHealth() > 0;
}

void BossHealthBar::UpdateHealthValues() {
    if (!m_bossSystem) return;

    float newHealthPercent = static_cast<float>(m_bossSystem->GetHealth()) /
                            static_cast<float>(m_bossSystem->GetMaxHealth());

    // Check if health decreased (trigger hurt effect)
    if (newHealthPercent < m_currentHealthPercent) {
        m_shadowHealthPercent = m_currentHealthPercent;  // Store old health for fade effect
        m_hurtFadeTimer = HURT_FADE_DURATION;            // Start fade timer
        m_whiteTrimDelay = 0.3f;                         // Wait 0.3 seconds before trimming white bar
    }

    m_currentHealthPercent = newHealthPercent; // Set target
    m_displayedHealthPercent = m_currentHealthPercent; // Update displayed value
}

void BossHealthBar::Reset() {
    // Reset all health percentages to full
    m_currentHealthPercent = 1.0f;
    m_shadowHealthPercent = 1.0f;
    m_displayedHealthPercent = 1.0f;
    m_hurtFadeTimer = 0.0f;
    m_whiteTrimDelay = 0.0f;
    
    // Update UI entities to show full health
    UpdateUIEntities();
    
    GN_LOG_INFO("BossHealthBar reset to full health");
}



} // namespace GameCore