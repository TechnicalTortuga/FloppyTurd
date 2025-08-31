#pragma once

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Components/GameComponents.h"
#include <vector>
#include <string>
#include <map>

namespace GameCore {

    /**
     * Hat status enumeration
     */
    enum class HatStatus {
        LOCKED,
        UNLOCKED
    };

    /**
     * Hat data structure
     */
    struct HatData {
        std::string name;
        std::string iconPath;
        // Turdlet textures (idle, jump, shoot)
        std::string turdletIdlePath;
        std::string turdletJumpPath;
        std::string turdletShootPath;
        // Teenage textures (idle, jump, shoot)
        std::string teenageIdlePath;
        std::string teenageJumpPath;
        std::string teenageShootPath;
        // Big turd textures (idle, jump, shoot)
        std::string bigTurdIdlePath;
        std::string bigTurdJumpPath;
        std::string bigTurdShootPath;
        int cost;
        HatStatus status;

        // Default constructor
        HatData() = default;

        HatData(const std::string& name, const std::string& iconPath,
                const std::string& turdletIdle, const std::string& turdletJump, const std::string& turdletShoot,
                const std::string& teenageIdle, const std::string& teenageJump, const std::string& teenageShoot,
                const std::string& bigIdle, const std::string& bigJump, const std::string& bigShoot,
                int cost, HatStatus status = HatStatus::LOCKED)
            : name(name), iconPath(iconPath),
              turdletIdlePath(turdletIdle), turdletJumpPath(turdletJump), turdletShootPath(turdletShoot),
              teenageIdlePath(teenageIdle), teenageJumpPath(teenageJump), teenageShootPath(teenageShoot),
              bigTurdIdlePath(bigIdle), bigTurdJumpPath(bigJump), bigTurdShootPath(bigShoot),
              cost(cost), status(status) {}

        // Legacy constructor for backward compatibility (assumes idle == jump)
        HatData(const std::string& name, const std::string& iconPath,
                const std::string& turdletNormal, const std::string& turdletShoot,
                const std::string& teenageNormal, const std::string& teenageShoot,
                const std::string& bigNormal, const std::string& bigShoot,
                int cost, HatStatus status = HatStatus::LOCKED)
            : name(name), iconPath(iconPath),
              turdletIdlePath(turdletNormal), turdletJumpPath(turdletNormal), turdletShootPath(turdletShoot),
              teenageIdlePath(teenageNormal), teenageJumpPath(teenageNormal), teenageShootPath(teenageShoot),
              bigTurdIdlePath(bigNormal), bigTurdJumpPath(bigNormal), bigTurdShootPath(bigShoot),
              cost(cost), status(status) {}
    };

    /**
     * HatsSystem - Manages hat collection, unlocking, and equipping
     */
    class HatsSystem {
    public:
        HatsSystem(Gnosis::ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates);
        ~HatsSystem();

        // Core functionality
        void InitializeHats();
        void CreateHatsGrid(float centerX, float centerY, float gridWidth, float gridHeight);
        void UpdateHatsGrid(float deltaTime);
        void SelectHat(int hatIndex);
        bool BuySelectedHat(int playerCoins);
        void EquipSelectedHat();

        // Getters
        int GetEquippedHatIndex() const { return m_equippedHatIndex; }
        int GetSelectedHatIndex() const { return m_selectedHatIndex; }
        int GetHatCount() const { return static_cast<int>(m_hats.size()); }
        int GetSelectedHatCost() const;

        // Hat queries
        const HatData* GetHatData(int index) const;
        bool IsHatUnlocked(int index) const;
        const std::string& GetHatName(int index) const;

        // UI management
        void UpdateScreenInfo();
        void ShowUI();
        void HideUI();

        // Persistence
        void SaveHatStatus();
        void LoadHatStatus();

        // Entity access
        Gnosis::Entity GetHatIconEntity(int index) const;
        Gnosis::Entity GetHatFrameEntity(int index) const;
        Gnosis::Entity GetCostDisplayEntity() const { return m_costDisplayEntity; }
        Gnosis::Entity GetActionButtonEntity() const { return m_actionButtonEntity; }

    private:
        // Core system dependencies
        Gnosis::ECS* m_ecsCoordinator;
        const GameCore::PlatformDelegates& m_platformDelegates;
        GameCore::PlatformDelegates m_delegates;

        // Hat data and state
        std::vector<HatData> m_hats;
        int m_selectedHatIndex;
        int m_equippedHatIndex;

        // Grid layout constants
        static constexpr int GRID_ROWS = 5;
        static constexpr int GRID_COLS = 3;
        static constexpr int MAX_HATS = GRID_ROWS * GRID_COLS;

        // UI entities for hats grid
        std::vector<Gnosis::Entity> m_hatIconEntities;
        std::vector<Gnosis::Entity> m_hatFrameEntities;
        std::vector<Gnosis::Entity> m_lockedFrameEntities;
        std::vector<Gnosis::Entity> m_debugHitboxEntities;
        std::map<int, Gnosis::Entity> m_hatToLockedFrameMap; // Maps hat index to locked frame entity
        Gnosis::Entity m_costDisplayEntity;
        Gnosis::Entity m_actionButtonEntity;

        // Screen dimensions for responsive layout
        float m_screenWidth;
        float m_screenHeight;

        // Private methods
        Gnosis::Entity CreateHatIconEntity(int hatIndex, float x, float y, float size);
        Gnosis::Entity CreateHatFrameEntity(int hatIndex, float x, float y, float size);
        void UpdateCostDisplay();
        void CalculateGridLayout(float centerX, float centerY, float gridWidth, float gridHeight,
                                std::vector<Gnosis::GNVector2>& positions);
        void EnsureHatTextureLoaded(int hatIndex);
        void CreateDebugHitboxEntities(float frameSize);
        void UpdateDebugHitboxColor(Gnosis::Entity entity, bool isTriggered);
    };

} // namespace GameCore