/**
 * @file HapticIntegrationExample.cpp
 * @brief Example integrations of the haptic feedback system
 * 
 * This file demonstrates how to integrate haptic feedback into various
 * game systems using both the direct delegate API and the HapticHelpers.
 * 
 * ============================================================================
 * IMPORTANT: This is a DOCUMENTATION/EXAMPLE file only!
 * This file is NOT compiled into the game - it's for reference only.
 * Copy snippets from this file into your actual game systems.
 * ============================================================================
 * 
 * NOTE: This file intentionally does not compile on its own as it references
 * game-specific types and systems that may not exist yet. Use the code
 * patterns shown here as a guide for your own implementations.
 */

#include "Engine/Platform/PlatformDelegates.h"
#include "Engine/Platform/HapticHelpers.h"
#include "FloppyTurd/Components/GameComponents.h"

using namespace GameCore;

// ============================================================================
// Example 1: PlayerControllerSystem Integration
// ============================================================================

class PlayerControllerSystemExample {
private:
    PlatformDelegates& m_delegates;

public:
    PlayerControllerSystemExample(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    void handleJump() {
        // Player jumped - trigger light haptic feedback
        HapticHelpers::TriggerJump(m_delegates);
        
        // Or use direct API:
        // if (m_delegates.haptic.triggerImpact) {
        //     m_delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.7f);
        // }
    }

    void handleLanding(float velocity) {
        // Calculate landing force based on fall velocity
        float landingForce = std::min(std::abs(velocity) / 100.0f, 1.0f);
        
        // Trigger haptic feedback scaled by landing force
        HapticHelpers::TriggerLanding(m_delegates, landingForce);
        
        // Or use direct API with custom logic:
        // if (m_delegates.haptic.triggerImpact) {
        //     HapticStyle style = landingForce > 0.7f ? HapticStyle::HEAVY : HapticStyle::MEDIUM;
        //     m_delegates.haptic.triggerImpact(style, landingForce);
        // }
    }

    void handleDash() {
        // Player performed dash - trigger sharp haptic
        HapticHelpers::TriggerDash(m_delegates);
    }
};

// ============================================================================
// Example 2: ObstacleSystem Integration (Collision Detection)
// ============================================================================

class ObstacleSystemExample {
private:
    PlatformDelegates& m_delegates;

public:
    ObstacleSystemExample(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    void onCollisionWithObstacle(const ObstacleComponent& obstacle) {
        // Different haptics for different obstacle types
        switch (obstacle.obstacleType) {
            case ObstacleType::Spike:
                // Sharp, heavy impact for spikes
                HapticHelpers::TriggerHeavyCollision(m_delegates);
                break;
                
            case ObstacleType::Wall:
                // Medium impact for walls
                if (m_delegates.haptic.triggerImpact) {
                    m_delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 0.8f);
                }
                break;
                
            case ObstacleType::Cloud:
                // Soft, light impact for clouds
                if (m_delegates.haptic.triggerImpact) {
                    m_delegates.haptic.triggerImpact(HapticStyle::SOFT, 0.4f);
                }
                break;
                
            default:
                HapticHelpers::TriggerCollision(m_delegates, 0.6f);
                break;
        }
    }

    void onGrazeObstacle() {
        // Player barely missed obstacle - subtle haptic
        HapticHelpers::TriggerLightCollision(m_delegates);
    }

