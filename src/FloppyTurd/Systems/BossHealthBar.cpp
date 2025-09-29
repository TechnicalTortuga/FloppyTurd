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

    // Create frame texture entity (background)
    m_backgroundEntity = m_ecsSystem->CreateEntity();
    Transform frameTransform(Gnosis::GNVector2(barX, barY), 0.0f, Gnosis::GNVector2(scale, scale));
    m_ecsSystem->AddComponent<Transform>(m_backgroundEntity, frameTransform);

    UIElement frameElement("", "");
    frameElement.normalTextureId = FRAME_TEXTURE_ID;
    frameElement.visible = false; // Initially hidden
    frameElement.textLayer = 15;
    m_ecsSystem->AddComponent<UIElement>(m_backgroundEntity, frameElement);

    // Create health fill texture entity
    m_healthFillEntity = m_ecsSystem->CreateEntity();
    Transform healthTransform(Gnosis::GNVector2(barX, barY), 0.0f, Gnosis::GNVector2(scale, scale));
    m_ecsSystem->AddComponent<Transform>(m_healthFillEntity, healthTransform);

    UIElement healthElement("", "");
    healthElement.normalTextureId = "BossBarHealth"; // Pre-colored red texture
    healthElement.visible = false; // Initially hidden
    healthElement.textLayer = 16;
    m_ecsSystem->AddComponent<UIElement>(m_healthFillEntity, healthElement);

    // Create hurt effect texture entity
    m_hurtEffectEntity = m_ecsSystem->CreateEntity();
    Transform hurtTransform(Gnosis::GNVector2(barX, barY), 0.0f, Gnosis::GNVector2(scale, scale));
    m_ecsSystem->AddComponent<Transform>(m_hurtEffectEntity, hurtTransform);

    UIElement hurtElement("", "");
    hurtElement.normalTextureId = "BossBarHurt"; // Pre-colored white texture
    hurtElement.visible = false; // Initially hidden
    hurtElement.textLayer = 17;
    m_ecsSystem->AddComponent<UIElement>(m_hurtEffectEntity, hurtElement);

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

    // Set visibility for all UI entities
    if (m_backgroundEntity != 0) {
        UIShape* bgShape = m_ecsSystem->GetComponent<UIShape>(m_backgroundEntity);
        if (bgShape) bgShape->visible = visible;
    }

    if (m_healthFillEntity != 0) {
        UIShape* fillShape = m_ecsSystem->GetComponent<UIShape>(m_healthFillEntity);
        if (fillShape) fillShape->visible = visible;
    }

    if (m_hurtEffectEntity != 0) {
        UIShape* hurtShape = m_ecsSystem->GetComponent<UIShape>(m_hurtEffectEntity);
        if (hurtShape) hurtShape->visible = visible;
    }

    if (m_bossNameEntity != 0) {
        UIElement* nameElement = m_ecsSystem->GetComponent<UIElement>(m_bossNameEntity);
        if (nameElement) nameElement->visible = visible;
    }

    // Health percentage text removed as requested
}

void BossHealthBar::UpdateUIEntities() {
    if (!m_ecsSystem) return;

    bool shouldBeVisible = IsVisible();

    // Update background visibility (always visible when boss bar is shown)
    if (m_backgroundEntity != 0) {
        UIElement* frameElement = m_ecsSystem->GetComponent<UIElement>(m_backgroundEntity);
        if (frameElement) frameElement->visible = shouldBeVisible;
    }

    // Update health fill - BossBarHealth texture is already red
    if (m_healthFillEntity != 0) {
        UIElement* healthElement = m_ecsSystem->GetComponent<UIElement>(m_healthFillEntity);
        if (healthElement) {
            healthElement->visible = shouldBeVisible && (m_currentHealthPercent > 0.0f);
        }
    }

    // Update damage effect - BossBarHurt texture is already white
    if (m_hurtEffectEntity != 0) {
        UIElement* hurtElement = m_ecsSystem->GetComponent<UIElement>(m_hurtEffectEntity);
        if (hurtElement) {
            if (m_hurtFadeTimer > 0.0f && shouldBeVisible && m_shadowHealthPercent > m_currentHealthPercent) {
                hurtElement->visible = true;
            } else {
                hurtElement->visible = false;
            }
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

    m_currentHealthPercent = newHealthPercent;
}



} // namespace GameCore
