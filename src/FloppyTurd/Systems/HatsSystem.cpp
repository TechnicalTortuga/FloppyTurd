#include "HatsSystem.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Utility/Utils.h"
#include "../../Engine/Platform/HapticHelpers.h"
#include "../Game/FloppyTurdGame.h"
#include <algorithm>
#include <sstream>
#include <fstream>

namespace GameCore {

    HatsSystem::HatsSystem(Gnosis::ECS* ecsCoordinator, const PlatformDelegates& delegates)
        : m_ecsCoordinator(ecsCoordinator)
        , m_platformDelegates(delegates)
        , m_costDisplayEntity(0)
        , m_actionButtonEntity(0)
        , m_selectedHatIndex(-1)
        , m_equippedHatIndex(-1)
        , m_screenWidth(0)
        , m_screenHeight(0)
    {
        GN_LOG_INFO("HatsSystem created");
        InitializeHats();
        
        // Load equipped/selected hat from game save system at startup
        if (GameCore::GetGame()) {
            m_equippedHatIndex = GameCore::GetGame()->GetEquippedHatIndex();
            m_selectedHatIndex = GameCore::GetGame()->GetSelectedHatIndex();
            GN_LOG_INFO("🎩 HatsSystem initialized with equipped hat: " + std::to_string(m_equippedHatIndex) + 
                       ", selected hat: " + std::to_string(m_selectedHatIndex) + " from game save");
        } else {
            GN_LOG_WARN("🎩 HatsSystem: No game instance available at construction - using defaults");
        }
    }