    void onCollisionWithEnemy(float damageDealt) {
        // Scale haptic based on damage dealt
        float impactForce = std::min(damageDealt / 50.0f, 1.0f);
        HapticHelpers::TriggerCollision(m_delegates, impactForce);
    }
};

// ============================================================================
// Example 3: PickupSystem Integration
// ============================================================================

class PickupSystemExample {
private:
    PlatformDelegates& m_delegates;

public:
    PickupSystemExample(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    void onCoinCollected(int coinValue) {
        // Simple haptic for coin pickup
        HapticHelpers::TriggerCoinPickup(m_delegates);
        
        // Optional: Scale intensity by coin value
        // float intensity = std::min(coinValue / 10.0f, 1.0f);
        // if (m_delegates.haptic.triggerImpact) {
        //     m_delegates.haptic.triggerImpact(HapticStyle::LIGHT, intensity);
        // }
    }

    void onPowerupCollected(PowerupType type) {
        // Different haptics for different powerups
        switch (type) {
            case PowerupType::Shield:
            case PowerupType::Invincibility:
                // Success notification for defensive powerups
                HapticHelpers::TriggerPowerupPickup(m_delegates);
                break;
                
            case PowerupType::SpeedBoost:
                // Sharp haptic for speed boost
                if (m_delegates.haptic.triggerImpact) {
                    m_delegates.haptic.triggerImpact(HapticStyle::RIGID, 0.9f);
                }
                break;
                
            case PowerupType::RareItem:
                // Special haptic for rare items
                HapticHelpers::TriggerRareItemPickup(m_delegates);
                break;
                
            default:
                HapticHelpers::TriggerPowerupPickup(m_delegates);
                break;
        }
    }
};

// ============================================================================
// Example 4: UISystem Integration (Menu Interactions)
// ============================================================================

class UISystemExample {
private:
    PlatformDelegates& m_delegates;
    int m_currentMenuIndex = 0;
    int m_previousMenuIndex = 0;

public:
    UISystemExample(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    void onButtonPressed() {
        // Button press haptic
        HapticHelpers::TriggerButtonPress(m_delegates);
    }

    void onMenuItemSelected(int index) {
        // Only trigger haptic if index actually changed
        if (index != m_previousMenuIndex) {
            HapticHelpers::TriggerMenuNavigation(m_delegates);
            m_previousMenuIndex = index;
        }
    }

    void onToggleSwitched(bool newValue) {
        // Toggle switch haptic
        HapticHelpers::TriggerToggle(m_delegates);
    }

    void onSliderValueChanged(float value) {
        // Only trigger selection feedback, not on every pixel
        // Use a threshold or step to avoid too many haptics
        static float lastHapticValue = 0.0f;
        if (std::abs(value - lastHapticValue) > 0.1f) {
            HapticHelpers::TriggerValueChange(m_delegates);
            lastHapticValue = value;
        }
    }

    void onPurchaseAttempt(bool success) {
        if (success) {
            HapticHelpers::TriggerPurchaseSuccess(m_delegates);
        } else {
            HapticHelpers::TriggerPurchaseFailure(m_delegates);
        }
    }

    void prepareForMenuOpen() {
        // Prepare haptic generator before menu opens for reduced latency
        HapticHelpers::PrepareUIInteraction(m_delegates);
    }
};

// ============================================================================
// Example 5: BossSystem Integration
// ============================================================================

class BossSystemExample {
private:
    PlatformDelegates& m_delegates;
    bool m_hasPreparedHaptics = false;

public:
    BossSystemExample(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    void onBossEncounterStart() {
        // Boss appears - dramatic entrance
        HapticHelpers::TriggerBossAppearance(m_delegates);
        
        // Prepare for heavy haptics during boss battle
        if (!m_hasPreparedHaptics) {
            HapticHelpers::PrepareBossBattle(m_delegates);
            m_hasPreparedHaptics = true;
        }
    }

    void onBossAttack(float attackPower) {
        // Scale haptic based on attack power
        if (attackPower > 50.0f) {
            HapticHelpers::TriggerBossAttack(m_delegates);
        } else {
            // Lighter haptic for weaker attacks
            if (m_delegates.haptic.triggerImpact) {
                m_delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 0.6f);
            }
        }
    }

    void onBossDamaged(float damageAmount) {
        // Haptic feedback when boss takes damage
        float intensity = std::min(damageAmount / 100.0f, 1.0f);
        if (m_delegates.haptic.triggerImpact) {
            m_delegates.haptic.triggerImpact(HapticStyle::MEDIUM, intensity);
        }
    }

    void onBossPhaseChange() {
        // Boss enters new phase - significant haptic
        HapticHelpers::TriggerBossPhaseChange(m_delegates);
    }

