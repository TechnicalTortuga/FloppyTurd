#include "BossHealthBar.h"
#include "BossSystem.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <cmath>
#include <chrono>

namespace GameCore {

BossHealthBar::BossHealthBar(BossSystem* bossSystem, const char* bossName, Gnosis::ECS* ecsSystem)
    : m_bossSystem(bossSystem)
    , m_bossName(bossName)
    , m_ecsSystem(ecsSystem)
    , m_currentHealthPercent(1.0f)
    , m_shadowHealthPercent(1.0f)
    , m_hurtFadeTimer(0.0f)
    , m_displayedHealthPercent(1.0f)
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

    // Create health sprite
    m_healthFillEntity = m_ecsSystem->CreateEntity();
    Sprite healthSprite("BossBarHealth", ORIGINAL_WIDTH * scale, ORIGINAL_HEIGHT * scale);
    healthSprite.visible = true;
    healthSprite.isAnimated = false;
    healthSprite.frameCount = 1;
    healthSprite.frameWidth = ORIGINAL_WIDTH;
    healthSprite.frameHeight = ORIGINAL_HEIGHT;
    healthSprite.layer = 13; // Behind frame (layer 15) and hurt effect (layer 14)
    m_ecsSystem->AddComponent<Sprite>(m_healthFillEntity, healthSprite);
    Transform healthTrans(Gnosis::GNVector2(barX, barY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f)); // Scale is baked into sprite size
    m_ecsSystem->AddComponent<Transform>(m_healthFillEntity, healthTrans);

    // Create hurt effect entity (white bar for damage flash)
    m_hurtEffectEntity = m_ecsSystem->CreateEntity();
    Sprite hurtSprite("BossBarHurt", ORIGINAL_WIDTH * scale, ORIGINAL_HEIGHT * scale);
    hurtSprite.visible = false;
    hurtSprite.frameWidth = ORIGINAL_WIDTH;
    hurtSprite.frameHeight = ORIGINAL_HEIGHT;
    hurtSprite.layer = 14; // Between health (layer 13) and frame (layer 15)
    m_ecsSystem->AddComponent<Sprite>(m_hurtEffectEntity, hurtSprite);
    // Offset hurt effect slightly to the right to prevent white edge showing behind frame
    float hurtOffsetX = 2.0f * scale; // Small offset to the right
    Transform hurtTrans(Gnosis::GNVector2(barX + hurtOffsetX, barY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f)); // Scale is baked into sprite size
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

    // Update hurt effect fade
    if (m_hurtFadeTimer > 0.0f) {
        m_hurtFadeTimer -= deltaTime;
        if (m_hurtFadeTimer < 0.0f) {
            m_hurtFadeTimer = 0.0f;
            m_shadowHealthPercent = m_currentHealthPercent;  // Reset shadow to match current
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

    // Update health sprite source rect
    if (m_healthFillEntity != 0) {
        Sprite* health = m_ecsSystem->GetComponent<Sprite>(m_healthFillEntity);
        if (health) {
            health->visible = shouldBeVisible && (m_displayedHealthPercent > 0.0f);
            // Update both source rect and rendered width
            health->sourceWidth = ORIGINAL_WIDTH * m_displayedHealthPercent;
            health->sourceHeight = ORIGINAL_HEIGHT;
            health->sourceX = 0;
            health->sourceY = 0;
            // CRITICAL: Also update sprite width so it renders at the correct size
            float scale = 8.0f; // Same scale used in CreateUIEntities
            health->width = (ORIGINAL_WIDTH * m_displayedHealthPercent) * scale;
        }
    }

    // Update damage effect (hurt flash) width based on shadow health
    if (m_hurtEffectEntity != 0) {
        Sprite* hurt = m_ecsSystem->GetComponent<Sprite>(m_hurtEffectEntity);
        if (hurt && m_hurtFadeTimer > 0.0f && shouldBeVisible && m_shadowHealthPercent > m_currentHealthPercent) {
            hurt->visible = true;
            float hurtPercent = m_shadowHealthPercent - m_currentHealthPercent;
            hurt->sourceWidth = ORIGINAL_WIDTH * hurtPercent;
            hurt->sourceHeight = ORIGINAL_HEIGHT;
            hurt->sourceX = ORIGINAL_WIDTH * m_currentHealthPercent; // Start after current health
            hurt->sourceY = 0;
            // CRITICAL: Also update sprite width for correct rendering
            float scale = 8.0f;
            hurt->width = (ORIGINAL_WIDTH * hurtPercent) * scale;
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
    
    // Update UI entities to show full health
    UpdateUIEntities();
    
    GN_LOG_INFO("BossHealthBar reset to full health");
}



} // namespace GameCore
