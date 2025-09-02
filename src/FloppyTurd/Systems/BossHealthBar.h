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
    Entity m_healthTextEntity = 0;

    // Display properties
    const char* m_bossName;
    float m_currentHealthPercent = 1.0f;
    float m_shadowHealthPercent = 1.0f;  // For hurt effect fade
    float m_hurtFadeTimer = 0.0f;
    const float HURT_FADE_DURATION = 0.75f;

    // Position and size (iPhone 16 portrait coordinates)
    const float BAR_WIDTH = 600.0f;
    const float BAR_HEIGHT = 40.0f;
    const float BAR_X = 589.5f;  // Center of screen
    const float BAR_Y = 200.0f;  // Near top of screen
    const float NAME_Y = BAR_Y - 60.0f;  // Above the bar

    // Colors
    const float HEALTH_COLOR[4] = {0.2f, 0.8f, 0.2f, 1.0f};      // Green
    const float HURT_COLOR[4] = {0.8f, 0.2f, 0.2f, 1.0f};        // Red
    const float BACKGROUND_COLOR[4] = {0.3f, 0.3f, 0.3f, 0.8f};  // Dark gray
    const float BORDER_COLOR[4] = {0.8f, 0.8f, 0.8f, 1.0f};      // Light gray

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