    HatsSystem::~HatsSystem()
    {
        // Clean up UI entities
        for (auto entity : m_hatIconEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                m_ecsCoordinator->DestroyEntity(entity);
            }
        }
        for (auto entity : m_hatFrameEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                m_ecsCoordinator->DestroyEntity(entity);
            }
        }
        if (m_costDisplayEntity != 0 && m_ecsCoordinator) {
            m_ecsCoordinator->DestroyEntity(m_costDisplayEntity);
            m_costDisplayEntity = 0;
        }
        if (m_actionButtonEntity != 0 && m_ecsCoordinator) {
            m_ecsCoordinator->DestroyEntity(m_actionButtonEntity);
            m_actionButtonEntity = 0;
        }

        GN_LOG_INFO("HatsSystem destroyed");
    }

    void HatsSystem::InitializeHats()
    {
        GN_LOG_INFO("Initializing hats collection");

        // Based on the actual texture files found in Assets/hats directory
        // Using correct texture names with .png extensions
        // Pattern: idle uses first frame of jump texture, jump/shoot use full 6-frame animation
        // Cowboy hat has dedicated single-frame idle texture, others use jump texture with frameCount=1
        m_hats = {
            // Row 1 - Free/unlocked hats
            HatData("Cowboy Hat", "cowboyhat.png",
                   "cowboyhatturdlet.png", "cowboyhatturdletjump.png", "cowboyhatturdletshoot.png", // idle (single frame), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "cowboyhatbigturd.png", "cowboyhatbigturdjump.png", "cowboyhatbigturdshoot.png", // big idle, jump, shoot
                   0, HatStatus::UNLOCKED),

            HatData("Flower", "flowerhat.png",
                   "flowerhatturdletjump.png", "flowerhatturdletjump.png", "flowerhatturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "flowerhatbigturdjump.png", "flowerhatbigturdjump.png", "flowerhatbigturdshoot.png", // big idle, jump, shoot
                   0, HatStatus::UNLOCKED),

            HatData("Doorag", "dooraghat.png",
                   "dooragturdletjump.png", "dooragturdletjump.png", "dooragturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "dooragbigturdjump.png", "dooragbigturdjump.png", "dooragbigturdshoot.png", // big idle, jump, shoot
                   0, HatStatus::UNLOCKED),

            HatData("Ballcap", "ballcap.png",
                   "ballcapturdletjump.png", "ballcapturdletjump.png", "ballcapturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "ballcapbigturdjump.png", "ballcapbigturdjump.png", "ballcapbigturdshoot.png", // big idle, jump, shoot
                   0, HatStatus::UNLOCKED),

            HatData("Beret", "Beret.png",
                   "berethatturdletjump.png", "berethatturdletjump.png", "berethatturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "berethatbigturdjump.png", "berethatbigturdjump.png", "berethatbigturdshoot.png", // big idle, jump, shoot
                   50, HatStatus::LOCKED),

            // Row 2 - Paid hats
            HatData("Crown", "crown.png",
                   "crownhatturdletjump.png", "crownhatturdletjump.png", "crownhatturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "crownhatbigturdjump.png", "crownhatbigturdjump.png", "crownhatbigturdshoot.png", // big idle, jump, shoot
                   100, HatStatus::LOCKED),

            HatData("Top Hat", "tophat.png",
                   "tophatturdletjump.png", "tophatturdletjump.png", "tophatturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "tophatbigturdjump.png", "tophatbigturdjump.png", "tophatbigturdshoot.png", // big idle, jump, shoot
                   150, HatStatus::LOCKED),

            HatData("Samurai", "SamuraiHelmet.png",
                   "samuraiturdletjump.png", "samuraiturdletjump.png", "samuraiturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "samuraibigturdjump.png", "samuraibigturdjump.png", "samuraibigturdshoot.png", // big idle, jump, shoot
                   200, HatStatus::LOCKED),

            HatData("Spartan", "SpartanHelmet.png",
                   "spartanhatturdletjump.png", "spartanhatturdletjump.png", "spartanhatturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "spartanhatbigturdjump.png", "spartanhatbigturdjump.png", "spartanhatbigturdshoot.png", // big idle, jump, shoot
                   250, HatStatus::LOCKED),

            HatData("Poop Hat", "poophat.png",
                   "poophatturdletjump.png", "poophatturdletjump.png", "poophatturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "poophatbigturdjump.png", "poophatbigturdjump.png", "poophatbigturdshoot.png", // big idle, jump, shoot
                   300, HatStatus::LOCKED),

            // Row 3 - More premium hats
            HatData("Straw Hat", "strawhat.png",
                   "strawhatturdletjump.png", "strawhatturdletjump.png", "strawhatturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "strawhatbigturdjump.png", "strawhatbigturdjump.png", "strawhatbigturdshoot.png", // big idle, jump, shoot
                   350, HatStatus::LOCKED),

            HatData("Shell Hat", "shellhat.png",
                   "shellhatturdletjump.png", "shellhatturdletjump.png", "shellhatturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "shellhatbigturdjump.png", "shellhatbigturdjump.png", "shellhatbigturdshoot.png", // big idle, jump, shoot
                   400, HatStatus::LOCKED),

            HatData("Pinwheel", "PinwheelHat.png",
                   "pinwheelturdletjump.png", "pinwheelturdletjump.png", "pinwheelturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "pinwheelbigturdjump.png", "pinwheelbigturdjump.png", "pinwheelbigturdshoot.png", // big idle, jump, shoot
                   450, HatStatus::LOCKED),

            HatData("Ramses", "RamsesHat.png",
                   "ramsesturdletjump.png", "ramsesturdletjump.png", "ramsesturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "ramsesbigturdjump.png", "ramsesbigturdjump.png", "ramsesbigturdshoot.png", // big idle, jump, shoot
                   500, HatStatus::LOCKED),

            HatData("Ushanka", "ushanka.png",
                   "ushankaturdletjump.png", "ushankaturdletjump.png", "ushankaturdletshoot.png", // idle (first frame of jump), jump (6 frames), shoot (6 frames)
                   "", "", "", // Skip teenage form
                   "ushankabigturdjump.png", "ushankabigturdjump.png", "ushankabigturdshoot.png", // big idle, jump, shoot
                   600, HatStatus::LOCKED)
        };

        // Ensure we don't exceed the grid size
        if (m_hats.size() > MAX_HATS) {
            m_hats.resize(MAX_HATS);
        }

        // Fill remaining slots with placeholders if needed
        while (m_hats.size() < MAX_HATS) {
            m_hats.push_back(HatData("Coming Soon", "placeholder.png",
                                   "placeholder.png", "placeholder.png",
                                   "placeholder.png", "placeholder.png",
                                   "placeholder.png", "placeholder.png",
                                   999, HatStatus::LOCKED));
        }

        GN_LOG_INFO("Initialized " + std::to_string(m_hats.size()) + " hats");
    }

    void HatsSystem::CreateHatsGrid(float centerX, float centerY, float gridWidth, float gridHeight)
    {
        GN_LOG_INFO("HatsSystem: Creating hats grid UI at center (" + std::to_string(centerX) + ", " + std::to_string(centerY) + ")");
        GN_LOG_INFO("HatsSystem: ECS Coordinator valid: " + std::string(m_ecsCoordinator ? "YES" : "NO"));
        GN_LOG_INFO("HatsSystem: Number of hats to create: " + std::to_string(m_hats.size()));

        // Calculate grid positions
        std::vector<Gnosis::GNVector2> positions;
        CalculateGridLayout(centerX, centerY, gridWidth, gridHeight, positions);

        // Clear existing entities
        for (auto entity : m_hatIconEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                m_ecsCoordinator->DestroyEntity(entity);
            }
        }
        for (auto entity : m_hatFrameEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                m_ecsCoordinator->DestroyEntity(entity);
            }
        }
        m_hatIconEntities.clear();
        m_hatFrameEntities.clear();

        // Create grid entities
        float iconSize = std::min(gridWidth / GRID_COLS, gridHeight / GRID_ROWS) * 0.8f;
        GN_LOG_INFO("HatsSystem: Icon size calculated: " + std::to_string(iconSize));
        GN_LOG_INFO("HatsSystem: Positions calculated: " + std::to_string(positions.size()));

        for (int i = 0; i < m_hats.size() && i < MAX_HATS; ++i) {
            if (i >= positions.size()) {
                GN_LOG_WARN("HatsSystem: Breaking loop - i=" + std::to_string(i) + " >= positions.size=" + std::to_string(positions.size()));
                break;
            }

            float x = positions[i].x;
            float y = positions[i].y;
            GN_LOG_INFO("HatsSystem: Creating hat " + std::to_string(i) + " '" + m_hats[i].name + "' at position (" + std::to_string(x) + ", " + std::to_string(y) + ")");

            // Create frame entity
            auto frameEntity = CreateHatFrameEntity(i, x, y, iconSize);
            GN_LOG_INFO("HatsSystem: Created frame entity " + std::to_string(frameEntity) + " for hat '" + m_hats[i].name + "'");
            m_hatFrameEntities.push_back(frameEntity);

            // Create icon entity
            auto iconEntity = CreateHatIconEntity(i, x, y, iconSize);
            GN_LOG_INFO("HatsSystem: Created icon entity " + std::to_string(iconEntity) + " for hat '" + m_hats[i].name + "'");
            m_hatIconEntities.push_back(iconEntity);
        }

        // Create cost display entity ABOVE the grid
        if (m_costDisplayEntity == 0) {
            m_costDisplayEntity = m_ecsCoordinator->CreateEntity();
            // Position ABOVE the grid, not below it
            Transform costTransform(Gnosis::GNVector2(centerX, centerY - gridHeight * 0.6f), 0.0f, Gnosis::GNVector2(6.0f, 6.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_costDisplayEntity, costTransform);

            UIElement costElement;
            costElement.buttonText = "Select a hat to see cost";
            costElement.fontSize = 32.0f;
            costElement.textColor = Gnosis::GNColor(255, 255, 255, 255);
            costElement.centerTextHorizontally = true;
            costElement.centerTextVertically = true;
            costElement.visible = false; // Initially hidden like Systems tab
            costElement.isEnabled = true;
            costElement.textLayer = 86; // Same layer range as Systems tab
            m_ecsCoordinator->AddComponent<UIElement>(m_costDisplayEntity, costElement);

            Sprite costSprite;
            costSprite.layer = 86; // Same layer range as Systems tab
            costSprite.visible = false; // Initially hidden like Systems tab
            m_ecsCoordinator->AddComponent<Sprite>(m_costDisplayEntity, costSprite);
        }

        // Create single action button (Buy/Equip) - positioned like Main Menu button
        if (m_actionButtonEntity == 0) {
            m_actionButtonEntity = m_ecsCoordinator->CreateEntity();

            // Update screen info to ensure we have current dimensions
            UpdateScreenInfo();

            // Position like Main Menu button: center horizontally, 75% down screen
            // Use EXACT same values as Main Menu button for perfect positioning
            float buttonCenterX = m_screenWidth * 0.5f; // Center horizontally on screen
            float buttonCenterY = m_screenHeight * 0.75f; // Position at 75% down screen

            // Use larger scale for main menu size (10x scaling for 900x160 button)
            float buttonScale = 10.0f;
            float buttonWidth = 90.0f * buttonScale;  // 900 pixels (like working buttons)
            float buttonHeight = 16.0f * buttonScale; // 160 pixels (like working buttons)

            // Use positioning helper to center button (EXACT same pattern as main menu buttons)
            Gnosis::GNVector2 buttonPosition = GameCore::CenterObjectAtPosition(buttonCenterX, buttonCenterY, buttonWidth, buttonHeight);
            float buttonX = buttonPosition.x;
            float buttonY = buttonPosition.y;

            GN_LOG_INFO("HatsSystem: Action button positioning - screen: " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight) +
                       ", center: (" + std::to_string(buttonCenterX) + ", " + std::to_string(buttonCenterY) + ")" +
                       ", final position: (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")" +
                       ", size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight));

            Transform actionButtonTransform(Gnosis::GNVector2(buttonX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
            m_ecsCoordinator->AddComponent<Transform>(m_actionButtonEntity, actionButtonTransform);

            // Create sprite using same button texture
            Sprite actionButtonSprite("FloppyButtonBlue.png", 90, 16);
            actionButtonSprite.layer = 91; // Above everything else
            actionButtonSprite.visible = false; // Initially hidden, will be shown when hat is selected
            m_ecsCoordinator->AddComponent<Sprite>(m_actionButtonEntity, actionButtonSprite);

            // Create UI element with centered text and larger font
            UIElement actionButtonUI("", "FloppyButtonBlue", "FloppyButtonBlueHover");
            actionButtonUI.fontSize = 62.0f; // Increased font size like Main Menu
            actionButtonUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
            actionButtonUI.centerTextHorizontally = true;
            actionButtonUI.centerTextVertically = true;
            actionButtonUI.textLayer = 91;
            actionButtonUI.visible = false; // Initially hidden
            actionButtonUI.isEnabled = true; // Make sure button is enabled for interaction
            actionButtonUI.normalTextureId = "FloppyButtonBlue"; // Add texture ID
            m_ecsCoordinator->AddComponent<UIElement>(m_actionButtonEntity, actionButtonUI);

            GN_LOG_INFO("HatsSystem: Created action button at (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) + ") with scale " + std::to_string(buttonScale));
        }

        GN_LOG_INFO("Created hats grid with " + std::to_string(m_hatIconEntities.size()) + " icons");

        // Load saved hat status
        LoadHatStatus();

        
    }

    void HatsSystem::UpdateHatsGrid(float deltaTime)
    {
        // Update cost display and buttons based on selected hat
        UpdateCostDisplay();

        // Handle hat selection (this would be called from touch input in the actual implementation)
        // For now, we'll just ensure the selected hat is highlighted
    }

    void HatsSystem::SelectHat(int hatIndex)
    {
        if (hatIndex < 0 || hatIndex >= m_hats.size()) return;

        m_selectedHatIndex = hatIndex;
        GN_LOG_INFO("Selected hat: " + m_hats[hatIndex].name);

        UpdateCostDisplay();
    }


    bool HatsSystem::BuySelectedHat(int playerCoins)
    {
        if (m_selectedHatIndex < 0 || m_selectedHatIndex >= m_hats.size()) return false;

        auto& hat = m_hats[m_selectedHatIndex];
        if (hat.status == HatStatus::UNLOCKED) return false; // Already unlocked

        if (playerCoins >= hat.cost) {
            hat.status = HatStatus::UNLOCKED;
            GN_LOG_INFO("Bought hat: " + hat.name);

            // Trigger haptic feedback for hat unlock
            HapticHelpers::TriggerHatUnlock(m_platformDelegates);

            // Hide the locked frame overlay for this hat
            auto lockedFrameIt = m_hatToLockedFrameMap.find(m_selectedHatIndex);
            if (lockedFrameIt != m_hatToLockedFrameMap.end()) {
                auto lockedFrameEntity = lockedFrameIt->second;
                if (lockedFrameEntity != 0 && m_ecsCoordinator) {
                    // Hide both sprite and UI element components
                    auto sprite = m_ecsCoordinator->GetComponent<Sprite>(lockedFrameEntity);
                    if (sprite) {
                        sprite->visible = false;
                        GN_LOG_INFO("HatsSystem: Hidden locked frame sprite for purchased hat '" + hat.name + "' (index " + std::to_string(m_selectedHatIndex) + ")");
                    }
                    auto uiElement = m_ecsCoordinator->GetComponent<UIElement>(lockedFrameEntity);
                    if (uiElement) {
                        uiElement->visible = false;
                        GN_LOG_INFO("HatsSystem: Hidden locked frame UI element for purchased hat '" + hat.name + "' (index " + std::to_string(m_selectedHatIndex) + ")");
                    }
                }
            } else {
                GN_LOG_WARN("HatsSystem: No locked frame entity found for purchased hat '" + hat.name + "' (index " + std::to_string(m_selectedHatIndex) + ")");
            }

            UpdateCostDisplay();

            // Save hat status after successful purchase
            SaveHatStatus();

            return true;
        }

        return false;
    }

    void HatsSystem::EquipSelectedHat()
    {
        if (m_selectedHatIndex < 0 || m_selectedHatIndex >= m_hats.size()) {
            GN_LOG_WARN("Cannot equip hat - invalid selectedHatIndex: " + std::to_string(m_selectedHatIndex));
            return;
        }

        auto& hat = m_hats[m_selectedHatIndex];
        if (hat.status != HatStatus::UNLOCKED) {
            GN_LOG_WARN("Cannot equip hat '" + hat.name + "' - not unlocked (status: " + std::to_string(static_cast<int>(hat.status)) + ")");
            return;
        }

        int oldEquippedIndex = m_equippedHatIndex;
        m_equippedHatIndex = m_selectedHatIndex;

        GN_LOG_INFO("Equipped hat: '" + hat.name + "' (index: " + std::to_string(m_selectedHatIndex) +
                   ", old equipped: " + std::to_string(oldEquippedIndex) + ")");
        GN_LOG_INFO("Hat textures - idle: '" + hat.turdletIdlePath + "', jump: '" + hat.turdletJumpPath + "', shoot: '" + hat.turdletShootPath + "'");

        // Update game's customization data (this will also save to JSON)
        if (GameCore::GetGame()) {
            GameCore::GetGame()->SetEquippedHatIndex(m_equippedHatIndex);
            GN_LOG_INFO("🎩 Synced equipped hat to game save system");
        }
        
        // Save hat status after equipping (legacy system)
        SaveHatStatus();

        // Note: We don't update PlayerComponent here as the texture switching
        // is handled automatically by PlayerControllerSystem checking the equipped hat
        UpdateCostDisplay();
    }

    const HatData* HatsSystem::GetHatData(int index) const
    {
        if (index < 0 || index >= m_hats.size()) return nullptr;
        return &m_hats[index];
    }

    bool HatsSystem::IsHatUnlocked(int index) const
    {
        if (index < 0 || index >= m_hats.size()) return false;
        return m_hats[index].status == HatStatus::UNLOCKED;
    }

    const std::string& HatsSystem::GetHatName(int index) const
    {
        static const std::string emptyString = "";
        if (index < 0 || index >= m_hats.size()) return emptyString;
        return m_hats[index].name;
    }

    int HatsSystem::GetSelectedHatCost() const
    {
        if (m_selectedHatIndex < 0 || m_selectedHatIndex >= m_hats.size()) return 0;
        return m_hats[m_selectedHatIndex].cost;
    }

    Gnosis::Entity HatsSystem::GetHatIconEntity(int index) const
    {
        if (index < 0 || index >= m_hatIconEntities.size()) return 0;
        return m_hatIconEntities[index];
    }

    Gnosis::Entity HatsSystem::GetHatFrameEntity(int index) const
    {
        if (index < 0 || index >= m_hatFrameEntities.size()) return 0;
        return m_hatFrameEntities[index];
    }

    void HatsSystem::UpdateScreenInfo()
    {
        // Use ECS screen dimensions (set by RenderSystem, accessible by all systems)
        if (m_ecsCoordinator) {
            m_screenWidth = m_ecsCoordinator->GetScreenWidth();
            m_screenHeight = m_ecsCoordinator->GetScreenHeight();

            GN_LOG_INFO("HatsSystem: Screen dimensions updated from ECS - " +
                       std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight));
            return;
        }

        // Final fallback to hardcoded defaults (should rarely be used)
        m_screenWidth = 1179.0f;
        m_screenHeight = 2556.0f;
        GN_LOG_WARN("HatsSystem: No ECS coordinator available, using hardcoded defaults");
    }

    Gnosis::Entity HatsSystem::CreateHatIconEntity(int hatIndex, float x, float y, float size)
    {
        if (hatIndex < 0 || hatIndex >= m_hats.size()) {
            GN_LOG_ERROR("HatsSystem: Invalid hatIndex " + std::to_string(hatIndex) + " for hats.size=" + std::to_string(m_hats.size()));
            return 0;
        }

        GN_LOG_INFO("HatsSystem: Creating icon entity for hat '" + m_hats[hatIndex].name + "' with texture '" + m_hats[hatIndex].iconPath + "'");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("HatsSystem: ECS Coordinator is null in CreateHatIconEntity!");
            return 0;
        }
        
        GN_LOG_INFO("HatsSystem: About to create entity for hatIndex=" + std::to_string(hatIndex));
        auto entity = m_ecsCoordinator->CreateEntity();
        if (entity == 0) {
            GN_LOG_ERROR("HatsSystem: Failed to create ECS entity for hat icon!");
            return 0;
        }
        GN_LOG_INFO("HatsSystem: Created ECS entity " + std::to_string(entity) + " for hat icon");

        // Use UI transform for screen-space positioning with 6x scale
        GN_LOG_INFO("HatsSystem: About to add Transform component to entity " + std::to_string(entity));
        // Fix icon centering - icons appear in top-left quadrant of 32x32 texture
        // Offset the centering to account for icon content not being centered in texture
        Gnosis::GNVector2 baseCenterPoint = CenterObjectAtPosition(x, y, size, size);
        // Adjust for icon content being in top-left quadrant by offsetting by half the icon size
        float iconOffset = size * 0.25f; // Quarter offset to move from top-left to center
        Gnosis::GNVector2 centerPoint(baseCenterPoint.x + iconOffset, baseCenterPoint.y + iconOffset);
        GN_LOG_INFO("HatsSystem: Icon centering - input position: (" + std::to_string(x) + ", " + std::to_string(y) +
                   "), size: " + std::to_string(size) + ", baseCenterPoint: (" + std::to_string(baseCenterPoint.x) + ", " + std::to_string(baseCenterPoint.y) +
                   "), adjusted centerPoint: (" + std::to_string(centerPoint.x) + ", " + std::to_string(centerPoint.y) + ")");
        Transform transform(centerPoint, 0.0f, Gnosis::GNVector2(6.0f, 6.0f));
        m_ecsCoordinator->AddComponent<Transform>(entity, transform);
        GN_LOG_INFO("HatsSystem: Added Transform component to entity " + std::to_string(entity));

        // Create sprite for the hat icon FIRST (following working Systems tab pattern)
        GN_LOG_INFO("HatsSystem: About to add Sprite component to entity " + std::to_string(entity));
        Sprite sprite(m_hats[hatIndex].iconPath, size, size);
        sprite.layer = 91; // ABOVE Systems tab (90) to avoid layer conflicts
        sprite.visible = false; // Start invisible, will be shown when tab is activated
        m_ecsCoordinator->AddComponent<Sprite>(entity, sprite);
        GN_LOG_INFO("HatsSystem: Added Sprite component to entity " + std::to_string(entity) + " with textureId='" + sprite.textureId + "'");

        // Add UI element component to make this screen-space (following working Systems tab pattern)
        GN_LOG_INFO("HatsSystem: About to add UIElement component to entity " + std::to_string(entity));
        UIElement uiElement;
        uiElement.visible = false; // Start invisible, will be shown when tab is activated
        uiElement.isEnabled = false; // Not interactive
        uiElement.textLayer = 91; // ABOVE Systems tab (90) to avoid layer conflicts
        uiElement.normalTextureId = m_hats[hatIndex].iconPath; // Add normal texture ID for hat icon
        m_ecsCoordinator->AddComponent<UIElement>(entity, uiElement);
        GN_LOG_INFO("HatsSystem: Added UIElement component to entity " + std::to_string(entity) + " with textLayer=" + std::to_string(uiElement.textLayer));

        GN_LOG_INFO("HatsSystem: Returning entity " + std::to_string(entity) + " from CreateHatIconEntity");
        return entity;
    }

    Gnosis::Entity HatsSystem::CreateHatFrameEntity(int hatIndex, float x, float y, float size)
    {
        if (hatIndex < 0 || hatIndex >= m_hats.size()) return 0;

        auto entity = m_ecsCoordinator->CreateEntity();

        // Use UI transform for screen-space positioning with 6x scale
        // Use CenterObjectAtPosition for proper centering of frame
        float frameWidth = size * 1.2f;
        float frameHeight = size * 1.2f;
        Gnosis::GNVector2 framePosition = CenterObjectAtPosition(x, y, frameWidth, frameHeight);
        GN_LOG_INFO("HatsSystem: Frame centering - input position: (" + std::to_string(x) + ", " + std::to_string(y) +
                   "), size: " + std::to_string(size) + ", frameWidth: " + std::to_string(frameWidth) +
                   ", calculated framePosition: (" + std::to_string(framePosition.x) + ", " + std::to_string(framePosition.y) + ")");
        Transform transform(framePosition, 0.0f, Gnosis::GNVector2(6.0f, 6.0f));
        m_ecsCoordinator->AddComponent<Transform>(entity, transform);

        // Determine frame texture based on hat status and selection
        // Every hat gets a base frame - locked hats get lock overlay on top
        std::string frameTextureId;
        if (hatIndex == m_selectedHatIndex) {
            frameTextureId = "HatFrameHover.png"; // Use hover texture for selected hat
        } else if (hatIndex == m_equippedHatIndex) {
            frameTextureId = "HatFrameHover.png"; // Use hover texture for equipped hat
        } else {
            frameTextureId = "HatFrame.png"; // Use normal frame texture for all others
        }
        GN_LOG_INFO("HatsSystem: Creating hat frame entity " + std::to_string(entity) + " for hat '" + m_hats[hatIndex].name + "' with frame texture '" + frameTextureId + "'");
        Sprite frameSprite(frameTextureId, size, size);
        frameSprite.layer = 90; // Behind icons (91) but above Systems tab (89 and below)
        frameSprite.visible = false; // Start invisible, will be shown when tab is activated
        m_ecsCoordinator->AddComponent<Sprite>(entity, frameSprite);
        GN_LOG_INFO("HatsSystem: Added Sprite component to hat frame entity " + std::to_string(entity) + " with texture '" + frameTextureId + "'");

        // Create UI element for the frame (following working Systems tab pattern)
        UIElement frameElement;
        frameElement.buttonText = ""; // No text for frames
        frameElement.fontSize = 0.0f;
        frameElement.visible = false; // Start invisible, will be shown when tab is activated
        frameElement.isEnabled = true;
        frameElement.textLayer = 90; // Match sprite layer
        frameElement.normalTextureId = frameTextureId; // Add normal texture ID for frame
        m_ecsCoordinator->AddComponent<UIElement>(entity, frameElement);

        // Create locked overlay frame if hat is locked
        if (m_hats[hatIndex].status == HatStatus::LOCKED) {
            GN_LOG_INFO("HatsSystem: Creating locked overlay frame for locked hat '" + m_hats[hatIndex].name + "'");

            // Create locked frame entity at same position with higher layer
            // Use CenterObjectAtPosition for proper centering of locked overlay
            Gnosis::Entity lockedFrameEntity = m_ecsCoordinator->CreateEntity();
            float lockedFrameWidth = size;
            float lockedFrameHeight = size;
            Gnosis::GNVector2 lockedFramePosition = CenterObjectAtPosition(x, y, lockedFrameWidth, lockedFrameHeight);
            Transform lockedFrameTransform(lockedFramePosition, 0.0f, Gnosis::GNVector2(6.0f, 6.0f));
            m_ecsCoordinator->AddComponent<Transform>(lockedFrameEntity, lockedFrameTransform);

            Sprite lockedFrameSprite("HatFrameLocked.png", size, size);
            lockedFrameSprite.layer = 93; // ABOVE regular frames (92) so lock overlay is on top
            lockedFrameSprite.visible = false; // Start invisible, will be shown when tab is activated
            m_ecsCoordinator->AddComponent<Sprite>(lockedFrameEntity, lockedFrameSprite);

            UIElement lockedFrameElement;
            lockedFrameElement.buttonText = ""; // No text for frames
            lockedFrameElement.fontSize = 0.0f;
            lockedFrameElement.visible = false; // Start invisible, will be shown when tab is activated
            lockedFrameElement.isEnabled = true;
            lockedFrameElement.textLayer = 92; // Match sprite layer
            lockedFrameElement.normalTextureId = "HatFrameLocked.png";
            m_ecsCoordinator->AddComponent<UIElement>(lockedFrameEntity, lockedFrameElement);

            // Store the locked frame entity for later management
            m_lockedFrameEntities.push_back(lockedFrameEntity);
            m_hatToLockedFrameMap[hatIndex] = lockedFrameEntity; // Map hat index to locked frame entity
            GN_LOG_INFO("HatsSystem: Created locked frame overlay entity " + std::to_string(lockedFrameEntity) + " for hat '" + m_hats[hatIndex].name + "' (index " + std::to_string(hatIndex) + ")");
        }

        return entity;
    }

    void HatsSystem::UpdateCostDisplay()
    {
        if (m_selectedHatIndex < 0 || m_selectedHatIndex >= m_hats.size()) {
            // No hat selected
            if (m_costDisplayEntity != 0 && m_ecsCoordinator) {
                UIElement* costElement = m_ecsCoordinator->GetComponent<UIElement>(m_costDisplayEntity);
                if (costElement) {
                    costElement->buttonText = "Select a hat to see cost";
                }
            }
            // Show action button with "Nothing Selected" text
            if (m_actionButtonEntity != 0 && m_ecsCoordinator) {
                UIElement* actionElement = m_ecsCoordinator->GetComponent<UIElement>(m_actionButtonEntity);
                if (actionElement) {
                    actionElement->visible = true;
                    actionElement->buttonText = "Nothing Selected";
                    actionElement->textColor = Gnosis::GNColor(150, 150, 150, 255); // Gray for no selection
                }
                Sprite* actionSprite = m_ecsCoordinator->GetComponent<Sprite>(m_actionButtonEntity);
                if (actionSprite) actionSprite->visible = true;
            }
            return;
        }

        auto& hat = m_hats[m_selectedHatIndex];

        // Update cost display
        if (m_costDisplayEntity != 0 && m_ecsCoordinator) {
            UIElement* costElement = m_ecsCoordinator->GetComponent<UIElement>(m_costDisplayEntity);
            if (costElement) {
                if (hat.status == HatStatus::UNLOCKED) {
                    costElement->buttonText = hat.name + " - Unlocked";
                    costElement->textColor = Gnosis::GNColor(100, 255, 100, 255); // Green for unlocked
                } else {
                    costElement->buttonText = hat.name + " - Cost: " + std::to_string(hat.cost) + " coins";
                    costElement->textColor = Gnosis::GNColor(255, 255, 100, 255); // Yellow for locked
                }
            }
        }

        // Update single action button
        if (m_actionButtonEntity != 0 && m_ecsCoordinator) {
            UIElement* actionElement = m_ecsCoordinator->GetComponent<UIElement>(m_actionButtonEntity);
            Sprite* actionSprite = m_ecsCoordinator->GetComponent<Sprite>(m_actionButtonEntity);

            if (actionElement && actionSprite) {
                if (hat.status == HatStatus::LOCKED) {
                    // Show buy button for locked hats
                    actionElement->buttonText = "Buy";
                    actionElement->textColor = Gnosis::GNColor(255, 255, 255, 255);
                    actionElement->visible = true;
                    actionSprite->visible = true;
                } else {
                    // Show equip/unequip button for unlocked hats
                    if (m_selectedHatIndex == m_equippedHatIndex) {
                        actionElement->buttonText = "Equipped";
                        actionElement->textColor = Gnosis::GNColor(100, 100, 255, 255); // Blue for equipped
                    } else {
                        actionElement->buttonText = "Equip";
                        actionElement->textColor = Gnosis::GNColor(255, 255, 255, 255); // White for available
                    }
                    actionElement->visible = true;
                    actionSprite->visible = true;
                }
            }
        }
    }

    void HatsSystem::CalculateGridLayout(float centerX, float centerY, float gridWidth, float gridHeight,
                                        std::vector<Gnosis::GNVector2>& positions)
    {
        positions.clear();

        GN_LOG_INFO("HatsSystem: CalculateGridLayout - center: (" + std::to_string(centerX) + ", " + std::to_string(centerY) + ")" +
                   ", grid: " + std::to_string(gridWidth) + "x" + std::to_string(gridHeight) +
                   ", GRID_COLS=" + std::to_string(GRID_COLS) + ", GRID_ROWS=" + std::to_string(GRID_ROWS));

        float cellWidth = gridWidth / GRID_COLS;
        float cellHeight = gridHeight / GRID_ROWS;
        float startX = centerX - gridWidth * 0.5f + cellWidth * 0.5f;
        float startY = centerY - gridHeight * 0.5f + cellHeight * 0.5f;

        GN_LOG_INFO("HatsSystem: Cell dimensions: " + std::to_string(cellWidth) + "x" + std::to_string(cellHeight) +
                   ", start position: (" + std::to_string(startX) + ", " + std::to_string(startY) + ")");

        for (int row = 0; row < GRID_ROWS; ++row) {
            for (int col = 0; col < GRID_COLS; ++col) {
                float x = startX + col * cellWidth;
                float y = startY + row * cellHeight;
                positions.push_back(Gnosis::GNVector2(x, y));
                GN_LOG_INFO("HatsSystem: Position [" + std::to_string(row) + "," + std::to_string(col) + "] = (" + std::to_string(x) + ", " + std::to_string(y) + ")");
            }
        }

        GN_LOG_INFO("HatsSystem: Generated " + std::to_string(positions.size()) + " grid positions");
    }

    void HatsSystem::CreateDebugHitboxEntities(float frameSize)
    {
        GN_LOG_INFO("HatsSystem: Creating debug hitbox entities");

        // Clear existing debug entities first
        for (auto entity : m_debugHitboxEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                m_ecsCoordinator->DestroyEntity(entity);
            }
        }
        m_debugHitboxEntities.clear();

        // Create debug hitbox for action button
        if (m_actionButtonEntity != 0) {
            auto debugEntity = m_ecsCoordinator->CreateEntity();

            // Copy transform from action button
            auto originalTransform = m_ecsCoordinator->GetComponent<Transform>(m_actionButtonEntity);
            if (originalTransform) {
                Transform debugTransform = *originalTransform;
                m_ecsCoordinator->AddComponent<Transform>(debugEntity, debugTransform);

                GN_LOG_INFO("HatsSystem: Action button debug - position: (" + std::to_string(originalTransform->position.x) + ", " + std::to_string(originalTransform->position.y) + ")" +
                           ", scale: (" + std::to_string(originalTransform->scale.x) + ", " + std::to_string(originalTransform->scale.y) + ")");
            }

            // Create debug rectangle using UIShape - match button sprite size
            // Button sprite is 90x16, so debug rectangle should match this
            float debugWidth = 90.0f;   // Match Sprite width
            float debugHeight = 16.0f;  // Match Sprite height

            // Create red rectangle using UIShape (no texture needed!)
            UIShape debugShape(UIShapeType::Rectangle, debugWidth, debugHeight,
                              Gnosis::GNColor(255, 0, 0, 128), 121, false); // Semi-transparent red, invisible
            m_ecsCoordinator->AddComponent<UIShape>(debugEntity, debugShape);

            // Verify visibility is set correctly
            auto verifyShape = m_ecsCoordinator->GetComponent<UIShape>(debugEntity);
            GN_LOG_INFO("HatsSystem: Created debug hitbox for action button: " + std::to_string(debugEntity) +
                       " size: " + std::to_string(debugWidth) + "x" + std::to_string(debugHeight) +
                       ", visibility set to: " + std::to_string(debugShape.visible) +
                       ", verified visibility: " + (verifyShape ? std::to_string(verifyShape->visible) : "null"));

            m_debugHitboxEntities.push_back(debugEntity);
        }

        // Create debug hitboxes for all hat frames
        for (size_t i = 0; i < m_hatFrameEntities.size(); ++i) {
            auto frameEntity = m_hatFrameEntities[i];
            if (frameEntity != 0) {
                auto debugEntity = m_ecsCoordinator->CreateEntity();

                // Copy transform from frame
                auto originalTransform = m_ecsCoordinator->GetComponent<Transform>(frameEntity);
                if (originalTransform) {
                    Transform debugTransform = *originalTransform;
                    m_ecsCoordinator->AddComponent<Transform>(debugEntity, debugTransform);

                    GN_LOG_INFO("HatsSystem: Frame " + std::to_string(i) + " debug - position: (" + std::to_string(originalTransform->position.x) + ", " + std::to_string(originalTransform->position.y) + ")" +
                               ", scale: (" + std::to_string(originalTransform->scale.x) + ", " + std::to_string(originalTransform->scale.y) + ")");
                }

                            // Create debug rectangle using UIShape for frames (match frame sprite size)
            // Frame sprite size is frameSize (32x32), no 1.2x scaling
            float frameSpriteSize = frameSize; // This matches the Sprite width/height
            float debugWidth = frameSpriteSize;
            float debugHeight = frameSpriteSize;

            GN_LOG_INFO("HatsSystem: Frame debug rectangle size calculation: frameSize=" + std::to_string(frameSize) +
                       ", frameSpriteSize=" + std::to_string(frameSpriteSize));

                            // Create red rectangle using UIShape (no texture needed!)
            UIShape debugShape(UIShapeType::Rectangle, debugWidth, debugHeight,
                              Gnosis::GNColor(255, 0, 0, 128), 120, false); // Semi-transparent red, invisible
            m_ecsCoordinator->AddComponent<UIShape>(debugEntity, debugShape);

                // Verify visibility is set correctly
                auto verifyShape = m_ecsCoordinator->GetComponent<UIShape>(debugEntity);
                GN_LOG_INFO("HatsSystem: Created debug hitbox for frame " + std::to_string(i) + ": " + std::to_string(debugEntity) +
                           " size: " + std::to_string(debugWidth) + "x" + std::to_string(debugHeight) +
                           ", visibility set to: " + std::to_string(debugShape.visible) +
                           ", verified visibility: " + (verifyShape ? std::to_string(verifyShape->visible) : "null"));

                m_debugHitboxEntities.push_back(debugEntity);
            }
        }

        GN_LOG_INFO("HatsSystem: Created " + std::to_string(m_debugHitboxEntities.size()) + " debug hitbox entities");
    }

    void HatsSystem::UpdateDebugHitboxColor(Gnosis::Entity entity, bool isTriggered)
    {
        if (entity != 0 && m_ecsCoordinator) {
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
            if (sprite) {
                if (isTriggered) {
                    sprite->textureId = "DebugHitboxGreen.png"; // Green when triggered
                } else {
                    sprite->textureId = "DebugHitboxRed.png"; // Red when not triggered
                }
            }
        }
    }

    void HatsSystem::SaveHatStatus()
    {
        GN_LOG_INFO("HatsSystem: Saving hat status...");

        // Create a simple text file for hat status persistence
        std::string hatStatusString;
        for (size_t i = 0; i < m_hats.size(); ++i) {
            hatStatusString += (m_hats[i].status == HatStatus::UNLOCKED) ? "1" : "0";
            if (i < m_hats.size() - 1) {
                hatStatusString += ",";
            }
        }

        // Save to a simple text file with equipped and selected indices on separate lines
        std::ofstream file("hat_status.txt");
        if (file.is_open()) {
            file << hatStatusString << "\n";
            file << m_equippedHatIndex << "\n";
            file << m_selectedHatIndex << "\n";
            file.close();
            GN_LOG_INFO("HatsSystem: Saved hat status to file: " + hatStatusString + 
                       " | Equipped: " + std::to_string(m_equippedHatIndex) + 
                       " | Selected: " + std::to_string(m_selectedHatIndex));
        } else {
            GN_LOG_WARN("HatsSystem: Failed to save hat status to file");
        }
    }

    void HatsSystem::LoadHatStatus()
    {
        GN_LOG_INFO("HatsSystem: Loading hat status...");

        // Load from the simple text file
        std::ifstream file("hat_status.txt");
        if (!file.is_open()) {
            GN_LOG_INFO("HatsSystem: No saved hat status file found, using defaults");
            return;
        }

        std::string statusStr;
        std::getline(file, statusStr);
        
        // Load equipped index (default to -1 if not present)
        std::string equippedStr;
        if (std::getline(file, equippedStr)) {
            m_equippedHatIndex = std::stoi(equippedStr);
        }
        
        // Load selected index (default to -1 if not present)
        std::string selectedStr;
        if (std::getline(file, selectedStr)) {
            m_selectedHatIndex = std::stoi(selectedStr);
        }
        
        file.close();

        if (statusStr.empty()) {
            GN_LOG_INFO("HatsSystem: Empty hat status file, using defaults");
            return;
        }

        GN_LOG_INFO("HatsSystem: Loaded hat status string: " + statusStr + 
                   " | Equipped: " + std::to_string(m_equippedHatIndex) + 
                   " | Selected: " + std::to_string(m_selectedHatIndex));
        
        // Sync with game's customization data (game save already loaded in constructor, but check legacy file)
        if (GameCore::GetGame()) {
            // If legacy file has different data than game save, sync it to game save
            if (m_equippedHatIndex >= 0 && m_equippedHatIndex != GameCore::GetGame()->GetEquippedHatIndex()) {
                GameCore::GetGame()->SetEquippedHatIndex(m_equippedHatIndex);
                GN_LOG_INFO("🎩 Synced legacy hat file data to game save - Equipped: " + std::to_string(m_equippedHatIndex));
            }
            if (m_selectedHatIndex >= 0 && m_selectedHatIndex != GameCore::GetGame()->GetSelectedHatIndex()) {
                GameCore::GetGame()->SetSelectedHatIndex(m_selectedHatIndex);
                GN_LOG_INFO("🎩 Synced legacy hat file data to game save - Selected: " + std::to_string(m_selectedHatIndex));
            }
        }

        // Parse the string and update hat status
        std::stringstream ss(statusStr);
        std::string token;
        size_t index = 0;

        while (std::getline(ss, token, ',') && index < m_hats.size()) {
            if (!token.empty()) {
                int status = std::stoi(token);
                HatStatus newStatus = (status == 1) ? HatStatus::UNLOCKED : HatStatus::LOCKED;

                if (m_hats[index].status != newStatus) {
                    GN_LOG_INFO("HatsSystem: Loading hat " + std::to_string(index) + " '" + m_hats[index].name +
                               "' status: " + (newStatus == HatStatus::UNLOCKED ? "UNLOCKED" : "LOCKED"));
                    m_hats[index].status = newStatus;
                }
            }
            index++;
        }

        GN_LOG_INFO("HatsSystem: Hat status loaded successfully");
    }

    void HatsSystem::ShowUI()
    {
        // Show debug hitbox entities (UIShape components) - DISABLED: Keep them hidden
        // for (auto entity : m_debugHitboxEntities) {
        //     if (entity != 0 && m_ecsCoordinator) {
        //         UIShape* shape = m_ecsCoordinator->GetComponent<UIShape>(entity);
        //         if (shape) shape->visible = true;
        //         GN_LOG_INFO("HatsSystem: ShowUI - showed debug hitbox entity " + std::to_string(entity));
        //     }
        // }
        GN_LOG_INFO("HatsSystem: ShowUI called - vector sizes: icons=" + std::to_string(m_hatIconEntities.size()) + ", frames=" + std::to_string(m_hatFrameEntities.size()));

        // Verify ECS coordinator is valid
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("HatsSystem: ECS Coordinator is null!");
            return;
        }

        // DEBUG: Log all entity IDs we're trying to show
        GN_LOG_INFO("HatsSystem: Attempting to show these entities:");
        for (auto entity : m_hatIconEntities) {
            GN_LOG_INFO("  Icon entity: " + std::to_string(entity));
        }
        for (auto entity : m_hatFrameEntities) {
            GN_LOG_INFO("  Frame entity: " + std::to_string(entity));
        }

        // Show all hat icon entities - simplified without try-catch to prevent early loop exit
        GN_LOG_INFO("HatsSystem: Starting to process " + std::to_string(m_hatIconEntities.size()) + " icon entities");
        for (size_t i = 0; i < m_hatIconEntities.size(); ++i) {
            Gnosis::Entity entity = m_hatIconEntities[i];
            GN_LOG_INFO("HatsSystem: Processing icon entity " + std::to_string(i) + "/" + std::to_string(m_hatIconEntities.size()) + " with ID " + std::to_string(entity));
            
            if (entity == 0) {
                GN_LOG_WARN("HatsSystem: Skipping null icon entity at index " + std::to_string(i));
                continue;
            }
            
            if (!m_ecsCoordinator) {
                GN_LOG_ERROR("HatsSystem: ECS Coordinator became null during processing");
                break;
            }

            // Show sprite component
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
            if (sprite) {
                sprite->visible = true;
                GN_LOG_INFO("HatsSystem: Set icon sprite visible=true, layer=" + std::to_string(sprite->layer) + " for entity " + std::to_string(entity));
            } else {
                GN_LOG_ERROR("HatsSystem: Could not get Sprite component for icon entity " + std::to_string(entity) + " at index " + std::to_string(i));
            }

            // Show UI element component
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
            if (uiElement) {
                uiElement->visible = true;
                GN_LOG_INFO("HatsSystem: Set icon UIElement visible=true, textLayer=" + std::to_string(uiElement->textLayer) + " for entity " + std::to_string(entity));
            } else {
                GN_LOG_ERROR("HatsSystem: Could not get UIElement component for icon entity " + std::to_string(entity) + " at index " + std::to_string(i));
            }
            
            GN_LOG_INFO("HatsSystem: Completed processing icon entity " + std::to_string(entity) + " at index " + std::to_string(i));
        }
        GN_LOG_INFO("HatsSystem: Finished processing all " + std::to_string(m_hatIconEntities.size()) + " icon entities");

        // Show all hat frame entities - simplified without try-catch to prevent early loop exit
        GN_LOG_INFO("HatsSystem: Starting to process " + std::to_string(m_hatFrameEntities.size()) + " frame entities");
        for (size_t i = 0; i < m_hatFrameEntities.size(); ++i) {
            Gnosis::Entity entity = m_hatFrameEntities[i];
            GN_LOG_INFO("HatsSystem: Processing frame entity " + std::to_string(i) + "/" + std::to_string(m_hatFrameEntities.size()) + " with ID " + std::to_string(entity));
            
            if (entity == 0) {
                GN_LOG_WARN("HatsSystem: Skipping null frame entity at index " + std::to_string(i));
                continue;
            }
            
            if (!m_ecsCoordinator) {
                GN_LOG_ERROR("HatsSystem: ECS Coordinator became null during processing");
                break;
            }

            // Show sprite component
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
            if (sprite) {
                sprite->visible = true;
                GN_LOG_INFO("HatsSystem: Set frame sprite visible=true, layer=" + std::to_string(sprite->layer) + " for entity " + std::to_string(entity));
            } else {
                GN_LOG_ERROR("HatsSystem: Could not get Sprite component for frame entity " + std::to_string(entity) + " at index " + std::to_string(i));
            }

            // Show UI element component
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
            if (uiElement) {
                uiElement->visible = true;
                GN_LOG_INFO("HatsSystem: Set frame UIElement visible=true, textLayer=" + std::to_string(uiElement->textLayer) + " for entity " + std::to_string(entity));
            } else {
                GN_LOG_ERROR("HatsSystem: Could not get UIElement component for frame entity " + std::to_string(entity) + " at index " + std::to_string(i));
            }
            
            GN_LOG_INFO("HatsSystem: Completed processing frame entity " + std::to_string(entity) + " at index " + std::to_string(i));
        }
        GN_LOG_INFO("HatsSystem: Finished processing all " + std::to_string(m_hatFrameEntities.size()) + " frame entities");

        // Show all locked frame entities
        GN_LOG_INFO("HatsSystem: Starting to process " + std::to_string(m_lockedFrameEntities.size()) + " locked frame entities");
        for (size_t i = 0; i < m_lockedFrameEntities.size(); ++i) {
            Gnosis::Entity entity = m_lockedFrameEntities[i];
            GN_LOG_INFO("HatsSystem: Processing locked frame entity " + std::to_string(i) + "/" + std::to_string(m_lockedFrameEntities.size()) + " with ID " + std::to_string(entity));
            
            if (entity == 0) {
                GN_LOG_WARN("HatsSystem: Skipping null locked frame entity at index " + std::to_string(i));
                continue;
            }
            
            if (!m_ecsCoordinator) {
                GN_LOG_ERROR("HatsSystem: ECS Coordinator became null during processing");
                break;
            }

            // Show sprite component
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
            if (sprite) {
                sprite->visible = true;
                GN_LOG_INFO("HatsSystem: Set locked frame sprite visible=true, layer=" + std::to_string(sprite->layer) + " for entity " + std::to_string(entity));
            } else {
                GN_LOG_ERROR("HatsSystem: Could not get Sprite component for locked frame entity " + std::to_string(entity) + " at index " + std::to_string(i));
            }

            // Show UI element component
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
            if (uiElement) {
                uiElement->visible = true;
                GN_LOG_INFO("HatsSystem: Set locked frame UIElement visible=true, textLayer=" + std::to_string(uiElement->textLayer) + " for entity " + std::to_string(entity));
            } else {
                GN_LOG_ERROR("HatsSystem: Could not get UIElement component for locked frame entity " + std::to_string(entity) + " at index " + std::to_string(i));
            }
            
            GN_LOG_INFO("HatsSystem: Completed processing locked frame entity " + std::to_string(entity) + " at index " + std::to_string(i));
        }
        GN_LOG_INFO("HatsSystem: Finished processing all " + std::to_string(m_lockedFrameEntities.size()) + " locked frame entities");

        // Show cost display
        if (m_costDisplayEntity != 0 && m_ecsCoordinator) {
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_costDisplayEntity);
            if (sprite) sprite->visible = true;
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_costDisplayEntity);
            if (uiElement) uiElement->visible = true;
        }

        // Show action button if it exists
        if (m_actionButtonEntity != 0 && m_ecsCoordinator) {
            Sprite* buttonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_actionButtonEntity);
            if (buttonSprite) buttonSprite->visible = true;
            UIElement* buttonUI = m_ecsCoordinator->GetComponent<UIElement>(m_actionButtonEntity);
            if (buttonUI) buttonUI->visible = true;
            GN_LOG_INFO("HatsSystem: ShowUI - showed action button entity " + std::to_string(m_actionButtonEntity));
        }

        // Create debug hitbox visualization entities
        if (m_debugHitboxEntities.empty()) {
            // We need to calculate the iconSize here to pass to CreateDebugHitboxEntities
            // This matches the calculation in CreateHatsGrid
            float screenWidth = m_screenWidth > 0 ? m_screenWidth : 1179.0f;
            float screenHeight = m_screenHeight > 0 ? m_screenHeight : 2556.0f;
            float gridWidth = screenWidth * 0.7f;
            float gridHeight = screenHeight * 0.4f;
            float iconSize = std::min(gridWidth / GRID_COLS, gridHeight / GRID_ROWS) * 0.8f;
            CreateDebugHitboxEntities(iconSize);
        }

        GN_LOG_INFO("HatsSystem: Created hats tab content successfully");
    }

    void HatsSystem::HideUI()
    {
        GN_LOG_INFO("HatsSystem: Hiding UI elements");

        // Hide all hat icon entities
        for (auto entity : m_hatIconEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                if (sprite) {
                    sprite->visible = false;
                }
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElement) {
                    uiElement->visible = false;
                }
            }
        }

        // Hide all hat frame entities
        for (auto entity : m_hatFrameEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                if (sprite) {
                    sprite->visible = false;
                }
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElement) {
                    uiElement->visible = false;
                }
            }
        }

        // Hide all debug hitbox entities
        for (auto entity : m_debugHitboxEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                UIShape* uiShape = m_ecsCoordinator->GetComponent<UIShape>(entity);
                if (uiShape) {
                    uiShape->visible = false;
                    GN_LOG_INFO("HatsSystem: HideUI - hid debug hitbox entity " + std::to_string(entity) +
                               ", visibility set to: " + std::to_string(uiShape->visible));
                }
            }
        }

        // Hide all locked frame entities
        for (auto entity : m_lockedFrameEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                if (sprite) {
                    sprite->visible = false;
                }
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElement) {
                    uiElement->visible = false;
                }
            }
        }

        // Hide cost display
        if (m_costDisplayEntity != 0 && m_ecsCoordinator) {
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_costDisplayEntity);
            if (sprite) {
                sprite->visible = false;
            }
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_costDisplayEntity);
            if (uiElement) {
                uiElement->visible = false;
            }
        }

        // Hide action button
        if (m_actionButtonEntity != 0 && m_ecsCoordinator) {
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_actionButtonEntity);
            if (sprite) {
                sprite->visible = false;
            }
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_actionButtonEntity);
            if (uiElement) {
                uiElement->visible = false;
            }
        }

        // Hide debug hitbox entities (UIShape components)
        for (auto entity : m_debugHitboxEntities) {
            if (entity != 0 && m_ecsCoordinator) {
                UIShape* shape = m_ecsCoordinator->GetComponent<UIShape>(entity);
                if (shape) shape->visible = false;
            }
        }
    }

} // namespace GameCore
