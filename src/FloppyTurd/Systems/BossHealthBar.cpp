#include "BossHealthBar.h"
#include "BossSystem.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include <cmath>

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
        if (m_healthTextEntity != 0) m_ecsSystem->DestroyEntity(m_healthTextEntity);
    }
    GN_LOG_INFO("BossHealthBar destroyed");
}

void BossHealthBar::CreateUIEntities() {
    if (!m_ecsSystem) return;

    // Create background rectangle entity
    m_backgroundEntity = m_ecsSystem->CreateEntity();
    Transform bgTransform(Gnosis::GNVector2(BAR_X - BAR_WIDTH/2.0f, BAR_Y), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_backgroundEntity, bgTransform);

    UIShape bgShape(UIShapeType::Rectangle, BAR_WIDTH, BAR_HEIGHT, Gnosis::GNColor(40, 40, 70, 230), 15);
    bgShape.visible = false; // Initially hidden
    m_ecsSystem->AddComponent<UIShape>(m_backgroundEntity, bgShape);

    // Create health fill entity
    m_healthFillEntity = m_ecsSystem->CreateEntity();
    Transform fillTransform(Gnosis::GNVector2(BAR_X - BAR_WIDTH/2.0f, BAR_Y), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_healthFillEntity, fillTransform);

    UIShape fillShape(UIShapeType::Rectangle, BAR_WIDTH, BAR_HEIGHT, Gnosis::GNColor(20, 128, 20, 255), 16);
    fillShape.visible = false; // Initially hidden
    m_ecsSystem->AddComponent<UIShape>(m_healthFillEntity, fillShape);

    // Create hurt effect entity
    m_hurtEffectEntity = m_ecsSystem->CreateEntity();
    Transform hurtTransform(Gnosis::GNVector2(BAR_X - BAR_WIDTH/2.0f, BAR_Y), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_hurtEffectEntity, hurtTransform);

    UIShape hurtShape(UIShapeType::Rectangle, BAR_WIDTH, BAR_HEIGHT, Gnosis::GNColor(128, 20, 20, 128), 17);
    hurtShape.visible = false; // Initially hidden
    m_ecsSystem->AddComponent<UIShape>(m_hurtEffectEntity, hurtShape);

    // Create boss name text entity
    m_bossNameEntity = m_ecsSystem->CreateEntity();
    Transform nameTransform(Gnosis::GNVector2(BAR_X, NAME_Y), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_bossNameEntity, nameTransform);

    UIElement nameElement(m_bossName, "");
    nameElement.fontSize = TEXT_SCALE;
    nameElement.textColor = Gnosis::GNColor(255, 255, 255, 255);
    nameElement.centerTextHorizontally = true;
    nameElement.centerTextVertically = true;
    nameElement.textLayer = 18;
    nameElement.visible = false; // Initially hidden
    m_ecsSystem->AddComponent<UIElement>(m_bossNameEntity, nameElement);

    // Create health text entity
    m_healthTextEntity = m_ecsSystem->CreateEntity();
    Transform textTransform(Gnosis::GNVector2(BAR_X, BAR_Y + BAR_HEIGHT + 20.0f), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    m_ecsSystem->AddComponent<Transform>(m_healthTextEntity, textTransform);

    UIElement textElement("100%", "");
    textElement.fontSize = TEXT_SCALE * 0.8f;
    textElement.textColor = Gnosis::GNColor(255, 255, 255, 255);
    textElement.centerTextHorizontally = true;
    textElement.centerTextVertically = true;
    textElement.textLayer = 18;
    textElement.visible = false; // Initially hidden
    m_ecsSystem->AddComponent<UIElement>(m_healthTextEntity, textElement);

    GN_LOG_INFO("BossHealthBar UI entities created");
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

    if (m_healthTextEntity != 0) {
        UIElement* textElement = m_ecsSystem->GetComponent<UIElement>(m_healthTextEntity);
        if (textElement) textElement->visible = visible;
    }
}

void BossHealthBar::UpdateUIEntities() {
    if (!m_ecsSystem) return;

    bool shouldBeVisible = IsVisible();

    // Update health fill width
    if (m_healthFillEntity != 0) {
        UIShape* fillShape = m_ecsSystem->GetComponent<UIShape>(m_healthFillEntity);
        if (fillShape) {
            fillShape->width = BAR_WIDTH * m_currentHealthPercent;
            fillShape->visible = shouldBeVisible;
        }
    }

    // Update hurt effect
    if (m_hurtEffectEntity != 0) {
        UIShape* hurtShape = m_ecsSystem->GetComponent<UIShape>(m_hurtEffectEntity);
        if (hurtShape) {
            if (m_hurtFadeTimer > 0.0f && shouldBeVisible) {
                hurtShape->visible = true;
                hurtShape->width = BAR_WIDTH * m_shadowHealthPercent;
                float fadeAlpha = (m_hurtFadeTimer / HURT_FADE_DURATION) * 128.0f;  // Max 128 alpha
                hurtShape->color.a = static_cast<unsigned char>(fadeAlpha);
            } else {
                hurtShape->visible = false;
            }
        }
    }

    // Update background visibility
    if (m_backgroundEntity != 0) {
        UIShape* bgShape = m_ecsSystem->GetComponent<UIShape>(m_backgroundEntity);
        if (bgShape) bgShape->visible = shouldBeVisible;
    }

    // Update boss name visibility
    if (m_bossNameEntity != 0) {
        UIElement* nameElement = m_ecsSystem->GetComponent<UIElement>(m_bossNameEntity);
        if (nameElement) nameElement->visible = shouldBeVisible;
    }

    // Update health text
    if (m_healthTextEntity != 0) {
        UIElement* textElement = m_ecsSystem->GetComponent<UIElement>(m_healthTextEntity);
        if (textElement) {
            int healthPercentInt = static_cast<int>(m_currentHealthPercent * 100.0f);
            textElement->buttonText = std::to_string(healthPercentInt) + "%";
            textElement->visible = shouldBeVisible;
        }
    }
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