    void onBossDeath() {
        // Dramatic haptic sequence for boss death
        HapticHelpers::TriggerBossDeath(m_delegates);
        
        // Or trigger custom pattern:
        // if (m_delegates.haptic.triggerPattern) {
        //     m_delegates.haptic.triggerPattern("boss_death");
        // }
    }
};

// ============================================================================
// Example 6: GameplayState Integration (Game State Changes)
// ============================================================================

class GameplayStateExample {
private:
    PlatformDelegates& m_delegates;

public:
    GameplayStateExample(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    void onLevelStart() {
        // Level starting - prepare haptics for gameplay
        HapticHelpers::PrepareGameplay(m_delegates);
        HapticHelpers::TriggerLevelStart(m_delegates);
    }

    void onLevelComplete(int score, int targetScore) {
        if (score >= targetScore) {
            // Player achieved target score
            HapticHelpers::TriggerLevelComplete(m_delegates);
        } else {
            // Completed but didn't meet target
            if (m_delegates.haptic.triggerNotification) {
                m_delegates.haptic.triggerNotification(HapticNotificationType::WARNING);
            }
        }
    }

    void onLevelUnlocked() {
        // New level unlocked - special pattern
        HapticHelpers::TriggerLevelUnlock(m_delegates);
    }

    void onGameOver() {
        // Game over - error notification
        HapticHelpers::TriggerGameOver(m_delegates);
    }

    void onPauseToggle(bool isPaused) {
        if (isPaused) {
            HapticHelpers::TriggerPause(m_delegates);
        } else {
            HapticHelpers::TriggerResume(m_delegates);
        }
    }

    void onLowHealthWarning(float healthPercent) {
        // Trigger warning when health is low
        if (healthPercent < 0.25f) {
            HapticHelpers::TriggerLowHealth(m_delegates);
        }
    }

    void onCountdownTick(int secondsRemaining) {
        // Haptic feedback for countdown timer
        HapticHelpers::TriggerCountdownTick(m_delegates);
        
        // Intensify haptic as time runs out
        if (secondsRemaining <= 3) {
            if (m_delegates.haptic.triggerImpact) {
                float intensity = 1.0f - (secondsRemaining / 3.0f);
                m_delegates.haptic.triggerImpact(HapticStyle::MEDIUM, intensity);
            }
        }
    }
};

// ============================================================================
// Example 7: HatsSystem Integration (Cosmetics)
// ============================================================================

class HatsSystemExample {
private:
    PlatformDelegates& m_delegates;

public:
    HatsSystemExample(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    void onHatUnlocked(HatType hatType) {
        // Different haptics for different hat rarities
        switch (hatType) {
            case HatType::Crown:
            case HatType::WizardHat:
                // Rare hats - special pattern
                HapticHelpers::TriggerHatUnlock(m_delegates);
                break;
                
            case HatType::TopHat:
            case HatType::Beanie:
                // Common hats - success notification
                if (m_delegates.haptic.triggerNotification) {
                    m_delegates.haptic.triggerNotification(HapticNotificationType::SUCCESS);
                }
                break;
                
            default:
                HapticHelpers::TriggerHatUnlock(m_delegates);
                break;
        }
    }

    void onHatEquipped(HatType hatType) {
        // Soft haptic when equipping hat
        HapticHelpers::TriggerHatEquip(m_delegates);
    }

    void onShopItemBrowse() {
        // Selection haptic when browsing shop items
        HapticHelpers::TriggerMenuNavigation(m_delegates);
    }
};

// ============================================================================
// Example 8: Advanced Pattern - Combo System
// ============================================================================

class ComboSystemExample {
private:
    PlatformDelegates& m_delegates;
    int m_currentCombo = 0;

public:
    ComboSystemExample(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    void onComboIncrement() {
        m_currentCombo++;
        
        // Scale haptic intensity and style based on combo count
        if (m_currentCombo >= 10) {
            // High combo - heavy haptic
            if (m_delegates.haptic.triggerImpact) {
                m_delegates.haptic.triggerImpact(HapticStyle::HEAVY, 1.0f);
            }
        } else if (m_currentCombo >= 5) {
            // Medium combo - medium haptic
            if (m_delegates.haptic.triggerImpact) {
                m_delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 0.8f);
            }
        } else {
            // Low combo - light haptic
            if (m_delegates.haptic.triggerImpact) {
                float intensity = 0.5f + (m_currentCombo * 0.1f);
                m_delegates.haptic.triggerImpact(HapticStyle::LIGHT, intensity);
            }
        }
    }

    void onComboBreak() {
        // Combo broken - error notification if combo was significant
        if (m_currentCombo >= 5) {
            if (m_delegates.haptic.triggerNotification) {
                m_delegates.haptic.triggerNotification(HapticNotificationType::ERROR);
            }
        }
        m_currentCombo = 0;
    }
};

// ============================================================================
// Example 9: Event-Driven Haptic Manager
// ============================================================================

class HapticEventManager {
private:
    PlatformDelegates& m_delegates;
    float m_lastHapticTime = 0.0f;
    const float MIN_HAPTIC_INTERVAL = 0.05f; // 50ms minimum between haptics

public:
    HapticEventManager(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    // Throttle haptics to prevent overwhelming the player
    bool canTriggerHaptic(float currentTime) {
        if (currentTime - m_lastHapticTime >= MIN_HAPTIC_INTERVAL) {
            m_lastHapticTime = currentTime;
            return true;
        }
        return false;
    }

    void triggerHapticIfAllowed(float currentTime, HapticStyle style, float intensity = 1.0f) {
        if (canTriggerHaptic(currentTime)) {
            if (m_delegates.haptic.triggerImpact) {
                m_delegates.haptic.triggerImpact(style, intensity);
            }
        }
    }
};

// ============================================================================
// Example 10: Conditional Haptics Based on Settings
// ============================================================================

class ConfigurableHapticSystem {
private:
    PlatformDelegates& m_delegates;
    bool m_hapticsEnabled = true;
    float m_hapticIntensityMultiplier = 1.0f;

public:
    ConfigurableHapticSystem(PlatformDelegates& delegates) 
        : m_delegates(delegates) {}

    void setHapticsEnabled(bool enabled) {
        m_hapticsEnabled = enabled;
        
        // Use delegate to set enabled state in HapticManager
        if (m_delegates.haptic.setEnabled) {
            m_delegates.haptic.setEnabled(enabled);
        }
    }

    void setIntensityMultiplier(float multiplier) {
        // Allow player to adjust haptic intensity (0.0 - 1.0)
        m_hapticIntensityMultiplier = std::clamp(multiplier, 0.0f, 1.0f);
    }

    void triggerHaptic(HapticStyle style, float baseIntensity = 1.0f) {
        if (!m_hapticsEnabled) return;
        
        float finalIntensity = baseIntensity * m_hapticIntensityMultiplier;
        if (m_delegates.haptic.triggerImpact) {
            m_delegates.haptic.triggerImpact(style, finalIntensity);
        }
    }

    bool isHapticSupported() {
        // Check if device supports haptics
        if (m_delegates.haptic.isSupported) {
            return m_delegates.haptic.isSupported();
        }
        return false;
    }
};