#pragma once

#include "../Components/GameComponents.h"
#include "../../Engine/Core/ECS.h"
#include "../../Engine/Core/GNLog.h"
#include <memory>
#include <string>

namespace GameCore {

class BossSystem;

/**
 * @brief Boss Health Bar System for iOS
 *
 * Displays a visual health bar for boss enemies with hurt effects.
 * Adapted from the old desktop implementation for iOS/FloppyTurd architecture.
 */
class BossHealthBar {
public:
    /**
     * @brief Constructor
     * @param bossSystem Reference to the boss system
     * @param bossName Display name for the boss
     */
    BossHealthBar(BossSystem* bossSystem, const char* bossName, Gnosis::ECS* ecsSystem);

    /**
     * @brief Destructor
     */
    ~BossHealthBar();

    /**
     * @brief Update the health bar
     * @param deltaTime Time since last update
     */
    void Update(float deltaTime);

    /**
     * @brief Set visibility of the health bar
     * @param visible Whether the health bar should be visible
     */
    void SetVisible(bool visible);

    /**
     * @brief Check if the health bar should be visible
     * @return true if boss is active and health bar should be shown
     */
    bool IsVisible() const;

private:
    /**
     * @brief Create the UI entities for the health bar
     */
    void CreateUIEntities();

    /**
     * @brief Update the UI entities based on current health
     */
    void UpdateUIEntities();

    /**
     * @brief Update health values and trigger hurt effects
     */
    void UpdateHealthValues();


    // Boss system reference
    BossSystem* m_bossSystem = nullptr;
    Gnosis::ECS* m_ecsSystem = nullptr;

    // UI entity IDs
    Entity m_backgroundEntity = 0;
    Entity m_healthFillEntity = 0;
    Entity m_hurtEffectEntity = 0;
    Entity m_bossNameEntity = 0;

    // Add Sprite pointers for bars
    Sprite* m_healthSprite = nullptr;
    Sprite* m_hurtSprite = nullptr;

    // Original dimensions from old code
    float originalWidth = 160.0f;
    float originalHeight = 32.0f;

    // Display properties
    const char* m_bossName;
    float m_currentHealthPercent = 1.0f;
    float m_shadowHealthPercent = 1.0f;  // For hurt effect fade
    float m_displayedHealthPercent = 1.0f;  // For smooth lerping
    float m_hurtFadeTimer = 0.0f;
    const float HURT_FADE_DURATION = 0.75f;
    bool m_manuallyHidden = false;  // Track if manually hidden (for pause menu)

    // Position and size - dynamic based on screen dimensions (matches old design)
    static constexpr float BAR_TOP_OFFSET = 0.02f;     // 2% from top (raised from 5%)
    static constexpr float NAME_OFFSET = 0.05f;        // 5% from top for name (raised from 8%)
    const int ORIGINAL_WIDTH = 160;                    // Original texture width
    const int ORIGINAL_HEIGHT = 32;                    // Original texture height

    // Texture IDs for boss bar components (matching old design)
    const char* FRAME_TEXTURE_ID = "BossBarFrame";
    const char* HEALTH_TEXTURE_ID = "BossBarHealth";
    const char* HURT_TEXTURE_ID = "BossBarHurt";

    // Text properties
    const float TEXT_SCALE = 2.0f;



    /**
     * @brief Render the health bar background
     */
    void RenderBackground();

    /**
     * @brief Render the health fill
     */
    void RenderHealthFill();

    /**
     * @brief Render the hurt effect (red overlay that fades)
     */
    void RenderHurtEffect();

    /**
     * @brief Render the boss name text
     */
    void RenderBossName();

    /**
     * @brief Render the health percentage text
     */
    void RenderHealthText();
};

} // namespace GameCore
