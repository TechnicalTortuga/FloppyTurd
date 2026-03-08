#pragma once

#include "PlatformDelegates.h"
#include "../../FloppyTurd/Game/FloppyTurdGame.h"

namespace GameCore {

    /**
     * @brief Haptic Helper Functions
     * 
     * Convenient wrapper functions for common haptic feedback patterns.
     * These functions check for delegate validity AND vibration settings before calling,
     * making them safe to call without checks in game code.
     * 
     * All functions now respect the global vibration setting from GetGame()->GetVibrationsEnabled()
     * 
     * Usage:
     *   HapticHelpers::TriggerJump(delegates);
     *   HapticHelpers::TriggerCollision(delegates, impactForce);
     */
    namespace HapticHelpers {

        /**
         * @brief Check if haptics should be triggered (vibrations enabled globally)
         * @return true if vibrations are enabled, false otherwise
         */
        inline bool AreVibrationsEnabled() {
            // Get global game instance to check vibration settings
            auto* game = GameCore::GetGame();
            if (!game) {
                return true; // Default to enabled if game instance not available
            }
            return game->GetVibrationsEnabled();
        }

        // ============================================================================
        // Player Actions
        // ============================================================================

        /**
         * @brief Trigger haptic feedback for player jump
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerJump(const PlatformDelegates& delegates) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.7f);
            }
        }

        /**
         * @brief Trigger haptic feedback for player landing
         * @param delegates Platform delegates containing haptic interface
         * @param landingForce Force of landing (0.0 - 1.0)
         */
        inline void TriggerLanding(const PlatformDelegates& delegates, float landingForce = 1.0f) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                HapticStyle style = landingForce > 0.7f ? HapticStyle::MEDIUM : HapticStyle::LIGHT;
                delegates.haptic.triggerImpact(style, landingForce);
            }
        }

        /**
         * @brief Trigger haptic feedback for player dash/dodge
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerDash(const PlatformDelegates& delegates) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::RIGID, 0.8f);
            }
        }

        // ============================================================================
        // Collisions
        // ============================================================================

        /**
         * @brief Trigger haptic feedback for collision
         * @param delegates Platform delegates containing haptic interface
         * @param impactForce Force of impact (0.0 - 1.0), scales intensity and style
         */
        inline void TriggerCollision(const PlatformDelegates& delegates, float impactForce = 1.0f) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                HapticStyle style;
                if (impactForce > 0.8f) {
                    style = HapticStyle::HEAVY;
                } else if (impactForce > 0.4f) {
                    style = HapticStyle::MEDIUM;
                } else {
                    style = HapticStyle::LIGHT;
                }
                delegates.haptic.triggerImpact(style, impactForce);
            }
        }

        /**
         * @brief Trigger haptic feedback for light collision/graze
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerLightCollision(const PlatformDelegates& delegates) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.5f);
            }
        }

        /**
         * @brief Trigger haptic feedback for heavy collision/impact
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerHeavyCollision(const PlatformDelegates& delegates) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::HEAVY, 1.0f);
            }
        }

        // ============================================================================
        // Pickups & Rewards
        // ============================================================================

        /**
         * @brief Trigger haptic feedback for coin/currency pickup
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerCoinPickup(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.6f);
            }
        }

        /**
         * @brief Trigger haptic feedback for powerup pickup
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerPowerupPickup(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerNotification) {
                delegates.haptic.triggerNotification(HapticNotificationType::SUCCESS);
            }
        }

        /**
         * @brief Trigger haptic feedback for rare item pickup
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerRareItemPickup(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 1.0f);
            }
        }

        // ============================================================================
        // UI Interactions
        // ============================================================================

        /**
         * @brief Trigger haptic feedback for button press
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerButtonPress(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.8f);
            }
        }

        /**
         * @brief Trigger haptic feedback for menu navigation
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerMenuNavigation(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerSelection) {
                delegates.haptic.triggerSelection();
            }
        }

        /**
         * @brief Trigger haptic feedback for toggle switch
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerToggle(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 0.7f);
            }
        }

        /**
         * @brief Trigger haptic feedback for slider/value change
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerValueChange(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerSelection) {
                delegates.haptic.triggerSelection();
            }
        }

        // ============================================================================
        // Game State Changes
        // ============================================================================

        /**
         * @brief Trigger haptic feedback for level start
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerLevelStart(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 0.8f);
            }
        }

        /**
         * @brief Trigger haptic feedback for level complete
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerLevelComplete(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerNotification) {
                delegates.haptic.triggerNotification(HapticNotificationType::SUCCESS);
            }
        }

        /**
         * @brief Trigger haptic feedback for level unlock
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerLevelUnlock(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerPattern) {
                delegates.haptic.triggerPattern("level_unlock");
            }
        }

        /**
         * @brief Trigger haptic feedback for game over
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerGameOver(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerNotification) {
                delegates.haptic.triggerNotification(HapticNotificationType::ERROR);
            }
        }

        /**
         * @brief Trigger haptic feedback for pause
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerPause(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 0.6f);
            }
        }

        /**
         * @brief Trigger haptic feedback for resume
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerResume(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 0.6f);
            }
        }

        // ============================================================================
        // Boss Events
        // ============================================================================

        /**
         * @brief Trigger haptic feedback for boss appearance
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerBossAppearance(const PlatformDelegates& delegates) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::HEAVY, 1.0f);
            }
        }

        /**
         * @brief Trigger haptic feedback for boss attack
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerBossAttack(const PlatformDelegates& delegates) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::HEAVY, 0.9f);
            }
        }

        /**
         * @brief Trigger haptic feedback for boss damage taken
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerBossDamage(const PlatformDelegates& delegates) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::MEDIUM, 0.7f);
            }
        }

        /**
         * @brief Trigger haptic feedback for boss death (dramatic pattern)
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerBossDeath(const PlatformDelegates& delegates) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerPattern) {
                delegates.haptic.triggerPattern("boss_death");
            }
        }

        /**
         * @brief Trigger haptic feedback for boss phase transition
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerBossPhaseChange(const PlatformDelegates& delegates) {
            if (AreVibrationsEnabled() && delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::HEAVY, 0.8f);
            }
        }

        // ============================================================================
        // Shop & Cosmetics
        // ============================================================================

        /**
         * @brief Trigger haptic feedback for hat unlock
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerHatUnlock(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerPattern) {
                delegates.haptic.triggerPattern("hat_unlock");
            }
        }

        /**
         * @brief Trigger haptic feedback for hat equip
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerHatEquip(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::SOFT, 0.7f);
            }
        }

        /**
         * @brief Trigger haptic feedback for purchase success
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerPurchaseSuccess(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerNotification) {
                delegates.haptic.triggerNotification(HapticNotificationType::SUCCESS);
            }
        }

        /**
         * @brief Trigger haptic feedback for purchase failure
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerPurchaseFailure(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerNotification) {
                delegates.haptic.triggerNotification(HapticNotificationType::ERROR);
            }
        }

        // ============================================================================
        // Warnings & Alerts
        // ============================================================================

        /**
         * @brief Trigger haptic feedback for low health warning
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerLowHealth(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerNotification) {
                delegates.haptic.triggerNotification(HapticNotificationType::WARNING);
            }
        }

        /**
         * @brief Trigger haptic feedback for danger warning
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerDangerWarning(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerNotification) {
                delegates.haptic.triggerNotification(HapticNotificationType::WARNING);
            }
        }

        /**
         * @brief Trigger haptic feedback for countdown tick
         * @param delegates Platform delegates containing haptic interface
         */
        inline void TriggerCountdownTick(const PlatformDelegates& delegates) {
            if (delegates.haptic.triggerImpact) {
                delegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.6f);
            }
        }

        // ============================================================================
        // Preparation Helpers
        // ============================================================================

        /**
         * @brief Prepare haptic generator for upcoming boss battle
         * Reduces latency for first haptic trigger
         * @param delegates Platform delegates containing haptic interface
         */
        inline void PrepareBossBattle(const PlatformDelegates& delegates) {
            if (delegates.haptic.prepare) {
                delegates.haptic.prepare(HapticStyle::HEAVY);
            }
        }

        /**
         * @brief Prepare haptic generator for UI interactions
         * Reduces latency for first haptic trigger
         * @param delegates Platform delegates containing haptic interface
         */
        inline void PrepareUIInteraction(const PlatformDelegates& delegates) {
            if (delegates.haptic.prepare) {
                delegates.haptic.prepare(HapticStyle::LIGHT);
            }
        }

        /**
         * @brief Prepare haptic generator for gameplay
         * Reduces latency for first haptic trigger
         * @param delegates Platform delegates containing haptic interface
         */
        inline void PrepareGameplay(const PlatformDelegates& delegates) {
            if (delegates.haptic.prepare) {
                delegates.haptic.prepare(HapticStyle::MEDIUM);
            }
        }

    } // namespace HapticHelpers

} // namespace GameCore