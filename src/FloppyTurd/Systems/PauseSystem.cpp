#include "PauseSystem.h"
#include "../../Engine/Utility/Utils.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include "../Game/FloppyTurdGame.h"
#include <algorithm>
#include <set>

namespace GameCore {

    PauseSystem::PauseSystem(Gnosis::ECS* ecsCoordinator, const PlatformDelegates& delegates, GameplayState* gameplayState)
        : m_ecsCoordinator(ecsCoordinator)
        , m_platformDelegates(delegates)
        , m_gameplayState(gameplayState)
        , m_isVisible(false)
        , m_isCreated(false)
        , m_currentTab(PauseMenuTab::SYSTEM)
        , m_screenWidth(1179.0f)
        , m_screenHeight(2556.0f)
        , m_pauseMenuEntity(0)
        , m_pauseMenuBackgroundEntity(0)
        , m_pauseMenuRibbonEntity(0)
        , m_pauseMenuContentEntity(0)
        , m_systemTitleEntity(0)
        , m_skillsTitleEntity(0)
        , m_hatsTitleEntity(0)
        , m_statsTitleEntity(0)
        , m_skillsContentEntity(0)
        , m_hatsContentEntity(0)
        , m_mainMenuButtonEntity(0)
        , m_draggingMaster(false)
        , m_draggingMusic(false)
        , m_draggingSFX(false)
        , m_activeDragKnob(-1)
        , m_dragStartX(0.0f)
        , m_dragKnobStartX(0.0f)
        , m_sliderX(0.0f)
        , m_sliderY(0.0f)
        , m_sliderW(0.0f)
        , m_sliderH(0.0f)
        , m_sliderSpacing(0.0f)
        , m_draggedKnobEntity(0)
        , m_dragOffsetX(0.0f)
        , m_masterSliderValue(1.0f)
        , m_musicSliderValue(1.0f)
        , m_sfxSliderValue(1.0f)
        , m_currentSkillIndex(0)
        , m_availableSkills()
        , m_skillSystem(nullptr)
        , m_hatsSystem(nullptr)
        , m_playerCoins(0)
        , m_playerEntity(0)
        , m_sessionPipes(0)
        , m_sessionCoins(0)
        , m_totalCoins(0)
        , m_grossTotalCoins(0)
        , m_totalFlops(0)
        , m_enemiesKilled(0)
        , m_totalPipes(0)
        , m_lastSkillButtonPressTime(0.0f)
        , m_skillButtonDebounceDelay(0.1f)
        , m_lastActionButtonPressTime(0.0f)
        , m_actionButtonDebounceDelay(0.5f)
        , m_lastRibbonButtonPressTime(0.0f)
        , m_ribbonButtonDebounceDelay(0.1f)
        , m_lastSettingsButtonPressTime(0.0f)
        , m_settingsButtonDebounceDelay(0.3f)
        , m_currentSessionTextEntity(0)
        , m_sessionCoinsTextEntity(0)
        , m_totalCoinsTextEntity(0)
        , m_totalFlopsTextEntity(0)
        , m_enemiesKilledTextEntity(0)
        , m_totalPipesTextEntity(0)
        , m_bossTimerTextEntity(0)
        , m_hatsBackgroundEntity(0)
        , m_hatsActionButtonEntity(0)
        , m_hatsCostDisplayEntity(0)
    {
        GN_LOG_INFO("PauseSystem created");

        // Initialize skill system data from GameplayState
        if (m_gameplayState) {
            // Get skill system and hats system from GameplayState using getter methods
            m_skillSystem = m_gameplayState->GetSkillSystem();
            m_hatsSystem = m_gameplayState->GetHatsSystem();

        // Initialize available skills
        m_availableSkills = {
            GameCore::SkillType::HalfHearts,
            GameCore::SkillType::ThirdHearts,
            GameCore::SkillType::CoinMagnet,
            GameCore::SkillType::HeartMagnet,
            GameCore::SkillType::CoinSafetyNet
        };

            // Get player entity and coins
            m_playerEntity = m_gameplayState->GetPlayerEntity();
            if (m_playerEntity != 0 && m_ecsCoordinator) {
                auto* playerComp = m_ecsCoordinator->GetComponent<PlayerComponent>(m_playerEntity);
                if (playerComp) {
                    // Combine both session and total coins for purchasing
                    m_playerCoins = playerComp->sessionCoins + playerComp->totalCoins;
                }
            }

            GN_LOG_INFO("PauseSystem: Initialized skill system data - player entity: " + std::to_string(m_playerEntity) +
                       ", skill system: " + (m_skillSystem ? "available" : "null") +
                       ", hats system: " + (m_hatsSystem ? "available" : "null"));
        } else {
            GN_LOG_ERROR("PauseSystem: GameplayState is null, cannot initialize skill system data");
        }
    }

    PauseSystem::~PauseSystem() {
        GN_LOG_INFO("PauseSystem destroyed");
        Cleanup();
    }

    // Orientation helpers
    bool PauseSystem::IsLandscapeMode() const {
        // Check if we have a gameplay state reference and use its orientation detection
        if (m_gameplayState) {
            return m_gameplayState->IsLandscapeMode();
        }
        // Fallback: check screen dimensions directly
        return m_screenWidth > m_screenHeight;
    }

    void PauseSystem::UpdateScreenDimensions(float width, float height) {
        // Only update if dimensions actually changed (prevents unnecessary updates)
        if (m_screenWidth != width || m_screenHeight != height) {
            GN_LOG_INFO("PauseSystem: Screen dimensions updated from " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight) +
                       " to " + std::to_string(width) + "x" + std::to_string(height));

        m_screenWidth = width;
        m_screenHeight = height;

            // If pause menu is already created and visible, we might need to reposition elements
            // But for now, just log the change - entities will be repositioned on next show
        }
    }

    void PauseSystem::UpdateSkillData(int currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills,
                                     SkillSystem* skillSystem, int playerCoins, Entity playerEntity) {
        m_currentSkillIndex = currentSkillIndex;
        m_availableSkills = availableSkills;
        m_skillSystem = skillSystem;
        m_playerCoins = playerCoins;
        m_playerEntity = playerEntity;
        GN_LOG_INFO("PauseSystem: Skill data updated - index: " + std::to_string(currentSkillIndex) +
                   ", skills: " + std::to_string(availableSkills.size()) + ", coins: " + std::to_string(playerCoins));
    }

    void PauseSystem::UpdateStatsData(int sessionPipes, int sessionCoins, int totalCoins, int grossTotalCoins, int totalFlops,
                                     int enemiesKilled, int totalPipes) {
        m_sessionPipes = sessionPipes;
        m_sessionCoins = sessionCoins;
        m_totalCoins = totalCoins;
        m_grossTotalCoins = grossTotalCoins;
        m_totalFlops = totalFlops;
        m_enemiesKilled = enemiesKilled;
        m_totalPipes = totalPipes;
        GN_LOG_INFO("PauseSystem: Stats data updated - session pipes: " + std::to_string(sessionPipes) +
                   ", total coins: " + std::to_string(totalCoins) + ", gross total: " + std::to_string(grossTotalCoins));
    }

    void PauseSystem::Initialize() {
        GN_LOG_INFO("PauseSystem: Initializing pause menu system");

        if (m_isCreated) {
            GN_LOG_INFO("PauseSystem: Already initialized, skipping");
            return;
        }

        // Create all pause menu entities upfront
        CreatePauseMenu();
        m_isCreated = true;

        GN_LOG_INFO("PauseSystem: Initialization complete");
    }

    void PauseSystem::Update(float deltaTime) {
        if (!m_isVisible) {
            return;
        }

        // Update button debounce timers
        m_lastSkillButtonPressTime += deltaTime;
        m_lastActionButtonPressTime += deltaTime;
        m_lastRibbonButtonPressTime += deltaTime;
        m_lastSettingsButtonPressTime += deltaTime;

        // Update any animated elements in the pause menu
        // For now, this is minimal as pause menu is mostly static
    }

    void PauseSystem::HandleInput(float touchX, float touchY, TouchState touchState) {
        GN_LOG_INFO("🚀 PauseSystem::HandleInput called with (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        if (!m_isVisible) {
            GN_LOG_INFO("PauseSystem: Not visible, ignoring input at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");
            GN_LOG_INFO("🚨 PauseSystem is not visible! Cannot process input.");
            return;
        }

        std::string stateStr = (touchState == TouchState::PRESSED) ? "PRESSED" :
                              (touchState == TouchState::HELD) ? "HELD" : "RELEASED";
        GN_LOG_INFO("✅ PauseSystem is visible, processing " + stateStr + " input at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        // Handle knob dragging for all touch states (needed for continuous dragging)
        if (m_currentTab == PauseMenuTab::SYSTEM) {
            HandleKnobDrag(touchX, touchY, touchState);
        }

        // CRITICAL FIX: Process button clicks on RELEASED instead of PRESSED to prevent double-clicks
        // This matches the pattern used in LeaderboardState and MainMenuState
        if (touchState != TouchState::RELEASED) {
            GN_LOG_INFO("PauseSystem: Ignoring " + stateStr + " touch - only processing RELEASED for clicks");
            return;
        }

        GN_LOG_INFO("PauseSystem: Handling RELEASED touch for button clicks at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ") - VISIBLE AND ACTIVE");

        // Handle pause menu input (this includes settings button detection)
        HandlePauseMenuInput(touchX, touchY);
    }

    void PauseSystem::Show() {
        if (!m_isCreated) {
            Initialize();
        }

        GN_LOG_INFO("PauseSystem: Showing pause menu");
        m_isVisible = true;

        // Reset debounce timers when showing pause menu to prevent immediate clicks
        m_lastRibbonButtonPressTime = 0.0f;
        m_lastSettingsButtonPressTime = 0.0f;

        // Show pause menu background
        ShowPauseMenu();
    }

    void PauseSystem::Hide() {
        GN_LOG_INFO("PauseSystem: Hiding pause menu");
        m_isVisible = false;

        // Hide pause menu
        HidePauseMenu();
    }

    void PauseSystem::SwitchToTab(PauseMenuTab tab) {
        GN_LOG_INFO("PauseSystem: Switching to tab " + std::to_string(static_cast<int>(tab)));
        m_currentTab = tab;
        ShowTabContent(tab);
    }

    void PauseSystem::Cleanup() {
        GN_LOG_INFO("PauseSystem: Cleaning up pause menu entities");

        // Destroy all pause menu entities
        if (m_pauseMenuBackgroundEntity != 0 && m_ecsCoordinator) {
            m_ecsCoordinator->DestroyEntity(m_pauseMenuBackgroundEntity);
            m_pauseMenuBackgroundEntity = 0;
        }

        if (m_pauseMenuRibbonEntity != 0 && m_ecsCoordinator) {
            m_ecsCoordinator->DestroyEntity(m_pauseMenuRibbonEntity);
            m_pauseMenuRibbonEntity = 0;
        }

        if (m_pauseMenuContentEntity != 0 && m_ecsCoordinator) {
            m_ecsCoordinator->DestroyEntity(m_pauseMenuContentEntity);
            m_pauseMenuContentEntity = 0;
        }

        // Clean up ribbon buttons
        for (auto& button : m_ribbonButtons) {
            if (button != 0 && m_ecsCoordinator) {
                m_ecsCoordinator->DestroyEntity(button);
            }
        }
        m_ribbonButtons.clear();

        m_isCreated = false;
        m_isVisible = false;

        GN_LOG_INFO("PauseSystem: Cleanup complete");
    }

    // Placeholder implementations - these will be filled in as we extract methods from GameplayState
    void PauseSystem::CreatePauseMenu() {
        GN_LOG_INFO("PauseSystem: Creating pause menu entities");
        CreatePauseMenuBackground();
        CreatePauseMenuRibbon();
        CreatePauseMenuContent();
    }

    void PauseSystem::CreatePauseMenuBackground() {
        GN_LOG_INFO("PauseSystem: Creating pause menu background");

        m_pauseMenuBackgroundEntity = m_ecsCoordinator->CreateEntity();
        if (m_pauseMenuBackgroundEntity != 0) {
            GN_LOG_INFO("PAUSE MENU DEBUG: Using cached screen dimensions: " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight));

            // Check if we have valid iPhone screen dimensions (not fallback 800x600)
            if (m_screenWidth < 1000 || m_screenHeight < 1000) {
                GN_LOG_WARN("PAUSE MENU DEBUG: Invalid screen dimensions detected - delaying pause menu creation");
                GN_LOG_WARN("PAUSE MENU DEBUG: Expected iPhone portrait dimensions like 1179x2556, Got: " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight));
                GN_LOG_WARN("PAUSE MENU DEBUG: Pause menu will be created when valid screen dimensions are available");

                // Mark pause menu as not created and return early
                m_isCreated = false;
                return;
            }
        } else {
            GN_LOG_WARN("PAUSE MENU DEBUG: Entity creation failed");
            m_isCreated = false;
            return;
        }

        // Choose texture based on orientation
        std::string bgTextureId;
        float bgScale;
        float textureWidth, textureHeight;

        if (IsLandscapeMode()) {
            // Landscape mode: use the regular background (which is rotated 90 degrees)
            bgTextureId = "PauseMenuBackground"; // Use the rotated background
            bgScale = 7.0f;  // May need adjustment for landscape
            // Swap dimensions since texture is rotated 90 degrees
            textureWidth = 300.0f;  // Was 160, now 300 (rotated)
            textureHeight = 160.0f; // Was 300, now 160 (rotated)
            GN_LOG_INFO("PauseSystem: Using landscape pause menu background (rotated 90 degrees) - dimensions: " +
                       std::to_string(textureWidth) + "x" + std::to_string(textureHeight));
        } else {
            // Portrait mode: use existing mobile background
            bgTextureId = "PauseMenuBackgroundMobile";
            textureWidth = 160.0f;
            textureHeight = 300.0f;
            
            // Dynamic scaling: Fit within 95% of width and 90% of height
            float maxW = m_screenWidth * 0.95f;
            float maxH = m_screenHeight * 0.90f;
            
            float scaleX = maxW / textureWidth;
            float scaleY = maxH / textureHeight;
            bgScale = std::min(scaleX, scaleY); // Dynamic scale
            
            GN_LOG_INFO("PauseSystem: Using portrait pause menu background - scale: " + std::to_string(bgScale) + 
                       " (calculated from " + std::to_string(maxW) + "x" + std::to_string(maxH) + ")");
        }

        // SCALE FIRST, then center: Calculate final rendered dimensions, then center those
        float scaledWidth = textureWidth * bgScale;   // 160 * 7 = 1120
        float scaledHeight = textureHeight * bgScale; // 300 * 7 = 2100
        float centerX = m_screenWidth * 0.5f;
        float centerY = m_screenHeight * 0.5f;
        // Lower the background slightly on the Y axis so the settings button is not covered
        GNVector2 bgPosition = CenterObjectAtPosition(centerX, centerY + 32.0f, scaledWidth, scaledHeight);

        GN_LOG_INFO("PAUSE MENU DEBUG: Screen center target: (" + std::to_string(centerX) + ", " + std::to_string(centerY) + ")");
        GN_LOG_INFO("PAUSE MENU DEBUG: Original texture: " + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) + " at " + std::to_string(bgScale) + "x scale");
        GN_LOG_INFO("PAUSE MENU DEBUG: Final rendered size: " + std::to_string(scaledWidth) + "x" + std::to_string(scaledHeight));
        GN_LOG_INFO("PAUSE MENU DEBUG: Calculated top-left position: (" + std::to_string(bgPosition.x) + ", " + std::to_string(bgPosition.y) + ")");

        Transform bgTransform(GNVector2(bgPosition.x, bgPosition.y), 0.0f, GNVector2(bgScale, bgScale));
        m_ecsCoordinator->AddComponent<Transform>(m_pauseMenuBackgroundEntity, bgTransform);

        // Create UIElement for pause menu background - this keeps it fixed on screen
        UIElement bgUI;
        bgUI.normalTextureId = bgTextureId;
        bgUI.visible = false; // Initially hidden
        bgUI.isEnabled = true;
        bgUI.textLayer = 80; // Above regular UI, below critical controls
        m_ecsCoordinator->AddComponent<UIElement>(m_pauseMenuBackgroundEntity, bgUI);

        // Add Sprite component so RenderSystem can get correct dimensions
        // This is crucial for proper centering - RenderSystem uses Sprite dimensions for UIElement textures
        Sprite bgSprite(bgTextureId, (int)textureWidth, (int)textureHeight);
        bgSprite.layer = 80; // Match updated textLayer
        bgSprite.visible = false; // Initially hidden
        m_ecsCoordinator->AddComponent<Sprite>(m_pauseMenuBackgroundEntity, bgSprite);

        GN_LOG_INFO("PauseSystem: Created pause menu background at position (" + std::to_string(bgPosition.x) + ", " + std::to_string(bgPosition.y) + ") with scale " + std::to_string(bgScale) + " (size: " + std::to_string(textureWidth * bgScale) + "x" + std::to_string(textureHeight * bgScale) + ")");
        GN_LOG_INFO("PauseSystem: Background will be centered at screen center (" + std::to_string(m_screenWidth * 0.5f) + ", " + std::to_string(m_screenHeight * 0.5f) + ")");
    }

    void PauseSystem::CreatePauseMenuRibbon() {
        GN_LOG_INFO("PauseSystem: Creating pause menu ribbon");

        m_pauseMenuRibbonEntity = m_ecsCoordinator->CreateEntity();
        if (m_pauseMenuRibbonEntity != 0) {
            // Position ribbon at top of screen
            float ribbonX = 0.0f; // Start at left edge
            float ribbonY = m_screenHeight * 0.15f; // 15% from top

            Transform ribbonTransform(GNVector2(ribbonX, ribbonY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_pauseMenuRibbonEntity, ribbonTransform);

            // Create UIElement for ribbon (invisible, just for positioning)
            UIElement ribbonUI;
            ribbonUI.visible = false; // Initially hidden
            ribbonUI.isEnabled = true;
            ribbonUI.textLayer = 45; // Above background, below other UI
            m_ecsCoordinator->AddComponent<UIElement>(m_pauseMenuRibbonEntity, ribbonUI);

            // Create ribbon buttons
            CreateRibbonButtons();

            GN_LOG_INFO("PauseSystem: Created pause menu ribbon at (" + std::to_string(ribbonX) + ", " + std::to_string(ribbonY) + ")");
        }
    }

    void PauseSystem::CreateRibbonButtons() {
        GN_LOG_INFO("PauseSystem: Creating ribbon buttons - screen: " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight) +
                   ", landscape mode: " + std::to_string(IsLandscapeMode()));

        // Button labels
        const char* buttonLabels[] = {"SKILLS", "HATS", "STATS", "SYSTEM"};

        // Calculate button dimensions based on orientation
        float buttonScale = 6.0f;
        int numButtons = 4;

        if (IsLandscapeMode()) {
            // Landscape mode: horizontal layout across top with FloppyButtonBlue
            GN_LOG_INFO("PauseSystem: Creating landscape mode buttons (horizontal layout)");

            // Get actual texture dimensions from RenderSystem
            int texWidth = 90, texHeight = 16; // Default FloppyButtonBlue dimensions
            if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
                rs->PreloadTexture("FloppyButtonBlue");
                if (!rs->GetTextureSize("FloppyButtonBlue", texWidth, texHeight)) {
                    texWidth = 90; texHeight = 16; // Fallback
                }
            }

            float buttonWidth = static_cast<float>(texWidth) * buttonScale;
            float buttonHeight = static_cast<float>(texHeight) * buttonScale;

            // Position buttons horizontally across the top
            float totalWidth = buttonWidth * numButtons;
            float spacing = (m_screenWidth - totalWidth) / (numButtons + 1); // Even spacing
            float startY = m_screenHeight * 0.08f; // 8% from top

            GN_LOG_INFO("PauseSystem: Landscape buttons - totalWidth: " + std::to_string(totalWidth) +
                       ", spacing: " + std::to_string(spacing) + ", startY: " + std::to_string(startY));

            for (int i = 0; i < numButtons; i++) {
                Entity buttonEntity = m_ecsCoordinator->CreateEntity();
                if (buttonEntity != 0) {
                    // Horizontal positioning across top
                    float buttonX = spacing + (i * (buttonWidth + spacing));
                    float buttonY = startY;

                    Transform buttonTransform(GNVector2(buttonX, buttonY), 0.0f, GNVector2(buttonScale, buttonScale));
                    m_ecsCoordinator->AddComponent<Transform>(buttonEntity, buttonTransform);

                    // Create UI element with FloppyButtonBlue
                    UIElement buttonUI;
                    buttonUI.normalTextureId = "FloppyButtonBlue";
                    buttonUI.buttonText = buttonLabels[i];
                    buttonUI.fontSize = 36.0f; // Increased font size for better visibility in landscape
                    buttonUI.textColor = GNColor(255, 255, 255, 255); // White text
                    buttonUI.centerTextHorizontally = true;
                    buttonUI.centerTextVertically = true;
                    buttonUI.textLayer = 95; // Above all content
                    buttonUI.textOffsetX = 0.0f; // Ensure no horizontal offset
                    buttonUI.textOffsetY = 0.0f; // No vertical offset for proper centering
                    buttonUI.visible = false; // Initially hidden
                    m_ecsCoordinator->AddComponent<UIElement>(buttonEntity, buttonUI);

                    // Add Sprite component
                    Sprite buttonSprite("FloppyButtonBlue", texWidth, texHeight);
                    buttonSprite.layer = 95; // Match textLayer
                    buttonSprite.visible = false; // Initially hidden
                    m_ecsCoordinator->AddComponent<Sprite>(buttonEntity, buttonSprite);

                    // Add Bounds component for proper collision detection
                    Bounds buttonBounds(buttonWidth, buttonHeight, 0.0f, 0.0f, false); // Top-left aligned
                    m_ecsCoordinator->AddComponent<Bounds>(buttonEntity, buttonBounds);

                    m_ribbonButtons.push_back(buttonEntity);

                    GN_LOG_INFO("PauseSystem: Created landscape button '" + std::string(buttonLabels[i]) +
                               "' at (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) +
                               ") - size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight));
                }
            }
        } else {
            // Portrait mode: vertical ribbon layout on left side
            GN_LOG_INFO("PauseSystem: Creating portrait mode ribbon buttons (vertical layout)");

            float buttonWidth = 64.0f * buttonScale;
            float buttonHeight = buttonWidth * 0.62f; // Match ribbon button aspect ratio

            // Recalculate background scale to match CreatePauseMenuBackground logic
            float textureWidth = 160.0f;
            float textureHeight = 300.0f;
            float maxW = m_screenWidth * 0.95f;
            float maxH = m_screenHeight * 0.90f; // 90% height target
            float scaleX = maxW / textureWidth;
            float scaleY = maxH / textureHeight;
            float bgScale = std::min(scaleX, scaleY);
            
            float bgHeight = 300.0f * bgScale;
            float bgTop = (m_screenHeight - bgHeight) * 0.5f;
            float startY = bgTop + buttonWidth * 0.18f; // Skills button higher
            float buttonX = -0.40f * buttonWidth; // Offset further left (40%) - matches original

            for (int i = 0; i < numButtons; i++) {
                Entity buttonEntity = m_ecsCoordinator->CreateEntity();
                if (buttonEntity != 0) {
                    // Vertical positioning like tabs - original positioning
                    float buttonY = startY + i * buttonHeight;

                // Calculate scale based on orientation to maintain proper aspect ratio
                    float scaleX = buttonWidth / 64.0f;
                    float scaleY = buttonHeight / 21.0f;
                Transform buttonTransform(GNVector2(buttonX, buttonY), 0.0f, GNVector2(scaleX, scaleY));
                m_ecsCoordinator->AddComponent<Transform>(buttonEntity, buttonTransform);

                    // Create UI element with ribbon button
                    UIElement buttonUI;
                    buttonUI.normalTextureId = "PauseMenuRibbonButton";
                    buttonUI.buttonText = buttonLabels[i];
                    buttonUI.fontSize = 38.0f;
                    buttonUI.textColor = GNColor(255, 255, 255, 255); // White text
                    buttonUI.centerTextHorizontally = false; // Left-align text for ribbons
                    buttonUI.centerTextVertically = true; // Center vertically
                    buttonUI.textLayer = 95; // Above all content
                    buttonUI.textOffsetX = buttonWidth * 0.42f; // Original left offset
                    buttonUI.textOffsetY = -48.0f; // Shift text up 48 pixels for proper centering
                    buttonUI.visible = false; // Initially hidden
                m_ecsCoordinator->AddComponent<UIElement>(buttonEntity, buttonUI);

                    // Add Sprite component
                    Sprite buttonSprite("PauseMenuRibbonButton", 64, 21);
                buttonSprite.layer = 95; // Match textLayer
                    buttonSprite.visible = false; // Initially hidden
                m_ecsCoordinator->AddComponent<Sprite>(buttonEntity, buttonSprite);

                    // Add Bounds component for proper collision detection
                    Bounds buttonBounds(buttonWidth, buttonHeight, 0.0f, 0.0f, false); // Top-left aligned
                    m_ecsCoordinator->AddComponent<Bounds>(buttonEntity, buttonBounds);

                m_ribbonButtons.push_back(buttonEntity);

                    GN_LOG_INFO("PauseSystem: Created portrait ribbon button '" + std::string(buttonLabels[i]) +
                               "' at (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) +
                               ") - size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight));
                }
            }
        }

        GN_LOG_INFO("PauseSystem: Created " + std::to_string(m_ribbonButtons.size()) + " ribbon buttons");
    }

    void PauseSystem::CreatePauseMenuContent() {
        GN_LOG_INFO("PauseSystem: Creating pause menu content area");

        m_pauseMenuContentEntity = m_ecsCoordinator->CreateEntity();
        if (m_pauseMenuContentEntity != 0) {
            // Position content area below ribbon
            float contentX = m_screenWidth * 0.5f; // Center horizontally
            float contentY = m_screenHeight * 0.45f; // Below ribbon

            Transform contentTransform(GNVector2(contentX, contentY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_pauseMenuContentEntity, contentTransform);

            // Create content sprite (invisible, just for positioning)
            Sprite contentSprite; // No texture, just for positioning
            contentSprite.layer = 85; // Above pause menu background (80)
            contentSprite.visible = false; // Initially hidden
            m_ecsCoordinator->AddComponent<Sprite>(m_pauseMenuContentEntity, contentSprite);

            // Create tab content - create ALL tabs upfront
            CreateSystemTab();
            CreateSkillsTab();
            CreateHatsTab();
            CreateStatsTab();

            GN_LOG_INFO("PauseSystem: Created pause menu content area at (" + std::to_string(contentX) + ", " + std::to_string(contentY) + ")");
        }
    }

    void PauseSystem::CreateSystemTab() {
        GN_LOG_INFO("PauseSystem: Creating system tab");

        // Create "System" title text at the very top of the pause menu
        if (m_systemTitleEntity == 0) {
            m_systemTitleEntity = m_ecsCoordinator->CreateEntity();

            // Position at the top center of the screen (like other tab titles)
            float titleX = m_screenWidth * 0.5f;
            float titleY;
            if (IsLandscapeMode()) {
                titleY = m_screenHeight * 0.22f; // Lower in landscape to account for tab buttons
            } else {
                titleY = m_screenHeight * 0.15f; // Near the top
            }

            Transform titleTransform(GNVector2(titleX, titleY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_systemTitleEntity, titleTransform);

            UIElement titleElem;
            titleElem.buttonText = "System";
            titleElem.fontSize = 72.0f; // Large title font
            titleElem.textColor = GNColor(255, 255, 255, 255);
            titleElem.centerTextHorizontally = true;
            titleElem.centerTextVertically = true;
            titleElem.visible = false;
            titleElem.isEnabled = true;
            titleElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_systemTitleEntity, titleElem);

            Sprite titleSprite;
            titleSprite.layer = 90;
            titleSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_systemTitleEntity, titleSprite);
        }

        // Create main menu button - EXACT same pattern as original
        GN_LOG_INFO("PauseSystem: Creating main menu button - screen: " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight));
        if (m_mainMenuButtonEntity == 0) {
            m_mainMenuButtonEntity = m_ecsCoordinator->CreateEntity();
            GN_LOG_INFO("PauseSystem: Created main menu button entity: " + std::to_string(m_mainMenuButtonEntity));

            // Calculate center position for systems tab area (EXACT same as original)
            float bgW = m_screenWidth * 0.8f;
            float bgH = m_screenHeight * 0.7f;
            float bgX = (m_screenWidth - bgW) * 0.5f;
            float bgY = (m_screenHeight - bgH) * 0.18f;

            // Position button in center of entire screen (not just tab area)
            float buttonCenterX = m_screenWidth * 0.5f; // Center horizontally on screen
            float buttonCenterY;
            if (IsLandscapeMode()) {
                buttonCenterY = m_screenHeight * 0.90f; // Match skill/hat button position
            } else {
                buttonCenterY = m_screenHeight * 0.75f; // Position at 75% down screen (lower)
            }

            // Use larger scale for main menu size (10x scaling for 900x160 button)
            float buttonScale = 10.0f;
            float buttonWidth = 90.0f * buttonScale;  // 900 pixels (like working buttons)
            float buttonHeight = 16.0f * buttonScale; // 160 pixels (like working buttons)

            // Use positioning helper to center button (EXACT same pattern as main menu buttons)
            GNVector2 buttonPosition = CenterObjectAtPosition(buttonCenterX, buttonCenterY, buttonWidth, buttonHeight);
            float buttonX = buttonPosition.x;  // This is the TOP-LEFT X position for the sprite
            float buttonY = buttonPosition.y;  // This is the TOP-LEFT Y position for the sprite

            // Log the calculated positions for debugging
            GN_LOG_INFO("PauseSystem: Screen dimensions - width: " + std::to_string(m_screenWidth) + ", height: " + std::to_string(m_screenHeight));
            GN_LOG_INFO("PauseSystem: Main menu button center - buttonCenterX: " + std::to_string(buttonCenterX) + ", buttonCenterY: " + std::to_string(buttonCenterY));
            GN_LOG_INFO("PauseSystem: Main menu button top-left - buttonX: " + std::to_string(buttonX) + ", buttonY: " + std::to_string(buttonY));
            GN_LOG_INFO("PauseSystem: Main menu button dimensions - width: " + std::to_string(buttonWidth) + ", height: " + std::to_string(buttonHeight));
            GN_LOG_INFO("PauseSystem: Main menu button expected text center - textCenterX: " + std::to_string(buttonX + buttonWidth * 0.5f) + ", textCenterY: " + std::to_string(buttonY + buttonHeight * 0.5f));
            GN_LOG_INFO("PauseSystem: Main menu button sprite position: (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")");

            Transform buttonTransform(GNVector2(buttonX, buttonY), 0.0f, GNVector2(buttonScale, buttonScale));
            m_ecsCoordinator->AddComponent<Transform>(m_mainMenuButtonEntity, buttonTransform);

            // Create sprite using FloppyButtonBlue.png (full filename for proper texture loading)
            // Use actual texture dimensions like working buttons (90x16, not 64x64)
            Sprite buttonSprite("FloppyButtonBlue.png", 90, 16); // Use actual texture dimensions like working buttons
            buttonSprite.layer = 90; // High layer above everything (pause menu background is 80)
            buttonSprite.visible = false; // Initially hidden
            GN_LOG_INFO("PauseSystem: Creating Main Menu button sprite: FloppyButtonBlue, 90x16, layer " + std::to_string(buttonSprite.layer) +
                       " at center (" + std::to_string(buttonCenterX) + ", " + std::to_string(buttonCenterY) + ") with scale " + std::to_string(buttonScale));
            m_ecsCoordinator->AddComponent<Sprite>(m_mainMenuButtonEntity, buttonSprite);

            // Create UI element for button text - use EXACT same pattern as main menu buttons
            UIElement buttonUI("MAIN MENU", "FloppyButtonBlue", "FloppyButtonBlueHover");
            buttonUI.fontSize = 62.0f; // Lowered font size
            buttonUI.textColor = GNColor(255, 255, 255, 255); // White text
            buttonUI.centerTextHorizontally = true;
            buttonUI.centerTextVertically = true;
            buttonUI.textLayer = 90; // Same layer as button sprite for proper alignment
            buttonUI.visible = false; // Initially hidden
            // NO OFFSET - text should be perfectly centered as requested
            buttonUI.textOffsetX = 0.0f;  // No horizontal offset for perfect centering
            buttonUI.textOffsetY = 0.0f;  // No vertical offset for perfect centering

            // Add debug logging for button creation
            GN_LOG_INFO("PauseSystem: Main menu button UI created - text: '" + std::string(buttonUI.buttonText) +
                       "', font size: " + std::to_string(buttonUI.fontSize) +
                       ", position: (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) + ")");

            m_ecsCoordinator->AddComponent<UIElement>(m_mainMenuButtonEntity, buttonUI);
        }

        // Create audio sliders
        CreateAudioSliders();

        GN_LOG_INFO("PauseSystem: System tab created");
    }

    void PauseSystem::CreateSkillsTab() {
        GN_LOG_INFO("PauseSystem: Creating skills tab");

        // Create "Skills" title text at the very top of the pause menu
        if (m_skillsTitleEntity == 0) {
            m_skillsTitleEntity = m_ecsCoordinator->CreateEntity();

            // Position at the top center of the screen (like other tab titles)
            float titleX = m_screenWidth * 0.5f;
            float titleY;
            if (IsLandscapeMode()) {
                titleY = m_screenHeight * 0.22f; // Lower in landscape to account for tab buttons
            } else {
                titleY = m_screenHeight * 0.15f; // Near the top
            }

            Transform titleTransform(GNVector2(titleX, titleY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_skillsTitleEntity, titleTransform);

            UIElement titleElem;
            titleElem.buttonText = "Skills";
            titleElem.fontSize = 72.0f; // Large title font
            titleElem.textColor = GNColor(255, 255, 255, 255);
            titleElem.centerTextHorizontally = true;
            titleElem.centerTextVertically = true;
            titleElem.visible = false;
            titleElem.isEnabled = true;
            titleElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_skillsTitleEntity, titleElem);

            Sprite titleSprite;
            titleSprite.layer = 90;
            titleSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_skillsTitleEntity, titleSprite);
        }

        // Create black background like stats tab (adjusted for orientation)
        if (m_skillsBackgroundEntity == 0) {
            m_skillsBackgroundEntity = m_ecsCoordinator->CreateEntity();
            // Dynamic percentage-based sizing - constrained to fit within pause menu
            float bgWidth, bgHeight;
            if (IsLandscapeMode()) {
                // Landscape mode: 55% of screen width, 50% of screen height
                bgWidth = m_screenWidth * 0.55f;
                bgHeight = m_screenHeight * 0.55f;
            } else {
                // Portrait mode: 55% of screen width, 50% of screen height
                bgWidth = m_screenWidth * 0.55f;
                bgHeight = m_screenHeight * 0.55f;
            }

            // Calculate layout positions (EXACT same as stats tab)
            float centerX = m_screenWidth * 0.5f;
            float centerY = m_screenHeight * 0.5f + 32.0f;

            // Center position with same offset as pause menu (+32 Y offset)
            GNVector2 bgPosition = CenterObjectAtPosition(centerX, centerY, bgWidth, bgHeight);

            Transform bgTransform(bgPosition, 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_skillsBackgroundEntity, bgTransform);

            // Create black rectangle with slight transparency (EXACT same as stats tab)
            UIShape bgShape;
            bgShape.type = UIShapeType::Rectangle;
            bgShape.width = bgWidth;
            bgShape.height = bgHeight;
            bgShape.color = GNColor(0, 0, 0, 200); // Black with ~78% opacity (200/255)
            bgShape.visible = false; // Initially hidden
            bgShape.layer = 81; // Above pause menu background (80) but below skills text (80)
            m_ecsCoordinator->AddComponent<UIShape>(m_skillsBackgroundEntity, bgShape);

            // Add UIElement component for proper visibility management
            UIElement bgElement;
            bgElement.visible = false; // Initially hidden
            bgElement.isEnabled = true;
            bgElement.textLayer = 81; // Match UIShape layer
            m_ecsCoordinator->AddComponent<UIElement>(m_skillsBackgroundEntity, bgElement);

            Sprite bgSprite;
            bgSprite.layer = 81; // Match textLayer
            bgSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_skillsBackgroundEntity, bgSprite);
        }

        // Create skill name text (positioned relative to the black background)
        if (m_skillsNameEntity == 0) {
            m_skillsNameEntity = m_ecsCoordinator->CreateEntity();

            // Position within the black background area (adjusted for landscape)
            float bgWidth, bgHeight;
            if (IsLandscapeMode()) {
                bgWidth = 1470.0f;
                bgHeight = 784.0f;
            } else {
                bgWidth = 1120.0f * 0.7f;
                bgHeight = 2100.0f * 0.7f;
            }

            float bgX = CenterObjectAtPosition(m_screenWidth * 0.5f, m_screenHeight * 0.5f + 32.0f, bgWidth, bgHeight).x;
            float bgY = CenterObjectAtPosition(m_screenWidth * 0.5f, m_screenHeight * 0.5f + 32.0f, bgWidth, bgHeight).y;

            float nameX = bgX + bgWidth * 0.5f; // Center within background
            float nameY;
            if (IsLandscapeMode()) {
                nameY = bgY + bgHeight * 0.3f; // Lower in landscape (30% down from background top)
            } else {
                nameY = bgY + 150.0f; // Near the top of the background
            }

            Transform nameTransform(GNVector2(nameX, nameY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_skillsNameEntity, nameTransform);

            UIElement nameElem;
            nameElem.buttonText = "Select a Skill";
            nameElem.fontSize = 48.0f;
            nameElem.textColor = GNColor(255, 255, 255, 255);
            nameElem.centerTextHorizontally = true;
            nameElem.centerTextVertically = true;
            nameElem.visible = false;
            nameElem.isEnabled = true;
            nameElem.textLayer = 82;
            m_ecsCoordinator->AddComponent<UIElement>(m_skillsNameEntity, nameElem);

            Sprite nameSprite;
            nameSprite.layer = 82;
            nameSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_skillsNameEntity, nameSprite);
        }

        // Create skill description text
        if (m_skillsDescriptionEntity == 0) {
            m_skillsDescriptionEntity = m_ecsCoordinator->CreateEntity();

            // Position within the black background area (adjusted for landscape)
            float bgWidth, bgHeight;
            if (IsLandscapeMode()) {
                bgWidth = 1470.0f;
                bgHeight = 784.0f;
            } else {
                bgWidth = 1120.0f * 0.7f;
                bgHeight = 2100.0f * 0.7f;
            }

            float bgX = CenterObjectAtPosition(m_screenWidth * 0.5f, m_screenHeight * 0.5f + 32.0f, bgWidth, bgHeight).x;
            float bgY = CenterObjectAtPosition(m_screenWidth * 0.5f, m_screenHeight * 0.5f + 32.0f, bgWidth, bgHeight).y;

            float descX = bgX + bgWidth * 0.5f; // Center within background
            float descY;
            if (IsLandscapeMode()) {
                descY = bgY + bgHeight * 0.5f; // Center vertically in landscape
            } else {
                descY = bgY + 300.0f; // Below name text
            }

            Transform descTransform(GNVector2(descX, descY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_skillsDescriptionEntity, descTransform);

            UIElement descElem;
            descElem.buttonText = "Choose a skill to unlock";
            descElem.fontSize = 32.0f;
            descElem.textColor = GNColor(255, 255, 255, 255);
            descElem.centerTextHorizontally = true;
            descElem.centerTextVertically = true;
            descElem.visible = false;
            descElem.isEnabled = true;
            descElem.textLayer = 82;
            m_ecsCoordinator->AddComponent<UIElement>(m_skillsDescriptionEntity, descElem);

            Sprite descSprite;
            descSprite.layer = 82;
            descSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_skillsDescriptionEntity, descSprite);
        }

        // Create cost text
        if (m_skillsCostEntity == 0) {
            m_skillsCostEntity = m_ecsCoordinator->CreateEntity();

            // Position cost text just above the unlock button
            float unlockButtonY;
            if (IsLandscapeMode()) {
                unlockButtonY = m_screenHeight * 0.85f; // Lower in landscape
            } else {
                unlockButtonY = m_screenHeight * 0.75f;
            }
            float costY = unlockButtonY - 150.0f; // Closer to button in landscape
            float costX = m_screenWidth * 0.5f; // Center horizontally on screen

            Transform costTransform(GNVector2(costX, costY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_skillsCostEntity, costTransform);

            UIElement costElem;
            costElem.buttonText = "Cost: 0 coins";
            costElem.fontSize = 36.0f;
            costElem.textColor = GNColor(255, 215, 0, 255); // Gold color
            costElem.centerTextHorizontally = true;
            costElem.centerTextVertically = true;
            costElem.visible = false;
            costElem.isEnabled = true;
            costElem.textLayer = 82;
            m_ecsCoordinator->AddComponent<UIElement>(m_skillsCostEntity, costElem);

            Sprite costSprite;
            costSprite.layer = 82;
            costSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_skillsCostEntity, costSprite);
        }

        // Create unlock button (positioned like main menu button in system tab)
        if (m_skillsUnlockButtonEntity == 0) {
            m_skillsUnlockButtonEntity = m_ecsCoordinator->CreateEntity();

            // Position like main menu button (same area as hats/main menu button)
            float buttonCenterX = m_screenWidth * 0.5f; // Center horizontally on screen
            float buttonCenterY;
            if (IsLandscapeMode()) {
                buttonCenterY = m_screenHeight * 0.90f; // Bring up slightly to meet main menu button in middle
            } else {
                buttonCenterY = m_screenHeight * 0.75f; // Position at 75% down screen (lower)
            }

            // Use same scale as other bottom buttons (10x scaling for 900x160 button)
            float buttonScale = 10.0f;
            float buttonWidth = 90.0f * buttonScale;  // 900 pixels
            float buttonHeight = 16.0f * buttonScale; // 160 pixels

            // Use positioning helper to center button
            GNVector2 buttonPosition = CenterObjectAtPosition(buttonCenterX, buttonCenterY, buttonWidth, buttonHeight);
            float buttonX = buttonPosition.x;
            float buttonY = buttonPosition.y;

            Transform buttonTransform(GNVector2(buttonX, buttonY), 0.0f, GNVector2(buttonScale, buttonScale));
            m_ecsCoordinator->AddComponent<Transform>(m_skillsUnlockButtonEntity, buttonTransform);

            // Create sprite using FloppyButtonBlue.png
            Sprite buttonSprite("FloppyButtonBlue.png", 90, 16);
            buttonSprite.layer = 90;
            buttonSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_skillsUnlockButtonEntity, buttonSprite);

            // Create UI element with proper textures
            UIElement buttonElem("BUY", "FloppyButtonBlue", "FloppyButtonBlueHover");
            buttonElem.fontSize = 62.0f;
            buttonElem.textColor = GNColor(255, 255, 255, 255);
            buttonElem.centerTextHorizontally = true;
            buttonElem.centerTextVertically = true;
            buttonElem.textLayer = 91;
            buttonElem.visible = false;
            buttonElem.isEnabled = true;
            buttonElem.normalTextureId = "FloppyButtonBlue"; // Set normal texture ID
            m_ecsCoordinator->AddComponent<UIElement>(m_skillsUnlockButtonEntity, buttonElem);
        }

        // Create left arrow button (small, positioned within black background)
        if (m_skillsLeftArrowEntity == 0) {
            m_skillsLeftArrowEntity = m_ecsCoordinator->CreateEntity();

            // Position arrows with edges aligned at 20% and 80% marks for true symmetry
            float unlockButtonY, costOffset;
            if (IsLandscapeMode()) {
                unlockButtonY = m_screenHeight * 0.85f;
                costOffset = 150.0f; // Closer in landscape
            } else {
                unlockButtonY = m_screenHeight * 0.75f;
                costOffset = 200.0f; // Original spacing
            }
            float costY = unlockButtonY - costOffset; // Cost text Y position
            float arrowY = costY; // Same Y as cost text for perfect centering
            float arrowScale = 8.0f; // Increased scale for better visibility
            float arrowSize = 16.0f * arrowScale; // 128 pixels total size (16x16 * 8)
            // Left arrow's left edge at 20% mark
            float arrowX = m_screenWidth * 0.20f; // 20% from left edge

            Transform arrowTransform(GNVector2(arrowX, arrowY), 0.0f, GNVector2(8.0f, 8.0f)); // Scale 8 for consistency
            m_ecsCoordinator->AddComponent<Transform>(m_skillsLeftArrowEntity, arrowTransform);

            Sprite arrowSprite;
            arrowSprite.textureId = "LeftArrow";
            arrowSprite.width = 16.0f;  // Base texture size (16x16)
            arrowSprite.height = 16.0f; // Base texture size (16x16)
            arrowSprite.layer = 90;
            arrowSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_skillsLeftArrowEntity, arrowSprite);

            // Add UIElement for proper rendering
            UIElement arrowUI("", "LeftArrow", "LeftArrowHover", "LeftArrow");
            arrowUI.visible = false;
            arrowUI.isEnabled = true;
            arrowUI.textLayer = 90;
            arrowUI.normalTextureId = "LeftArrow"; // Set normal texture ID
            m_ecsCoordinator->AddComponent<UIElement>(m_skillsLeftArrowEntity, arrowUI);
        }

        // Create right arrow button (small, positioned within black background)
        if (m_skillsRightArrowEntity == 0) {
            m_skillsRightArrowEntity = m_ecsCoordinator->CreateEntity();

            // Position arrows with edges aligned at 20% and 80% marks for true symmetry
            float unlockButtonY, costOffset;
            if (IsLandscapeMode()) {
                unlockButtonY = m_screenHeight * 0.85f;
                costOffset = 150.0f; // Closer in landscape
            } else {
                unlockButtonY = m_screenHeight * 0.75f;
                costOffset = 200.0f; // Original spacing
            }
            float costY = unlockButtonY - costOffset; // Cost text Y position
            float arrowY = costY; // Same Y as cost text for perfect centering
            float arrowScale = 8.0f; // Increased scale for better visibility
            float arrowSize = 16.0f * arrowScale; // 128 pixels total size (16x16 * 8)
            // Right arrow's right edge at 80% mark (so left edge at 80% - button width)
            float arrowX = m_screenWidth * 0.80f - arrowSize; // 80% minus button width

            Transform arrowTransform(GNVector2(arrowX, arrowY), 0.0f, GNVector2(arrowScale, arrowScale));
            m_ecsCoordinator->AddComponent<Transform>(m_skillsRightArrowEntity, arrowTransform);

            Sprite arrowSprite;
            arrowSprite.textureId = "RightArrow";
            arrowSprite.width = 16.0f;  // Base texture size (16x16)
            arrowSprite.height = 16.0f; // Base texture size (16x16)
            arrowSprite.layer = 90;
            arrowSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_skillsRightArrowEntity, arrowSprite);

            // Add UIElement for proper rendering
            UIElement arrowUI("", "RightArrow", "RightArrowHover", "RightArrow");
            arrowUI.visible = false;
            arrowUI.isEnabled = true;
            arrowUI.textLayer = 90;
            arrowUI.normalTextureId = "RightArrow"; // Set normal texture ID
            m_ecsCoordinator->AddComponent<UIElement>(m_skillsRightArrowEntity, arrowUI);
        }

        GN_LOG_INFO("PauseSystem: Skills tab created");
    }

    void PauseSystem::CreateHatsTab() {
        GN_LOG_INFO("PauseSystem: Creating hats tab");

        // Create "Hats" title text at the very top of the pause menu
        if (m_hatsTitleEntity == 0) {
            m_hatsTitleEntity = m_ecsCoordinator->CreateEntity();

            // Position at the top center of the screen (like other tab titles)
            float titleX = m_screenWidth * 0.5f;
            float titleY;
            if (IsLandscapeMode()) {
                titleY = m_screenHeight * 0.22f; // Lower in landscape to account for tab buttons
            } else {
                titleY = m_screenHeight * 0.15f; // Near the top
            }

            Transform titleTransform(GNVector2(titleX, titleY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_hatsTitleEntity, titleTransform);

            UIElement titleElem;
            titleElem.buttonText = "Hats";
            titleElem.fontSize = 72.0f; // Large title font
            titleElem.textColor = GNColor(255, 255, 255, 255);
            titleElem.centerTextHorizontally = true;
            titleElem.centerTextVertically = true;
            titleElem.visible = false;
            titleElem.isEnabled = true;
            titleElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_hatsTitleEntity, titleElem);

            Sprite titleSprite;
            titleSprite.layer = 90;
            titleSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_hatsTitleEntity, titleSprite);
        }

        // Create black background for hats tab (SAME PATTERN AS SKILLS TAB using UIShape)
        if (m_hatsBackgroundEntity == 0) {
            m_hatsBackgroundEntity = m_ecsCoordinator->CreateEntity();

            // Dynamic percentage-based sizing - constrained to fit within pause menu
            float bgWidth, bgHeight;
            if (IsLandscapeMode()) {
                // Landscape mode: 55% of screen width, 50% of screen height
                bgWidth = m_screenWidth * 0.55f;
                bgHeight = m_screenHeight * 0.55f;
            } else {
                // Portrait mode: 55% of screen width, 50% of screen height
                bgWidth = m_screenWidth * 0.55f;
                bgHeight = m_screenHeight * 0.55f;
            }

            // Calculate layout positions (EXACT same as skills tab)
            float centerX = m_screenWidth * 0.5f;
            float centerY = m_screenHeight * 0.5f + 32.0f;

            // Center position with same offset as pause menu (+32 Y offset)
            GNVector2 bgPosition = CenterObjectAtPosition(centerX, centerY, bgWidth, bgHeight);

            Transform bgTransform(bgPosition, 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_hatsBackgroundEntity, bgTransform);

            // Create black rectangle with slight transparency (EXACT same as skills tab using UIShape)
            UIShape bgShape;
            bgShape.type = UIShapeType::Rectangle;
            bgShape.width = bgWidth;
            bgShape.height = bgHeight;
            bgShape.color = GNColor(0, 0, 0, 200); // Black with ~78% opacity (matches Skills tab)
            bgShape.visible = false; // Initially hidden
            bgShape.layer = 89; // Behind hats (90) but above pause menu background (80)
            m_ecsCoordinator->AddComponent<UIShape>(m_hatsBackgroundEntity, bgShape);

            // Add UIElement component for proper visibility management
            UIElement bgElement;
            bgElement.visible = false; // Initially hidden
            bgElement.isEnabled = true;
            bgElement.textLayer = 89; // Match UIShape layer
            m_ecsCoordinator->AddComponent<UIElement>(m_hatsBackgroundEntity, bgElement);

            Sprite bgSprite;
            bgSprite.layer = 89; // Match textLayer
            bgSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_hatsBackgroundEntity, bgSprite);
            
            GN_LOG_INFO("PauseSystem: Created hats background using UIShape: " + std::to_string(bgWidth) + "x" + std::to_string(bgHeight));
        }

        // Calculate grid dimensions (3x5 grid) - adjusted for better spacing
        float gridWidth, gridHeight, centerY;
        if (IsLandscapeMode()) {
            gridWidth = m_screenWidth * 0.75f;   // Wider in landscape (75% of screen width)
            gridHeight = m_screenHeight * 0.35f; // Shorter in landscape (35% of screen height)
            centerY = m_screenHeight * 0.4f;     // Position higher in landscape (40% down)
        } else {
            gridWidth = m_screenWidth * 0.7f;   // 70% of screen width
            gridHeight = m_screenHeight * 0.4f; // 40% of screen height
            centerY = m_screenHeight * 0.45f;   // 45% down
        }
        float centerX = m_screenWidth * 0.5f;

        // Create hats grid UI elements directly in PauseSystem (like original implementation)
        // HatsSystem provides data, PauseSystem manages UI
        CreateHatsGridUI(centerX, centerY, gridWidth, gridHeight);

        // Create hat action button (Buy/Equip)
        CreateHatActionButton();

        // Create hat cost display
        CreateHatCostDisplay();

        GN_LOG_INFO("PauseSystem: Hats tab created with proper background and grid positioning");
    }

    void PauseSystem::CreateHatsGridUI(float centerX, float centerY, float gridWidth, float gridHeight)
    {
        GN_LOG_INFO("PauseSystem: Creating hats grid UI at center (" + std::to_string(centerX) + ", " + std::to_string(centerY) + ")");

        if (!m_hatsSystem) {
            GN_LOG_WARN("PauseSystem: HatsSystem not available for grid creation");
            return;
        }

        // Calculate grid layout (adjusted for orientation)
        int GRID_ROWS, GRID_COLS;
        if (IsLandscapeMode()) {
            GRID_ROWS = 3;  // Landscape: 3 rows
            GRID_COLS = 5;  // Landscape: 5 columns
        } else {
            GRID_ROWS = 5;  // Portrait: 5 rows
            GRID_COLS = 3;  // Portrait: 3 columns
        }
        const int MAX_HATS = GRID_ROWS * GRID_COLS;

        // Calculate positions for each hat in the grid with better spacing
        std::vector<GNVector2> positions;
        float padding = IsLandscapeMode() ? 110.0f : 30.0f; // Extra padding in landscape for maximum vertical separation between hat frames
        float availableWidth = gridWidth - (padding * (GRID_COLS - 1));
        float availableHeight = gridHeight - (padding * (GRID_ROWS - 1));
        float cellWidth = availableWidth / GRID_COLS;
        float cellHeight = availableHeight / GRID_ROWS;

        // Start grid lower to avoid overlap with labels and provide more vertical spacing
        float gridStartY;
        if (IsLandscapeMode()) {
            gridStartY = centerY - (gridHeight * 0.2f) - (16.0f * 6.0f); // Start even lower in landscape, minus 16px scaled to move UP
        } else {
            gridStartY = centerY - (gridHeight * 0.4f) - (16.0f * 6.0f); // Original positioning in portrait, minus 16px scaled to move UP
        }

        for (int row = 0; row < GRID_ROWS; ++row) {
            for (int col = 0; col < GRID_COLS; ++col) {
                float x = centerX - (gridWidth * 0.5f) + (col * (cellWidth + padding)) + (cellWidth * 0.5f);
                float y = gridStartY + (row * (cellHeight + padding)) + (cellHeight * 0.5f);
                positions.push_back(GNVector2(x, y));
            }
        }

        // Create hat frame and icon entities - MATCH ORIGINAL HATS SYSTEM LOGIC
        int hatCount = m_hatsSystem->GetHatCount();
        for (int i = 0; i < std::min(hatCount, MAX_HATS) && i < (int)positions.size(); ++i) {
            float x = positions[i].x;
            float y = positions[i].y;

            // IMPORTANT: UI iteration uses i=0-14 for the 15 hats
            // But equippedHatIndex/selectedHatIndex use 0=unequipped, 1-15=actual hats
            // So we need to add 1 to i when getting hat data and comparing indices
            int hatIndex = i + 1; // Convert UI index to game index (1-15)
            
            // Get hat data using game index
            auto hatData = m_hatsSystem->GetHatData(hatIndex);
            if (!hatData) continue;

            // Create frame entity - MATCH ORIGINAL LOGIC
            auto frameEntity = m_ecsCoordinator->CreateEntity();

            // Frame texture is 32x32, scaled by 6.0f for UI
            float frameScaledSize = 32.0f * 6.0f;  // 192x192 final size
            GNVector2 framePosition = CenterObjectAtPosition(x, y, frameScaledSize, frameScaledSize);
            Transform frameTransform(framePosition, 0.0f, GNVector2(6.0f, 6.0f));
            m_ecsCoordinator->AddComponent<Transform>(frameEntity, frameTransform);

            // Determine frame texture based on hat status and selection (like original)
            std::string frameTextureId;
            if (hatIndex == m_hatsSystem->GetSelectedHatIndex()) {
                frameTextureId = "HatFrameHover.png"; // Use hover texture for selected hat
            } else if (hatIndex == m_hatsSystem->GetEquippedHatIndex()) {
                frameTextureId = "HatFrameHover.png"; // Use hover texture for equipped hat
            } else {
                frameTextureId = "HatFrame.png"; // Use normal frame texture for all others
            }

            Sprite frameSprite(frameTextureId, 32.0f, 32.0f); // Frame texture is 32x32
            frameSprite.layer = 90; // Behind icons (91) but above Systems tab (89 and below)
            frameSprite.visible = false; // Start invisible, will be shown when tab is activated
            m_ecsCoordinator->AddComponent<Sprite>(frameEntity, frameSprite);

            // Create UI element for the frame (following working Systems tab pattern)
            UIElement frameElement;
            frameElement.buttonText = ""; // No text for frames
            frameElement.fontSize = 0.0f;
            frameElement.visible = false; // Start invisible, will be shown when tab is activated
            frameElement.isEnabled = true;
            frameElement.textLayer = 90; // Match sprite layer
            frameElement.normalTextureId = frameTextureId; // Add normal texture ID for frame
            m_ecsCoordinator->AddComponent<UIElement>(frameEntity, frameElement);

            // Store frame entity for later access
            if ((int)m_hatFrameEntities.size() <= i) {
                m_hatFrameEntities.resize(i + 1);
            }
            m_hatFrameEntities[i] = frameEntity;

            // Create icon entity - MATCH ORIGINAL LOGIC
            auto iconEntity = m_ecsCoordinator->CreateEntity();

            // CRITICAL: Hat icons are 16x16 textures, frames are 32x32
            // Icon: 16x16 texture * 6.0f scale = 96x96 final size
            // Frame: 32x32 texture * 6.0f scale = 192x192 final size
            // Both centered at same (x, y) point - CenterObjectAtPosition handles the size difference
            float iconScaledSize = 16.0f * 6.0f;  // 96x96 final size
            GNVector2 iconPosition = CenterObjectAtPosition(x, y, iconScaledSize, iconScaledSize);
            Transform iconTransform(iconPosition, 0.0f, GNVector2(6.0f, 6.0f));
            m_ecsCoordinator->AddComponent<Transform>(iconEntity, iconTransform);

            // Create sprite for the hat icon - texture is 16x16
            Sprite iconSprite(hatData->iconPath, 16.0f, 16.0f);
            iconSprite.layer = 91; // ABOVE regular frames (90) to avoid layer conflicts
            iconSprite.visible = false; // Start invisible, will be shown when tab is activated
            m_ecsCoordinator->AddComponent<Sprite>(iconEntity, iconSprite);

            // Add UI element component to make this screen-space (following working Systems tab pattern)
            UIElement iconElement;
            iconElement.visible = false; // Start invisible, will be shown when tab is activated
            iconElement.isEnabled = false; // Not interactive
            iconElement.textLayer = 91; // ABOVE Systems tab (90) to avoid layer conflicts
            iconElement.normalTextureId = hatData->iconPath; // Add normal texture ID for hat icon
            m_ecsCoordinator->AddComponent<UIElement>(iconEntity, iconElement);

            // Store icon entity
            if ((int)m_hatIconEntities.size() <= i) {
                m_hatIconEntities.resize(i + 1);
            }
            m_hatIconEntities[i] = iconEntity;

            // Create locked overlay frame if hat is locked - MATCH ORIGINAL LOGIC
            if (hatData->status == HatStatus::LOCKED) {
                GN_LOG_INFO("PauseSystem: Creating locked overlay frame for locked hat '" + hatData->name + "'");

                // Create locked frame entity at same position with higher layer
                auto lockedFrameEntity = m_ecsCoordinator->CreateEntity();
                float lockedFrameSize = 32.0f * 6.0f;  // 192x192 final size (same as frame)
                GNVector2 lockedFramePosition = CenterObjectAtPosition(x, y, lockedFrameSize, lockedFrameSize);
                Transform lockedFrameTransform(lockedFramePosition, 0.0f, GNVector2(6.0f, 6.0f));
                m_ecsCoordinator->AddComponent<Transform>(lockedFrameEntity, lockedFrameTransform);

                Sprite lockedFrameSprite("HatFrameLocked.png", 32.0f, 32.0f);
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
                if ((int)m_lockedFrameEntities.size() <= i) {
                    m_lockedFrameEntities.resize(i + 1);
                }
                m_lockedFrameEntities[i] = lockedFrameEntity;

                GN_LOG_INFO("PauseSystem: Created locked frame overlay entity for hat '" + hatData->name + "' (index " + std::to_string(i) + ")");
            }

            GN_LOG_INFO("PauseSystem: Created hat " + std::to_string(i) + " '" + hatData->name + "' at position (" +
                       std::to_string(x) + ", " + std::to_string(y) + ") with frame texture '" + frameTextureId + "'");
        }

        GN_LOG_INFO("PauseSystem: Created hats grid with " + std::to_string(m_hatFrameEntities.size()) + " frames");
    }

    void PauseSystem::CreateHatActionButton()
    {
        GN_LOG_INFO("PauseSystem: Creating hat action button");

        if (m_hatsActionButtonEntity == 0) {
            m_hatsActionButtonEntity = m_ecsCoordinator->CreateEntity();

            // Position below the grid (same as main menu button)
            float buttonCenterX = m_screenWidth * 0.5f; // Center horizontally on screen
            float buttonCenterY;
            if (IsLandscapeMode()) {
                buttonCenterY = m_screenHeight * 0.90f; // Bring up slightly to meet main menu button in middle
            } else {
                buttonCenterY = m_screenHeight * 0.75f; // Position at 75% down screen (same as main menu)
            }

            // Use same scale as main menu button (10x scaling for 900x160 button)
            float buttonScale = 10.0f;
            float buttonWidth = 90.0f * buttonScale;  // 900 pixels (like working buttons)
            float buttonHeight = 16.0f * buttonScale; // 160 pixels (like working buttons)

            // Use positioning helper to center button (EXACT same pattern as main menu buttons)
            GNVector2 buttonPosition = CenterObjectAtPosition(buttonCenterX, buttonCenterY, buttonWidth, buttonHeight);
            float buttonX = buttonPosition.x;  // This is the TOP-LEFT X position for the sprite
            float buttonY = buttonPosition.y;  // This is the TOP-LEFT Y position for the sprite

            Transform buttonTransform(GNVector2(buttonX, buttonY), 0.0f, GNVector2(buttonScale, buttonScale));
            m_ecsCoordinator->AddComponent<Transform>(m_hatsActionButtonEntity, buttonTransform);

            // Create sprite using same pattern as main menu button
            Sprite buttonSprite("FloppyButtonBlue.png", 90, 16); // Use actual texture dimensions
            buttonSprite.layer = 91; // Above main menu button layer
            buttonSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_hatsActionButtonEntity, buttonSprite);

            // Create UI element with same pattern as main menu button
            UIElement buttonElement("Nothing Selected", "FloppyButtonBlue", "FloppyButtonBlueHover");
            buttonElement.fontSize = 62.0f; // Same font size as main menu button
            buttonElement.textColor = GNColor(150, 150, 150, 255); // Gray for no selection
            buttonElement.centerTextHorizontally = true;
            buttonElement.centerTextVertically = true;
            buttonElement.textLayer = 91; // Same layer as button sprite
            buttonElement.visible = false;
            buttonElement.isEnabled = false;
            buttonElement.normalTextureId = "FloppyButtonBlue"; // Add texture ID
            buttonElement.textOffsetX = 0.0f;  // No horizontal offset for perfect centering
            buttonElement.textOffsetY = 0.0f;  // No vertical offset for perfect centering
            m_ecsCoordinator->AddComponent<UIElement>(m_hatsActionButtonEntity, buttonElement);

            GN_LOG_INFO("PauseSystem: Created hat action button at top-left (" + std::to_string(buttonX) + ", " + std::to_string(buttonY) + ") with scale " + std::to_string(buttonScale));
        }
    }

    void PauseSystem::CreateHatCostDisplay()
    {
        GN_LOG_INFO("PauseSystem: Creating hat cost display");

        if (m_hatsCostDisplayEntity == 0) {
            m_hatsCostDisplayEntity = m_ecsCoordinator->CreateEntity();

            // Position above the action button - lower it more for better spacing
            float textX = m_screenWidth * 0.5f;
            float textY;
            if (IsLandscapeMode()) {
                textY = m_screenHeight * 0.80f; // Higher up (less low) for better visual balance
            } else {
                textY = m_screenHeight * 0.70f; // Above the button
            }

            Transform textTransform(GNVector2(textX, textY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_hatsCostDisplayEntity, textTransform);

            UIElement textElement("Select a hat to see cost", "");
            textElement.visible = false;

            textElement.isEnabled = false;
            textElement.textLayer = 90;
            textElement.fontSize = 32.0f;
            textElement.centerTextHorizontally = true;
            textElement.textColor = GNColor(255, 255, 255, 255);
            m_ecsCoordinator->AddComponent<UIElement>(m_hatsCostDisplayEntity, textElement);

            Sprite textSprite;
            textSprite.layer = 90;
            textSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_hatsCostDisplayEntity, textSprite);

            GN_LOG_INFO("PauseSystem: Created hat cost display at (" + std::to_string(textX) + ", " + std::to_string(textY) + ")");
        }
    }

    void PauseSystem::UpdateHatDisplay()
    {
        if (!m_hatsSystem) return;

        // Sync player coins to ensure we have the latest coin count
        SyncPlayerCoins();

        int selectedHatIndex = m_hatsSystem->GetSelectedHatIndex();

        // Update cost display and action button
        // Note: index 0 = unequipped/no hat, 1-15 = actual hats
        if (selectedHatIndex <= 0 || selectedHatIndex > m_hatsSystem->GetHatCount()) {
            // No hat selected or invalid index
            if (m_hatsCostDisplayEntity != 0 && m_ecsCoordinator) {
                UIElement* costElement = m_ecsCoordinator->GetComponent<UIElement>(m_hatsCostDisplayEntity);
                if (costElement) {
                    costElement->buttonText = "Select a hat to see cost";
                    costElement->textColor = GNColor(255, 255, 255, 255);
                }
            }

            if (m_hatsActionButtonEntity != 0 && m_ecsCoordinator) {
                UIElement* actionElement = m_ecsCoordinator->GetComponent<UIElement>(m_hatsActionButtonEntity);
                if (actionElement) {
                    actionElement->buttonText = "Nothing Selected";
                    actionElement->isEnabled = false;
                    actionElement->textColor = GNColor(150, 150, 150, 255); // Gray for no selection
                }
            }
        } else {
            // Hat selected - show cost and action
            auto hatData = m_hatsSystem->GetHatData(selectedHatIndex);
            if (hatData) {
                if (m_hatsCostDisplayEntity != 0 && m_ecsCoordinator) {
                    UIElement* costElement = m_ecsCoordinator->GetComponent<UIElement>(m_hatsCostDisplayEntity);
                    if (costElement) {
                        if (m_hatsSystem->IsHatUnlocked(selectedHatIndex)) {
                            costElement->buttonText = hatData->name + " - Unlocked";
                            costElement->textColor = GNColor(100, 255, 100, 255); // Green for unlocked
                        } else {
                            costElement->buttonText = hatData->name + " - Cost: " + std::to_string(hatData->cost) + " coins";
                            costElement->textColor = GNColor(255, 255, 100, 255); // Yellow for locked
                        }
                    }
                }

                if (m_hatsActionButtonEntity != 0 && m_ecsCoordinator) {
                    UIElement* actionElement = m_ecsCoordinator->GetComponent<UIElement>(m_hatsActionButtonEntity);
                    if (actionElement) {
                        if (m_hatsSystem->IsHatUnlocked(selectedHatIndex)) {
                            if (selectedHatIndex == m_hatsSystem->GetEquippedHatIndex()) {
                                actionElement->buttonText = "Unequip";
                                actionElement->isEnabled = true;
                                actionElement->textColor = GNColor(255, 200, 100, 255); // Orange for unequip
                            } else {
                                actionElement->buttonText = "Equip";
                                actionElement->isEnabled = true;
                                actionElement->textColor = GNColor(255, 255, 255, 255);
                            }
                        } else {
                            if (m_playerCoins >= hatData->cost) {
                                actionElement->buttonText = "Buy";
                                actionElement->isEnabled = true;
                                actionElement->textColor = GNColor(255, 255, 255, 255);
                            } else {
                                actionElement->buttonText = "Not enough coins";
                                actionElement->isEnabled = true; // Keep enabled so denied sound can play
                                actionElement->textColor = GNColor(255, 100, 100, 255); // Red for insufficient coins
                            }
                        }
                    }
                }
            }
        }

        // Update frame visuals
        UpdateHatFrameVisuals();
    }

    void PauseSystem::SyncPlayerCoins()
    {
        // Sync player coins from ECS component to ensure we have the latest coin count
        // Combine both session coins and total coins for purchasing
        if (m_playerEntity != 0 && m_ecsCoordinator) {
            auto* playerComp = m_ecsCoordinator->GetComponent<PlayerComponent>(m_playerEntity);
            if (playerComp) {
                int oldCoins = m_playerCoins;
                m_playerCoins = playerComp->sessionCoins + playerComp->totalCoins;
                GN_LOG_INFO("PauseSystem: Synced player coins (session: " + std::to_string(playerComp->sessionCoins) +
                           ", total: " + std::to_string(playerComp->totalCoins) +
                           ", combined: " + std::to_string(m_playerCoins) +
                           ", old: " + std::to_string(oldCoins) + ")");
            } else {
                GN_LOG_ERROR("PauseSystem: Failed to get player component for coin sync!");
            }
        } else {
            GN_LOG_ERROR("PauseSystem: Cannot sync coins - playerEntity: " + std::to_string(m_playerEntity) +
                        ", ecsCoordinator: " + (m_ecsCoordinator ? "valid" : "null"));
        }
    }

    void PauseSystem::UpdateHatFrameVisuals()
    {
        if (!m_hatsSystem) return;

        // Update all frame textures based on current selection and equipped state
        for (size_t i = 0; i < m_hatFrameEntities.size(); ++i) {
            auto frameEntity = m_hatFrameEntities[i];
            if (frameEntity != 0 && m_ecsCoordinator) {
                auto* sprite = m_ecsCoordinator->GetComponent<Sprite>(frameEntity);
                auto* uiElement = m_ecsCoordinator->GetComponent<UIElement>(frameEntity);

                if (sprite && uiElement) {
                    // Convert UI index to game index for comparisons (i=0-14 -> hatIndex=1-15)
                    int hatIndex = i + 1;
                    std::string frameTextureId;
                    if (hatIndex == m_hatsSystem->GetSelectedHatIndex()) {
                        frameTextureId = "HatFrameHover.png"; // Use hover texture for selected hat
                    } else if (hatIndex == m_hatsSystem->GetEquippedHatIndex()) {
                        frameTextureId = "HatFrameHover.png"; // Use hover texture for equipped hat
                    } else {
                        frameTextureId = "HatFrame.png"; // Use normal frame texture for all others
                    }

                    // Update both sprite and UI element textures
                    sprite->textureId = frameTextureId;
                    uiElement->normalTextureId = frameTextureId;

                    GN_LOG_INFO("PauseSystem: Updated frame " + std::to_string(i) + " texture to '" + frameTextureId + "'");
                }
            }
            
            // Update locked frame overlay visibility based on hat unlock status
            if (i < m_lockedFrameEntities.size()) {
                auto lockedFrameEntity = m_lockedFrameEntities[i];
                if (lockedFrameEntity != 0 && m_ecsCoordinator) {
                    // Convert UI index to game index (i=0-14 -> hatIndex=1-15)
                    int hatIndex = i + 1;
                    // Check if this hat is unlocked
                    bool isUnlocked = m_hatsSystem->IsHatUnlocked(hatIndex);
                    
                    // Hide locked frame if hat is unlocked, show if locked
                    auto* lockedSprite = m_ecsCoordinator->GetComponent<Sprite>(lockedFrameEntity);
                    auto* lockedUI = m_ecsCoordinator->GetComponent<UIElement>(lockedFrameEntity);
                    
                    if (lockedSprite) {
                        lockedSprite->visible = !isUnlocked;
                    }
                    if (lockedUI) {
                        lockedUI->visible = !isUnlocked;
                    }
                    
                    if (isUnlocked) {
                        GN_LOG_INFO("PauseSystem: Hidden locked frame overlay for unlocked hat at index " + std::to_string(i));
                    }
                }
            }
        }
    }

    void PauseSystem::CreateStatsTab() {
        GN_LOG_INFO("PauseSystem: Creating stats tab");

        // Create "Stats" title text at the very top of the pause menu
        if (m_statsTitleEntity == 0) {
            m_statsTitleEntity = m_ecsCoordinator->CreateEntity();

            // Position at the top center of the screen (like other tab titles)
            float titleX = m_screenWidth * 0.5f;
            float titleY;
            if (IsLandscapeMode()) {
                titleY = m_screenHeight * 0.22f; // Lower in landscape to account for tab buttons
            } else {
                titleY = m_screenHeight * 0.15f; // Near the top
            }

            Transform titleTransform(GNVector2(titleX, titleY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_statsTitleEntity, titleTransform);

            UIElement titleElem;
            titleElem.buttonText = "Stats";
            titleElem.fontSize = 72.0f; // Large title font
            titleElem.textColor = GNColor(255, 255, 255, 255);
            titleElem.centerTextHorizontally = true;
            titleElem.centerTextVertically = true;
            titleElem.visible = false;
            titleElem.isEnabled = true;
            titleElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_statsTitleEntity, titleElem);

            Sprite titleSprite;
            titleSprite.layer = 90;
            titleSprite.visible = false;
            m_ecsCoordinator->AddComponent<Sprite>(m_statsTitleEntity, titleSprite);
        }

        // Dynamic percentage-based sizing - constrained to fit within pause menu
        float bgWidth, bgHeight;
        if (IsLandscapeMode()) {
            // Landscape mode: 55% of screen width, 55% of screen height
            bgWidth = m_screenWidth * 0.55f;
            bgHeight = m_screenHeight * 0.55f;
        } else {
            // Portrait mode: 55% of screen width, 55% of screen height
            bgWidth = m_screenWidth * 0.55f;
            bgHeight = m_screenHeight * 0.55f;
        }

        // Calculate layout positions
        float centerX = m_screenWidth * 0.5f;
        float centerY = m_screenHeight * 0.5f + 32.0f;

        // Create black rectangle background for stats tab (70% of pause menu background size)
        if (m_statsBackgroundEntity == 0) {
            m_statsBackgroundEntity = m_ecsCoordinator->CreateEntity();

            // Center position with same offset as pause menu (+32 Y offset)
            GNVector2 bgPosition = CenterObjectAtPosition(centerX, centerY, bgWidth, bgHeight);

            Transform bgTransform(bgPosition, 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_statsBackgroundEntity, bgTransform);

            // Create black rectangle with slight transparency
            UIShape bgShape;
            bgShape.type = UIShapeType::Rectangle;
            bgShape.width = bgWidth;
            bgShape.height = bgHeight;
            bgShape.color = GNColor(0, 0, 0, 200); // Black with ~78% opacity (200/255)
            bgShape.visible = false; // Initially hidden
            bgShape.layer = 82; // Above pause menu background (80) but below content (90)
            m_ecsCoordinator->AddComponent<UIShape>(m_statsBackgroundEntity, bgShape);

            // Add UIElement component for proper visibility management
            UIElement bgElement;
            bgElement.visible = false; // Initially hidden
            bgElement.isEnabled = true;
            bgElement.textLayer = 79; // Match UIShape layer
            m_ecsCoordinator->AddComponent<UIElement>(m_statsBackgroundEntity, bgElement);

            GN_LOG_INFO("PauseSystem: Created stats background rectangle at (" + std::to_string(bgPosition.x) + ", " + std::to_string(bgPosition.y) + ") size " + std::to_string(bgWidth) + "x" + std::to_string(bgHeight));
        }

        float startY;
        if (IsLandscapeMode()) {
            // Landscape: start higher up (moved from -0.25f to -0.35f) to bring content up
            startY = centerY - (bgHeight * 0.35f); // Start higher in landscape
        } else {
            startY = centerY - (bgHeight * 0.4f); // Start near top of rectangle in portrait
        }
        float lineSpacing = 80.0f; // Space between stats
        float fontSize = 42.0f; // Increased from 32.0f for better readability

        // Create individual stat text entities
        // 1. Current Session Pipes
        if (m_currentSessionTextEntity == 0) {
            m_currentSessionTextEntity = m_ecsCoordinator->CreateEntity();
            Transform transform(GNVector2(centerX, startY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_currentSessionTextEntity, transform);

            UIElement uiElem;
            uiElem.buttonText = "Session Pipes: 0"; // Will be updated when shown
            uiElem.fontSize = fontSize;
            uiElem.textColor = GNColor(255, 255, 255, 255);
            uiElem.centerTextHorizontally = true;
            uiElem.visible = false;
            uiElem.isEnabled = true;
            uiElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_currentSessionTextEntity, uiElem);
        }

        // 2. Session Coins
        if (m_sessionCoinsTextEntity == 0) {
            m_sessionCoinsTextEntity = m_ecsCoordinator->CreateEntity();
            Transform transform(GNVector2(centerX, startY + lineSpacing), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_sessionCoinsTextEntity, transform);

            UIElement uiElem;
            uiElem.buttonText = "Session Coins: 0"; // Will be updated when shown
            uiElem.fontSize = fontSize;
            uiElem.textColor = GNColor(255, 255, 100, 255); // Light gold for session coins
            uiElem.centerTextHorizontally = true;
            uiElem.visible = false;
            uiElem.isEnabled = true;
            uiElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_sessionCoinsTextEntity, uiElem);
        }

        // 3. Total Coins
        if (m_totalCoinsTextEntity == 0) {
            m_totalCoinsTextEntity = m_ecsCoordinator->CreateEntity();
            Transform transform(GNVector2(centerX, startY + lineSpacing * 2), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_totalCoinsTextEntity, transform);

            UIElement uiElem;
            uiElem.buttonText = "Current Total Coins: 0"; // Will be updated when shown (total spendable coins: stored + session)
            uiElem.fontSize = fontSize;
            uiElem.textColor = GNColor(255, 215, 0, 255); // Gold color for coins
            uiElem.centerTextHorizontally = true;
            uiElem.visible = false;
            uiElem.isEnabled = true;
            uiElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_totalCoinsTextEntity, uiElem);
        }

        // 4. Gross Total Coins
        if (m_grossTotalCoinsTextEntity == 0) {
            m_grossTotalCoinsTextEntity = m_ecsCoordinator->CreateEntity();
            Transform transform(GNVector2(centerX, startY + lineSpacing * 3), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_grossTotalCoinsTextEntity, transform);

            UIElement uiElem;
            uiElem.buttonText = "Gross Total Coins: 0"; // Will be updated when shown (lifetime accumulation)
            uiElem.fontSize = fontSize;
            uiElem.textColor = GNColor(200, 255, 200, 255); // Light gold-green for lifetime record
            uiElem.centerTextHorizontally = true;
            uiElem.visible = false;
            uiElem.isEnabled = true;
            uiElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_grossTotalCoinsTextEntity, uiElem);
        }

        // 5. Total Flops
        if (m_totalFlopsTextEntity == 0) {
            m_totalFlopsTextEntity = m_ecsCoordinator->CreateEntity();
            Transform transform(GNVector2(centerX, startY + lineSpacing * 4), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_totalFlopsTextEntity, transform);

            UIElement uiElem;
            uiElem.buttonText = "Total Flops: 0"; // Will be updated when shown
            uiElem.fontSize = fontSize;
            uiElem.textColor = GNColor(150, 255, 150, 255); // Light green for victories
            uiElem.centerTextHorizontally = true;
            uiElem.visible = false;
            uiElem.isEnabled = true;
            uiElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_totalFlopsTextEntity, uiElem);
        }

        // 6. Enemies Killed
        if (m_enemiesKilledTextEntity == 0) {
            m_enemiesKilledTextEntity = m_ecsCoordinator->CreateEntity();
            Transform transform(GNVector2(centerX, startY + lineSpacing * 5), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_enemiesKilledTextEntity, transform);

            UIElement uiElem;
            uiElem.buttonText = "Enemies Killed: 0"; // Will be updated when shown
            uiElem.fontSize = fontSize;
            uiElem.textColor = GNColor(255, 100, 100, 255); // Light red for deaths
            uiElem.centerTextHorizontally = true;
            uiElem.visible = false;
            uiElem.isEnabled = true;
            uiElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_enemiesKilledTextEntity, uiElem);
        }

        // 7. Total Pipes
        if (m_totalPipesTextEntity == 0) {
            m_totalPipesTextEntity = m_ecsCoordinator->CreateEntity();
            Transform transform(GNVector2(centerX, startY + lineSpacing * 6), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_totalPipesTextEntity, transform);

            UIElement uiElem;
            uiElem.buttonText = "Total Pipes: 0"; // Will be updated when shown
            uiElem.fontSize = fontSize;
            uiElem.textColor = GNColor(150, 150, 255, 255); // Light blue for pipes
            uiElem.centerTextHorizontally = true;
            uiElem.visible = false;
            uiElem.isEnabled = true;
            uiElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_totalPipesTextEntity, uiElem);
        }

        // 8. Rat King Boss Timer (Level 6 only)
        if (m_bossTimerTextEntity == 0) {
            m_bossTimerTextEntity = m_ecsCoordinator->CreateEntity();
            Transform transform(GNVector2(centerX, startY + lineSpacing * 7), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(m_bossTimerTextEntity, transform);

            UIElement uiElem;
            uiElem.buttonText = "Rat King Timer: 00:00"; // Will be updated when shown
            uiElem.fontSize = fontSize;
            uiElem.textColor = GNColor(255, 215, 0, 255); // Gold color for boss timer
            uiElem.centerTextHorizontally = true;
            uiElem.visible = false;
            uiElem.isEnabled = true;
            uiElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(m_bossTimerTextEntity, uiElem);
        }

        // 9. Level High Scores Section - Add a separator and then each level's high score
        // Position these at the bottom of the stats background
        float levelScoresStartY = startY + lineSpacing * 9; // Start after the 8 stats above (including boss timer)
        float levelTitleSpacing = 100.0f; // Extra spacing between title and first level
        float levelLineSpacing = lineSpacing; // Same spacing as main stats (80.0f)
        float levelFontSize = fontSize; // Same as main stats (42.0f)
        
        // Create a title/separator for level high scores
        if (m_levelHighScoreTextEntities.empty()) {
            // Reserve space for title + 6 levels
            m_levelHighScoreTextEntities.reserve(7);
            
            // Create "Level High Scores" title (without dashes)
            Entity titleEntity = m_ecsCoordinator->CreateEntity();
            Transform titleTransform(GNVector2(centerX, levelScoresStartY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsCoordinator->AddComponent<Transform>(titleEntity, titleTransform);
            
            UIElement titleElem;
            titleElem.buttonText = "Level High Scores";
            titleElem.fontSize = 48.0f; // Larger than main stats (42.0f)
            titleElem.textColor = GNColor(200, 200, 100, 255); // Yellowish for section header
            titleElem.centerTextHorizontally = true;
            titleElem.visible = false;
            titleElem.isEnabled = true;
            titleElem.textLayer = 90;
            m_ecsCoordinator->AddComponent<UIElement>(titleEntity, titleElem);
            m_levelHighScoreTextEntities.push_back(titleEntity);
            
            // Create text entities for each of the 6 levels
            const char* levelNames[] = {"Park", "Sewer", "Desert", "Snow", "Castle", "Boss"};
            for (int i = 0; i < 6; i++) {
                Entity levelEntity = m_ecsCoordinator->CreateEntity();
                // Add extra spacing after title, then use regular spacing between levels
                float yPos = levelScoresStartY + levelTitleSpacing + (levelLineSpacing * i);
                Transform transform(GNVector2(centerX, yPos), 0.0f, GNVector2(1.0f, 1.0f));
                m_ecsCoordinator->AddComponent<Transform>(levelEntity, transform);
                
                UIElement uiElem;
                // Boss (level 6) will show time instead of score - will be updated when shown
                if (i == 5) { // Boss level
                    uiElem.buttonText = "Boss: 0:00";
                } else {
                    uiElem.buttonText = std::string(levelNames[i]) + ": 0 pipes";
                }
                uiElem.fontSize = levelFontSize;
                uiElem.textColor = GNColor(180, 180, 255, 255); // Light blue-ish for level scores
                uiElem.centerTextHorizontally = true;
                uiElem.visible = false;
                uiElem.isEnabled = true;
                uiElem.textLayer = 90;
                m_ecsCoordinator->AddComponent<UIElement>(levelEntity, uiElem);
                
                m_levelHighScoreTextEntities.push_back(levelEntity);
            }
            
            GN_LOG_INFO("PauseSystem: Created level high score entities for 6 levels");
        }

        GN_LOG_INFO("PauseSystem: Created stats tab content");
    }

    void PauseSystem::CreateAudioSliders() {
        GN_LOG_INFO("PauseSystem: Creating audio sliders");

        // Calculate screen center for portrait mode centering
        float centerX = m_screenWidth * 0.5f;

        // Calculate background area (EXACT same as original)
        float bgW = m_screenWidth * 0.8f;
        float bgH = m_screenHeight * 0.7f;
        float bgX = (m_screenWidth - bgW) * 0.5f;
        float bgY = (m_screenHeight - bgH) * 0.18f;

        // Mobile detection like MainMenuState - iPhone typically has width < height in portrait
        bool isMobile = (m_screenWidth < m_screenHeight) && (m_screenHeight > 1000);
        float baseUiScale = isMobile ? 8.0f : 1.0f;  // Store as local variable
        // Use scale of 8 for knobs, even larger for better visibility
        float uiScale = 8.0f;
        GN_LOG_INFO("PauseSystem: Mobile detection: isMobile=" + std::to_string(isMobile) + ", uiScale=" + std::to_string(uiScale) +
                   " (screen: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight) + ")");

        // Slider layout (adjusted for landscape mode)
        float sliderStartY;
        if (IsLandscapeMode()) {
            // Landscape: start lower than Systems tab label, even more condensed for main menu button
            m_sliderX = bgX + 0.15f * bgW + 16.0f; // Start further right (15% + 16px)
            sliderStartY = bgY + 0.45f * bgH; // Start lower to be below Systems tab label
            m_sliderW = (bgW - 0.30f * bgW - 32.0f) * 0.6f; // Reduce track length to 60% of original for tighter range
            m_sliderSpacing = 200.0f; // Even more condensed spacing for landscape to fit main menu button
        } else {
            // Portrait: original positioning
            m_sliderX = bgX + 0.20f * bgW + 32.0f; // Start further right (20% + 32px) to avoid tab buttons
            sliderStartY = bgY + 0.35f * bgH; // Start much lower to move everything down
            m_sliderW = bgW - 0.40f * bgW - 64.0f; // Reduce width proportionally to account for rightward movement
            m_sliderSpacing = 320.0f; // Much more spacing between slider groups for better separation
        }
        m_sliderY = sliderStartY;
        m_sliderH = 18.0f;

        // MASTER SLIDER (first) - Center tracks vertically with knobs
        float masterTrackY = m_sliderY + 9.0f; // Track Y position
        float musicTrackY = m_sliderY + m_sliderSpacing + 9.0f; // Track Y position
        float sfxTrackY = m_sliderY + m_sliderSpacing * 2 + 9.0f; // Track Y position

        // MASTER TRACK - Use UIShape for proper rendering (EXACT same as original)
        if (m_masterTrackEntity == 0) {
            m_masterTrackEntity = m_ecsCoordinator->CreateEntity();
            Transform t(GNVector2(m_sliderX, masterTrackY), 0.0f, GNVector2(1.0f, 1.0f));

            UIShape shape;
            shape.visible = false;
            shape.width = m_sliderW;
            shape.height = m_sliderH;
            shape.color = GNColor(128, 128, 128, 255); // Gray color like main menu options
            shape.layer = 83; // Below knob (85) but above background (80)

            UIElement ui;
            ui.visible = false;
            ui.textLayer = 83; // Match UIShape layer for consistent layering
            ui.isEnabled = true;

            GN_LOG_INFO("PauseSystem: Creating MASTER track UIShape: " + std::to_string(m_sliderW) + "x" + std::to_string(m_sliderH) +
                       " at (" + std::to_string(m_sliderX) + ", " + std::to_string(masterTrackY) + ") layer " + std::to_string(shape.layer) +
                       " entity=" + std::to_string(m_masterTrackEntity));

            m_ecsCoordinator->AddComponent<Transform>(m_masterTrackEntity, t);
            m_ecsCoordinator->AddComponent<UIShape>(m_masterTrackEntity, shape);
            m_ecsCoordinator->AddComponent<UIElement>(m_masterTrackEntity, ui);
        }

        // MASTER LABEL
        if (m_masterLabelEntity == 0) {
            m_masterLabelEntity = m_ecsCoordinator->CreateEntity();
            float labelSpacing;
            if (IsLandscapeMode()) {
                labelSpacing = 120.0f; // Condensed spacing in landscape
            } else {
                labelSpacing = 160.0f; // Even more spacing between label and track for better symmetry
            }
            float labelY = masterTrackY - labelSpacing;
            // In landscape, move labels further left to avoid knob overlap; in portrait, also move them left to avoid ribbon button overlap
            float labelX = IsLandscapeMode() ? (m_sliderX - 250.0f) : (m_sliderX - 50.0f); // Moved from -200 to -250 in landscape
            Transform t(GNVector2(labelX, labelY), 0.0f, GNVector2(1.0f, 1.0f));
            UIElement ui("MASTER", "", "");
            ui.fontSize = IsLandscapeMode() ? 38.0f : 42.0f; // Slightly smaller in landscape
            ui.textColor = GNColor(255, 255, 255, 255);
            ui.centerTextHorizontally = false; ui.centerTextVertically = true; ui.visible = false; ui.textLayer = 84;
            m_ecsCoordinator->AddComponent<Transform>(m_masterLabelEntity, t);
            m_ecsCoordinator->AddComponent<UIElement>(m_masterLabelEntity, ui);
        }

        // MASTER KNOB
        if (m_masterKnobEntity == 0) {
            m_masterKnobEntity = m_ecsCoordinator->CreateEntity();
            // Use MainMenuState knob sizing approach - consistent UI scaling
            float knobSize = 16.0f * uiScale;  // Same as MainMenuState
            float scale = uiScale;  // Use UI scale instead of hardcoded 6.0f
            float scaledKnobSize = knobSize;

            // Clamp volume value to 0.0-1.0 range like Options menu
            float masterVolume = std::max(0.0f, std::min(1.0f, m_masterSliderValue));

            // Position knob to use full track range - knob centers move within track boundaries
            // At 0%: knob center at track start + half knob, at 100%: knob center at track end - half knob
            float knobCenterX = m_sliderX + (scaledKnobSize * 0.5f) + masterVolume * (m_sliderW - scaledKnobSize);
            float knobX = knobCenterX - (scaledKnobSize * 0.5f);
            float knobY = masterTrackY + m_sliderH * 0.5f - (scaledKnobSize * 0.5f);

            Transform t(GNVector2(knobX, knobY), 0.0f, GNVector2(scale, scale));
            Sprite s("poophat", 16, 16); s.layer = 85; s.visible = false;
            UIElement ui("", "poophat", "poophat");
            ui.visible = false; ui.textLayer = 85; ui.isEnabled = true;
            m_ecsCoordinator->AddComponent<Transform>(m_masterKnobEntity, t);
            m_ecsCoordinator->AddComponent<Sprite>(m_masterKnobEntity, s);
            m_ecsCoordinator->AddComponent<UIElement>(m_masterKnobEntity, ui);
        }

        // MUSIC TRACK - Use UIShape for proper rendering (EXACT same as original)
        if (m_musicTrackEntity == 0) {
            m_musicTrackEntity = m_ecsCoordinator->CreateEntity();
            Transform t(GNVector2(m_sliderX, musicTrackY), 0.0f, GNVector2(1.0f, 1.0f));

            UIShape shape;
            shape.visible = false;
            shape.width = m_sliderW;
            shape.height = m_sliderH;
            shape.color = GNColor(128, 128, 128, 255); // Gray color like main menu options
            shape.layer = 83; // Below knob (85) but above background (80)

            UIElement ui;
            ui.visible = false;
            ui.textLayer = 83; // Match UIShape layer for consistent layering
            ui.isEnabled = true;

            GN_LOG_INFO("PauseSystem: Creating MUSIC track UIShape: " + std::to_string(m_sliderW) + "x" + std::to_string(m_sliderH) +
                       " at (" + std::to_string(m_sliderX) + ", " + std::to_string(musicTrackY) + ") layer " + std::to_string(shape.layer) +
                       " entity=" + std::to_string(m_musicTrackEntity));

            m_ecsCoordinator->AddComponent<Transform>(m_musicTrackEntity, t);
            m_ecsCoordinator->AddComponent<UIShape>(m_musicTrackEntity, shape);
            m_ecsCoordinator->AddComponent<UIElement>(m_musicTrackEntity, ui);
        }

        // MUSIC LABEL
        if (m_musicLabelEntity == 0) {
            m_musicLabelEntity = m_ecsCoordinator->CreateEntity();
            float labelSpacing = IsLandscapeMode() ? 120.0f : 160.0f; // Condensed spacing in landscape
            float labelY = musicTrackY - labelSpacing;
            // In landscape, move labels further left to avoid knob overlap; in portrait, also move them left to avoid ribbon button overlap
            float labelX = IsLandscapeMode() ? (m_sliderX - 250.0f) : (m_sliderX - 50.0f); // Moved from -200 to -250 in landscape
            Transform t(GNVector2(labelX, labelY), 0.0f, GNVector2(1.0f, 1.0f));
            UIElement ui("MUSIC", "", "");
            ui.fontSize = IsLandscapeMode() ? 38.0f : 42.0f; // Slightly smaller in landscape
            ui.textColor = GNColor(255, 255, 255, 255);
            ui.centerTextHorizontally = false; ui.centerTextVertically = true; ui.visible = false; ui.textLayer = 84;
            m_ecsCoordinator->AddComponent<Transform>(m_musicLabelEntity, t);
            m_ecsCoordinator->AddComponent<UIElement>(m_musicLabelEntity, ui);
        }

        // MUSIC KNOB
        if (m_musicKnobEntity == 0) {
            m_musicKnobEntity = m_ecsCoordinator->CreateEntity();
            // Use MainMenuState knob sizing approach - consistent UI scaling
            float knobSize = 16.0f * uiScale;  // Same as MainMenuState
            float scale = uiScale;  // Use UI scale instead of hardcoded 6.0f
            float scaledKnobSize = knobSize;

            // Clamp volume value to 0.0-1.0 range like Options menu
            float musicVolume = std::max(0.0f, std::min(1.0f, m_musicSliderValue));

            // Position knob to use full track range - knob edges align with track boundaries
            // At 0%: knob left edge aligns with track start, at 100%: knob right edge aligns with track end
            float knobCenterX = m_sliderX + (scaledKnobSize * 0.5f) + musicVolume * (m_sliderW - scaledKnobSize);
            float knobX = knobCenterX - (scaledKnobSize * 0.5f);
            float knobY = musicTrackY + m_sliderH * 0.5f - (scaledKnobSize * 0.5f);

            Transform t(GNVector2(knobX, knobY), 0.0f, GNVector2(scale, scale));
            Sprite s("poophat", 16, 16); s.layer = 85; s.visible = false;
            UIElement ui("", "poophat", "poophat");
            ui.visible = false; ui.textLayer = 85; ui.isEnabled = true;
            m_ecsCoordinator->AddComponent<Transform>(m_musicKnobEntity, t);
            m_ecsCoordinator->AddComponent<Sprite>(m_musicKnobEntity, s);
            m_ecsCoordinator->AddComponent<UIElement>(m_musicKnobEntity, ui);
        }

        // SFX TRACK - Use UIShape for proper rendering (EXACT same as original)
        if (m_sfxTrackEntity == 0) {
            m_sfxTrackEntity = m_ecsCoordinator->CreateEntity();
            Transform t(GNVector2(m_sliderX, sfxTrackY), 0.0f, GNVector2(1.0f, 1.0f));

            UIShape shape;
            shape.visible = false;
            shape.width = m_sliderW;
            shape.height = m_sliderH;
            shape.color = GNColor(128, 128, 128, 255); // Gray color like main menu options
            shape.layer = 83; // Below knob (85) but above background (80)

            UIElement ui;
            ui.visible = false;
            ui.textLayer = 83; // Match UIShape layer for consistent layering
            ui.isEnabled = true;

            GN_LOG_INFO("PauseSystem: Creating SFX track UIShape: " + std::to_string(m_sliderW) + "x" + std::to_string(m_sliderH) +
                       " at (" + std::to_string(m_sliderX) + ", " + std::to_string(sfxTrackY) + ") layer " + std::to_string(shape.layer) +
                       " entity=" + std::to_string(m_sfxTrackEntity));

            m_ecsCoordinator->AddComponent<Transform>(m_sfxTrackEntity, t);
            m_ecsCoordinator->AddComponent<UIShape>(m_sfxTrackEntity, shape);
            m_ecsCoordinator->AddComponent<UIElement>(m_sfxTrackEntity, ui);
        }

        // SFX LABEL
        if (m_sfxLabelEntity == 0) {
            m_sfxLabelEntity = m_ecsCoordinator->CreateEntity();
            float labelSpacing = IsLandscapeMode() ? 120.0f : 160.0f; // Condensed spacing in landscape
            float labelY = sfxTrackY - labelSpacing;
            // In landscape, move labels further left to avoid knob overlap; in portrait, also move them left to avoid ribbon button overlap
            float labelX = IsLandscapeMode() ? (m_sliderX - 250.0f) : (m_sliderX - 50.0f); // Moved from -200 to -250 in landscape
            Transform t(GNVector2(labelX, labelY), 0.0f, GNVector2(1.0f, 1.0f));
            UIElement ui("SFX", "", "");
            ui.fontSize = IsLandscapeMode() ? 38.0f : 42.0f; // Slightly smaller in landscape
            ui.textColor = GNColor(255, 255, 255, 255);
            ui.centerTextHorizontally = false; ui.centerTextVertically = true; ui.visible = false; ui.textLayer = 84;
            m_ecsCoordinator->AddComponent<Transform>(m_sfxLabelEntity, t);
            m_ecsCoordinator->AddComponent<UIElement>(m_sfxLabelEntity, ui);
        }

        // SFX KNOB
        if (m_sfxKnobEntity == 0) {
            m_sfxKnobEntity = m_ecsCoordinator->CreateEntity();
            // Use MainMenuState knob sizing approach - consistent UI scaling
            float knobSize = 16.0f * uiScale;  // Same as MainMenuState
            float scale = uiScale;  // Use UI scale instead of hardcoded 6.0f
            float scaledKnobSize = knobSize;

            // Clamp volume value to 0.0-1.0 range like Options menu
            float sfxVolume = std::max(0.0f, std::min(1.0f, m_sfxSliderValue));

            // Position knob to use full track range - knob edges align with track boundaries
            // At 0%: knob left edge aligns with track start, at 100%: knob right edge aligns with track end
            float knobCenterX = m_sliderX + (scaledKnobSize * 0.5f) + sfxVolume * (m_sliderW - scaledKnobSize);
            float knobX = knobCenterX - (scaledKnobSize * 0.5f);
            float knobY = sfxTrackY + m_sliderH * 0.5f - (scaledKnobSize * 0.5f);

            Transform t(GNVector2(knobX, knobY), 0.0f, GNVector2(scale, scale));
            Sprite s("poophat", 16, 16); s.layer = 85; s.visible = false;
            UIElement ui("", "poophat", "poophat");
            ui.visible = false; ui.textLayer = 85; ui.isEnabled = true;
            m_ecsCoordinator->AddComponent<Transform>(m_sfxKnobEntity, t);
            m_ecsCoordinator->AddComponent<Sprite>(m_sfxKnobEntity, s);
            m_ecsCoordinator->AddComponent<UIElement>(m_sfxKnobEntity, ui);
        }

        // VIBRATION TOGGLE - Position at same Y level as MASTER label in landscape, below SFX in portrait
        // iPad doesn't have Taptic Engine, so hide vibration toggle on iPad
        float aspectRatio = m_screenWidth / m_screenHeight;
        bool isTablet = IsLandscapeMode() ? (aspectRatio < 1.6f) : (aspectRatio > 0.6f);
        
        // Only create vibration toggle on iPhone (not iPad)
        if (!isTablet) {
        // Calculate vibrationY to align with master label in landscape, below SFX slider in portrait
        float labelSpacingForVibration;
        float vibrationLabelY;
        float vibrationToggleY;
        
        if (IsLandscapeMode()) {
            labelSpacingForVibration = 120.0f; // Same as master label spacing
            vibrationLabelY = masterTrackY - labelSpacingForVibration; // Same Y as MASTER label
            vibrationToggleY = vibrationLabelY + 110.0f; // Button below label
        } else {
            // Portrait mode: position below SFX track
            labelSpacingForVibration = 160.0f;
            vibrationLabelY = sfxTrackY + labelSpacingForVibration; // Below SFX track
            vibrationToggleY = vibrationLabelY + 110.0f; // Button below label
        }
        
        // Vibration toggle button (X sprite) - configured as a proper toggle button
        if (m_vibrationToggleEntity == 0) {
            m_vibrationToggleEntity = m_ecsCoordinator->CreateEntity();
            
            // Get current vibration state from game
            bool vibrationsEnabled = true;
            if (GameCore::GetGame()) {
                vibrationsEnabled = GameCore::GetGame()->GetVibrationsEnabled();
            }
            
            // Center button horizontally below the label
            float toggleScale = 7.0f;
            float buttonWidth = 16.0f * toggleScale; // Actual button width
            float labelX;
            if (IsLandscapeMode()) {
                labelX = m_sliderX + m_sliderW + 350.0f; // Match landscape label X
            } else {
                labelX = centerX; // Match portrait label X (centered)
            }
            float toggleX = labelX - (buttonWidth * 0.5f); // Center button under label
            Transform t(GNVector2(toggleX, vibrationToggleY), 0.0f, GNVector2(toggleScale, toggleScale));
            
            Sprite s(vibrationsEnabled ? "xbuttonselected" : "xbuttonunselected", 64, 64);
            s.visible = false;
            s.layer = 85;
            
            // Configure as a proper toggle button with toggle-specific fields
            UIElement ui("", 
                vibrationsEnabled ? "xbuttonselected" : "xbuttonunselected",
                vibrationsEnabled ? "xbuttonselected" : "xbuttonunselected");
            ui.visible = false;
            ui.textLayer = 85;
            ui.isEnabled = true;
            ui.isToggle = true;  // Mark as toggle button
            ui.toggleState = vibrationsEnabled;  // Set initial state
            ui.toggleOnTexture = "xbuttonselected";   // ON texture
            ui.toggleOffTexture = "xbuttonunselected"; // OFF texture
            
            m_ecsCoordinator->AddComponent<Transform>(m_vibrationToggleEntity, t);
            m_ecsCoordinator->AddComponent<Sprite>(m_vibrationToggleEntity, s);
            m_ecsCoordinator->AddComponent<UIElement>(m_vibrationToggleEntity, ui);
        }
        
        // Vibration label - position at same Y as MASTER label (top row)
        if (m_vibrationLabelEntity == 0) {
            m_vibrationLabelEntity = m_ecsCoordinator->CreateEntity();
            // In landscape: position to the right of sliders
            // In portrait: center horizontally like "Systems" label
            float labelX;
            if (IsLandscapeMode()) {
                labelX = m_sliderX + m_sliderW + 350.0f;
            } else {
                labelX = centerX; // Center horizontally in portrait
            }
            
            Transform t(GNVector2(labelX, vibrationLabelY), 0.0f, GNVector2(1.0f, 1.0f));
            UIElement ui("VIBRATIONS", "", "");
            ui.fontSize = IsLandscapeMode() ? 38.0f : 42.0f; // Use landscape-appropriate font sizes
            ui.textColor = GNColor(255, 255, 255, 255);
            ui.centerTextHorizontally = true; // Center the text
            ui.centerTextVertically = true;
            ui.visible = false;
            ui.textLayer = 84;
            
            m_ecsCoordinator->AddComponent<Transform>(m_vibrationLabelEntity, t);
            m_ecsCoordinator->AddComponent<UIElement>(m_vibrationLabelEntity, ui);
        }
        } // End if (!isTablet) - vibration toggle only on iPhone

        GN_LOG_INFO("PauseSystem: Audio sliders and vibration toggle created");
    }

    void PauseSystem::ShowPauseMenu() {
        GN_LOG_INFO("PauseSystem: Showing pause menu");

        // Initialize slider values from current audio settings FIRST (before making anything visible)
        if (GameCore::GetGame()) {
            m_masterSliderValue = GameCore::GetGame()->GetMasterVolume();
            m_musicSliderValue = GameCore::GetGame()->GetMusicVolume();
            m_sfxSliderValue = GameCore::GetGame()->GetSFXVolume();
            GN_LOG_INFO("PauseSystem: Initialized slider values - Master: " + std::to_string(m_masterSliderValue) +
                       ", Music: " + std::to_string(m_musicSliderValue) + ", SFX: " + std::to_string(m_sfxSliderValue));
            
            // Update knob positions to reflect loaded values
            float knobSize = 16.0f * 8.0f;
            float masterTrackY = m_sliderY + 9.0f;
            float musicTrackY = m_sliderY + m_sliderSpacing + 9.0f;
            float sfxTrackY = m_sliderY + m_sliderSpacing * 2 + 9.0f;
            
            if (m_masterKnobEntity != 0 && m_ecsCoordinator) {
                auto* transform = m_ecsCoordinator->GetComponent<Transform>(m_masterKnobEntity);
                if (transform) {
                    float knobCenterX = m_sliderX + m_masterSliderValue * m_sliderW;
                    transform->position.x = knobCenterX - (knobSize * 0.5f);
                    transform->position.y = masterTrackY + m_sliderH * 0.5f - (knobSize * 0.5f);
                }
            }
            if (m_musicKnobEntity != 0 && m_ecsCoordinator) {
                auto* transform = m_ecsCoordinator->GetComponent<Transform>(m_musicKnobEntity);
                if (transform) {
                    float knobCenterX = m_sliderX + m_musicSliderValue * m_sliderW;
                    transform->position.x = knobCenterX - (knobSize * 0.5f);
                    transform->position.y = musicTrackY + m_sliderH * 0.5f - (knobSize * 0.5f);
                }
            }
            if (m_sfxKnobEntity != 0 && m_ecsCoordinator) {
                auto* transform = m_ecsCoordinator->GetComponent<Transform>(m_sfxKnobEntity);
                if (transform) {
                    float knobCenterX = m_sliderX + m_sfxSliderValue * m_sliderW;
                    transform->position.x = knobCenterX - (knobSize * 0.5f);
                    transform->position.y = sfxTrackY + m_sliderH * 0.5f - (knobSize * 0.5f);
                }
            }
            GN_LOG_INFO("PauseSystem: Updated knob positions to reflect loaded audio values");
        }

        // ATOMIC RENDERING FIX: Collect all entities to show, then make them all visible at once
        // This prevents the "domino effect" where elements appear sequentially across frames
        std::vector<Entity> entitiesToShow;
        std::vector<std::pair<UIElement*, bool>> uiElementsToShow;
        std::vector<std::pair<Sprite*, bool>> spritesToShow;
        std::vector<std::pair<UIShape*, bool>> shapesToShow;

        // Add background entities
        if (m_pauseMenuBackgroundEntity != 0 && m_ecsCoordinator) {
            entitiesToShow.push_back(m_pauseMenuBackgroundEntity);
        }

        // Add ribbon entity
        if (m_pauseMenuRibbonEntity != 0 && m_ecsCoordinator) {
            entitiesToShow.push_back(m_pauseMenuRibbonEntity);
        }

        // Add ribbon buttons
        for (Entity& buttonEntity : m_ribbonButtons) {
            if (buttonEntity != 0) {
                entitiesToShow.push_back(buttonEntity);
            }
        }

        // Collect tab content entities based on current tab
        HideAllTabContent();  // Ensure clean slate
        switch (m_currentTab) {
            case PauseMenuTab::SYSTEM:
                if (m_systemTitleEntity != 0) entitiesToShow.push_back(m_systemTitleEntity);
                if (m_mainMenuButtonEntity != 0) entitiesToShow.push_back(m_mainMenuButtonEntity);
                if (m_masterKnobEntity != 0) entitiesToShow.push_back(m_masterKnobEntity);
                if (m_masterTrackEntity != 0) entitiesToShow.push_back(m_masterTrackEntity);
                if (m_masterLabelEntity != 0) entitiesToShow.push_back(m_masterLabelEntity);
                if (m_musicKnobEntity != 0) entitiesToShow.push_back(m_musicKnobEntity);
                if (m_musicTrackEntity != 0) entitiesToShow.push_back(m_musicTrackEntity);
                if (m_musicLabelEntity != 0) entitiesToShow.push_back(m_musicLabelEntity);
                if (m_sfxKnobEntity != 0) entitiesToShow.push_back(m_sfxKnobEntity);
                if (m_sfxTrackEntity != 0) entitiesToShow.push_back(m_sfxTrackEntity);
                if (m_sfxLabelEntity != 0) entitiesToShow.push_back(m_sfxLabelEntity);
                if (m_vibrationLabelEntity != 0) entitiesToShow.push_back(m_vibrationLabelEntity);
                if (m_vibrationToggleEntity != 0) entitiesToShow.push_back(m_vibrationToggleEntity);
                break;
            case PauseMenuTab::SKILLS:
                if (m_skillsTitleEntity != 0) entitiesToShow.push_back(m_skillsTitleEntity);
                if (m_skillsBackgroundEntity != 0) entitiesToShow.push_back(m_skillsBackgroundEntity);
                if (m_skillsContentEntity != 0) entitiesToShow.push_back(m_skillsContentEntity);
                if (m_skillsNameEntity != 0) entitiesToShow.push_back(m_skillsNameEntity);
                if (m_skillsDescriptionEntity != 0) entitiesToShow.push_back(m_skillsDescriptionEntity);
                if (m_skillsCostEntity != 0) entitiesToShow.push_back(m_skillsCostEntity);
                if (m_skillsUnlockButtonEntity != 0) entitiesToShow.push_back(m_skillsUnlockButtonEntity);
                if (m_skillsLeftArrowEntity != 0) entitiesToShow.push_back(m_skillsLeftArrowEntity);
                if (m_skillsRightArrowEntity != 0) entitiesToShow.push_back(m_skillsRightArrowEntity);
                break;
            case PauseMenuTab::HATS:
                if (m_hatsTitleEntity != 0) entitiesToShow.push_back(m_hatsTitleEntity);
                if (m_hatsBackgroundEntity != 0) entitiesToShow.push_back(m_hatsBackgroundEntity);
                if (m_hatsContentEntity != 0) entitiesToShow.push_back(m_hatsContentEntity);
                for (auto frameEntity : m_hatFrameEntities) {
                    if (frameEntity != 0) entitiesToShow.push_back(frameEntity);
                }
                for (auto iconEntity : m_hatIconEntities) {
                    if (iconEntity != 0) entitiesToShow.push_back(iconEntity);
                }
                for (auto lockedFrameEntity : m_lockedFrameEntities) {
                    if (lockedFrameEntity != 0) entitiesToShow.push_back(lockedFrameEntity);
                }
                if (m_hatsActionButtonEntity != 0) entitiesToShow.push_back(m_hatsActionButtonEntity);
                if (m_hatsCostDisplayEntity != 0) entitiesToShow.push_back(m_hatsCostDisplayEntity);
                break;
            case PauseMenuTab::STATS:
                if (m_statsTitleEntity != 0) entitiesToShow.push_back(m_statsTitleEntity);
                if (m_statsBackgroundEntity != 0) entitiesToShow.push_back(m_statsBackgroundEntity);
                if (m_currentSessionTextEntity != 0) entitiesToShow.push_back(m_currentSessionTextEntity);
                if (m_sessionCoinsTextEntity != 0) entitiesToShow.push_back(m_sessionCoinsTextEntity);
                if (m_totalCoinsTextEntity != 0) entitiesToShow.push_back(m_totalCoinsTextEntity);
                if (m_totalFlopsTextEntity != 0) entitiesToShow.push_back(m_totalFlopsTextEntity);
                if (m_grossTotalCoinsTextEntity != 0) entitiesToShow.push_back(m_grossTotalCoinsTextEntity);
                if (m_enemiesKilledTextEntity != 0) entitiesToShow.push_back(m_enemiesKilledTextEntity);
                if (m_totalPipesTextEntity != 0) entitiesToShow.push_back(m_totalPipesTextEntity);
                for (auto levelEntity : m_levelHighScoreTextEntities) {
                    if (levelEntity != 0) entitiesToShow.push_back(levelEntity);
                }
                break;
        }

        // Now atomically set ALL entities to visible at once
        for (Entity entity : entitiesToShow) {
            if (entity != 0 && m_ecsCoordinator) {
                UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElem) {
                    uiElem->visible = true;
                }
                
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                if (sprite) {
                    sprite->visible = true;
                }
                
                UIShape* shape = m_ecsCoordinator->GetComponent<UIShape>(entity);
                if (shape) {
                    shape->visible = true;
                }
            }
        }

        GN_LOG_INFO("PauseSystem: Pause menu shown atomically (" + std::to_string(entitiesToShow.size()) + " entities)");
    }

    void PauseSystem::HidePauseMenu() {
        GN_LOG_INFO("PauseSystem: Hiding pause menu");

        // Hide pause menu background
        if (m_pauseMenuBackgroundEntity != 0 && m_ecsCoordinator) {
            UIElement* bgUI = m_ecsCoordinator->GetComponent<UIElement>(m_pauseMenuBackgroundEntity);
            if (bgUI) {
                bgUI->visible = false;
            }

            // Also hide the Sprite component
            Sprite* bgSprite = m_ecsCoordinator->GetComponent<Sprite>(m_pauseMenuBackgroundEntity);
            if (bgSprite) {
                bgSprite->visible = false;
            }
        }

        // Hide pause menu ribbon
        if (m_pauseMenuRibbonEntity != 0 && m_ecsCoordinator) {
            UIElement* ribbonUI = m_ecsCoordinator->GetComponent<UIElement>(m_pauseMenuRibbonEntity);
            if (ribbonUI) {
                ribbonUI->visible = false;
            }
        }

        // Hide ribbon buttons
        for (Entity& buttonEntity : m_ribbonButtons) {
            if (buttonEntity != 0 && m_ecsCoordinator) {
                UIElement* buttonUI = m_ecsCoordinator->GetComponent<UIElement>(buttonEntity);
                if (buttonUI) {
                    buttonUI->visible = false;
                }

                // Also hide the Sprite component
                Sprite* buttonSprite = m_ecsCoordinator->GetComponent<Sprite>(buttonEntity);
                if (buttonSprite) {
                    buttonSprite->visible = false;
                }
            }
        }

        // Hide all tab content
        HideAllTabContent();

        GN_LOG_INFO("PauseSystem: Pause menu hidden");
    }

    void PauseSystem::ShowTabContent(PauseMenuTab tab) {
        GN_LOG_INFO("PauseSystem: Showing tab content for tab " + std::to_string(static_cast<int>(tab)));
        HideAllTabContent();

        switch (tab) {
            case PauseMenuTab::SYSTEM:
                ShowSystemTab();
                break;
            case PauseMenuTab::SKILLS:
                ShowSkillsTab();
                break;
            case PauseMenuTab::HATS:
                ShowHatsTab();
                break;
            case PauseMenuTab::STATS:
                ShowStatsTab();
                break;
        }
    }

    void PauseSystem::HideAllTabContent() {
        GN_LOG_INFO("PauseSystem: Hiding all tab content");

        // Helper lambda to hide UI element and sprite components
        auto hideEntity = [this](Entity entity) {
            if (entity != 0 && m_ecsCoordinator) {
                // Hide UIElement component
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElement) {
                    uiElement->visible = false;
                }

                // Hide Sprite component
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                if (sprite) {
                    sprite->visible = false;
                }

                // Hide UIShape component (for backgrounds and tracks)
                UIShape* shape = m_ecsCoordinator->GetComponent<UIShape>(entity);
                if (shape) {
                    shape->visible = false;
                    GN_LOG_INFO("PauseSystem: Hiding UIShape component");
                }
            }
        };

        // Hide system tab entities
        hideEntity(m_systemTitleEntity);
        hideEntity(m_mainMenuButtonEntity);
        hideEntity(m_masterKnobEntity);
        hideEntity(m_masterTrackEntity);
        hideEntity(m_masterLabelEntity);
        hideEntity(m_musicKnobEntity);
        hideEntity(m_musicTrackEntity);
        hideEntity(m_musicLabelEntity);
        hideEntity(m_sfxKnobEntity);
        hideEntity(m_sfxTrackEntity);
        hideEntity(m_sfxLabelEntity);
        hideEntity(m_vibrationLabelEntity);
        hideEntity(m_vibrationToggleEntity);

        // Hide skills tab entities
        hideEntity(m_skillsBackgroundEntity);
        hideEntity(m_skillsTitleEntity);
        hideEntity(m_skillsContentEntity);
        hideEntity(m_skillsNameEntity);
        hideEntity(m_skillsDescriptionEntity);
        hideEntity(m_skillsCostEntity);
        hideEntity(m_skillsUnlockButtonEntity);
        hideEntity(m_skillsLeftArrowEntity);
        hideEntity(m_skillsRightArrowEntity);

        // Hide hats tab entities
        hideEntity(m_hatsBackgroundEntity);
        hideEntity(m_hatsTitleEntity);
        hideEntity(m_hatsContentEntity);

        // Hide hats grid UI elements
        for (auto frameEntity : m_hatFrameEntities) {
            if (frameEntity != 0 && m_ecsCoordinator) {
                Sprite* frameSprite = m_ecsCoordinator->GetComponent<Sprite>(frameEntity);
                if (frameSprite) frameSprite->visible = false;
                UIElement* frameUI = m_ecsCoordinator->GetComponent<UIElement>(frameEntity);
                if (frameUI) frameUI->visible = false;
            }
        }

        for (auto iconEntity : m_hatIconEntities) {
            if (iconEntity != 0 && m_ecsCoordinator) {
                Sprite* iconSprite = m_ecsCoordinator->GetComponent<Sprite>(iconEntity);
                if (iconSprite) iconSprite->visible = false;
                UIElement* iconUI = m_ecsCoordinator->GetComponent<UIElement>(iconEntity);
                if (iconUI) iconUI->visible = false;
            }
        }

        // Hide all locked frame entities
        for (auto lockedFrameEntity : m_lockedFrameEntities) {
            if (lockedFrameEntity != 0 && m_ecsCoordinator) {
                Sprite* lockedFrameSprite = m_ecsCoordinator->GetComponent<Sprite>(lockedFrameEntity);
                if (lockedFrameSprite) lockedFrameSprite->visible = false;
                UIElement* lockedFrameUI = m_ecsCoordinator->GetComponent<UIElement>(lockedFrameEntity);
                if (lockedFrameUI) lockedFrameUI->visible = false;
            }
        }

        // Hide action button and cost display
        if (m_hatsActionButtonEntity != 0 && m_ecsCoordinator) {
            Sprite* buttonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_hatsActionButtonEntity);
            if (buttonSprite) buttonSprite->visible = false;
            UIElement* buttonUI = m_ecsCoordinator->GetComponent<UIElement>(m_hatsActionButtonEntity);
            if (buttonUI) buttonUI->visible = false;
        }

        if (m_hatsCostDisplayEntity != 0 && m_ecsCoordinator) {
            Sprite* costSprite = m_ecsCoordinator->GetComponent<Sprite>(m_hatsCostDisplayEntity);
            if (costSprite) costSprite->visible = false;
            UIElement* costUI = m_ecsCoordinator->GetComponent<UIElement>(m_hatsCostDisplayEntity);
            if (costUI) costUI->visible = false;
        }

        // Hide stats tab entities
        hideEntity(m_statsBackgroundEntity);
        hideEntity(m_statsTitleEntity);
        hideEntity(m_currentSessionTextEntity);
        hideEntity(m_sessionCoinsTextEntity);
        hideEntity(m_totalCoinsTextEntity);
        hideEntity(m_grossTotalCoinsTextEntity);
        hideEntity(m_totalFlopsTextEntity);
        hideEntity(m_enemiesKilledTextEntity);
        hideEntity(m_totalPipesTextEntity);
        hideEntity(m_bossTimerTextEntity); // Hide Rat King boss timer
        
        // Hide level high score entities
        for (auto levelScoreEntity : m_levelHighScoreTextEntities) {
            hideEntity(levelScoreEntity);
        }

        GN_LOG_INFO("PauseSystem: All tab content hidden");
    }

    void PauseSystem::ShowSystemTab() {
        GN_LOG_INFO("PauseSystem: Showing system tab");

        // Show system title
        if (m_systemTitleEntity != 0 && m_ecsCoordinator) {
            Sprite* titleSprite = m_ecsCoordinator->GetComponent<Sprite>(m_systemTitleEntity);
            if (titleSprite) {
                titleSprite->visible = true;
            }
            UIElement* titleUI = m_ecsCoordinator->GetComponent<UIElement>(m_systemTitleEntity);
            if (titleUI) {
                titleUI->visible = true;
            }
        }

        // Show system tab entities
        if (m_mainMenuButtonEntity != 0 && m_ecsCoordinator) {
            Sprite* buttonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_mainMenuButtonEntity);
            if (buttonSprite) {
                buttonSprite->visible = true;
            }
        }

        // Show main menu button UI element
        if (m_mainMenuButtonEntity != 0 && m_ecsCoordinator) {
            UIElement* buttonUI = m_ecsCoordinator->GetComponent<UIElement>(m_mainMenuButtonEntity);
            if (buttonUI) {
                buttonUI->visible = true;
                GN_LOG_INFO("PauseSystem: Made main menu button UI visible");
            }
        }

        // Show MASTER volume controls
        if (m_masterKnobEntity != 0 && m_ecsCoordinator) {
            Sprite* masterKnob = m_ecsCoordinator->GetComponent<Sprite>(m_masterKnobEntity);
            if (masterKnob) {
                masterKnob->visible = true;
            }
            UIElement* masterKnobUI = m_ecsCoordinator->GetComponent<UIElement>(m_masterKnobEntity);
            if (masterKnobUI) {
                masterKnobUI->visible = true;
            }
        }

        if (m_masterTrackEntity != 0 && m_ecsCoordinator) {
            UIShape* masterTrack = m_ecsCoordinator->GetComponent<UIShape>(m_masterTrackEntity);
            if (masterTrack) {
                masterTrack->visible = true;
                GN_LOG_INFO("PauseSystem: Made MASTER track visible");
            }
            UIElement* masterTrackUI = m_ecsCoordinator->GetComponent<UIElement>(m_masterTrackEntity);
            if (masterTrackUI) {
                masterTrackUI->visible = true;
            }
        }

        if (m_masterLabelEntity != 0 && m_ecsCoordinator) {
            UIElement* masterLabelUI = m_ecsCoordinator->GetComponent<UIElement>(m_masterLabelEntity);
            if (masterLabelUI) {
                masterLabelUI->visible = true;
            }
        }

        // Show audio slider entities if they exist
        if (m_musicKnobEntity != 0 && m_ecsCoordinator) {
            Sprite* musicKnob = m_ecsCoordinator->GetComponent<Sprite>(m_musicKnobEntity);
            if (musicKnob) {
                musicKnob->visible = true;
            }
            UIElement* musicKnobUI = m_ecsCoordinator->GetComponent<UIElement>(m_musicKnobEntity);
            if (musicKnobUI) {
                musicKnobUI->visible = true;
            }
        }

        if (m_sfxKnobEntity != 0 && m_ecsCoordinator) {
            Sprite* sfxKnob = m_ecsCoordinator->GetComponent<Sprite>(m_sfxKnobEntity);
            if (sfxKnob) {
                sfxKnob->visible = true;
            }
            UIElement* sfxKnobUI = m_ecsCoordinator->GetComponent<UIElement>(m_sfxKnobEntity);
            if (sfxKnobUI) {
                sfxKnobUI->visible = true;
            }
        }

        // Show slider tracks and labels
        if (m_musicTrackEntity != 0 && m_ecsCoordinator) {
            UIShape* musicTrack = m_ecsCoordinator->GetComponent<UIShape>(m_musicTrackEntity);
            if (musicTrack) {
                musicTrack->visible = true;
                GN_LOG_INFO("PauseSystem: Made MUSIC track visible");
            }
            UIElement* musicTrackUI = m_ecsCoordinator->GetComponent<UIElement>(m_musicTrackEntity);
            if (musicTrackUI) {
                musicTrackUI->visible = true;
            }
        }

        if (m_sfxTrackEntity != 0 && m_ecsCoordinator) {
            UIShape* sfxTrack = m_ecsCoordinator->GetComponent<UIShape>(m_sfxTrackEntity);
            if (sfxTrack) {
                sfxTrack->visible = true;
                GN_LOG_INFO("PauseSystem: Made SFX track visible");
            }
            UIElement* sfxTrackUI = m_ecsCoordinator->GetComponent<UIElement>(m_sfxTrackEntity);
            if (sfxTrackUI) {
                sfxTrackUI->visible = true;
            }
        }

        if (m_musicLabelEntity != 0 && m_ecsCoordinator) {
            UIElement* musicLabelUI = m_ecsCoordinator->GetComponent<UIElement>(m_musicLabelEntity);
            if (musicLabelUI) {
                musicLabelUI->visible = true;
            }
        }

        if (m_sfxLabelEntity != 0 && m_ecsCoordinator) {
            UIElement* sfxLabelUI = m_ecsCoordinator->GetComponent<UIElement>(m_sfxLabelEntity);
            if (sfxLabelUI) {
                sfxLabelUI->visible = true;
            }
        }

        // Show vibration label and toggle
        if (m_vibrationLabelEntity != 0 && m_ecsCoordinator) {
            UIElement* vibrationLabelUI = m_ecsCoordinator->GetComponent<UIElement>(m_vibrationLabelEntity);
            if (vibrationLabelUI) {
                vibrationLabelUI->visible = true;
                GN_LOG_INFO("PauseSystem: Made vibration label visible");
            }
        }

        if (m_vibrationToggleEntity != 0 && m_ecsCoordinator) {
            // Sync vibration state from game before showing
            bool vibrationsEnabled = true;
            if (GameCore::GetGame()) {
                vibrationsEnabled = GameCore::GetGame()->GetVibrationsEnabled();
            }
            
            Sprite* vibrationToggleSprite = m_ecsCoordinator->GetComponent<Sprite>(m_vibrationToggleEntity);
            if (vibrationToggleSprite) {
                vibrationToggleSprite->textureId = vibrationsEnabled ? "xbuttonselected" : "xbuttonunselected";
                vibrationToggleSprite->visible = true;
                GN_LOG_INFO("PauseSystem: Made vibration toggle sprite visible - state: " + std::string(vibrationsEnabled ? "ON" : "OFF"));
            }
            UIElement* vibrationToggleUI = m_ecsCoordinator->GetComponent<UIElement>(m_vibrationToggleEntity);
            if (vibrationToggleUI) {
                vibrationToggleUI->normalTextureId = vibrationsEnabled ? "xbuttonselected" : "xbuttonunselected";
                vibrationToggleUI->hoverTextureId = vibrationsEnabled ? "xbuttonselected" : "xbuttonunselected";
                vibrationToggleUI->visible = true;
                GN_LOG_INFO("PauseSystem: Made vibration toggle UI visible");
            }
        }

        GN_LOG_INFO("PauseSystem: System tab shown");
    }

    void PauseSystem::ShowSkillsTab() {
        GN_LOG_INFO("PauseSystem: Showing skills tab");

        // Show skills title
        if (m_skillsTitleEntity != 0 && m_ecsCoordinator) {
            Sprite* titleSprite = m_ecsCoordinator->GetComponent<Sprite>(m_skillsTitleEntity);
            if (titleSprite) {
                titleSprite->visible = true;
            }
            UIElement* titleUI = m_ecsCoordinator->GetComponent<UIElement>(m_skillsTitleEntity);
            if (titleUI) {
                titleUI->visible = true;
            }
        }

        // Show skills background
        if (m_skillsBackgroundEntity != 0 && m_ecsCoordinator) {
            UIShape* bgShape = m_ecsCoordinator->GetComponent<UIShape>(m_skillsBackgroundEntity);
            if (bgShape) {
                bgShape->visible = true;
                GN_LOG_INFO("PauseSystem: Skills background shape shown - layer " + std::to_string(bgShape->layer));
            }
            UIElement* bgUI = m_ecsCoordinator->GetComponent<UIElement>(m_skillsBackgroundEntity);
            if (bgUI) {
                bgUI->visible = true;
                GN_LOG_INFO("PauseSystem: Skills background UI shown - layer " + std::to_string(bgUI->textLayer));
            }
        }

        // Show skills content
        if (m_skillsContentEntity != 0 && m_ecsCoordinator) {
            Sprite* contentSprite = m_ecsCoordinator->GetComponent<Sprite>(m_skillsContentEntity);
            if (contentSprite) {
                contentSprite->visible = true;
            }
            UIElement* contentUI = m_ecsCoordinator->GetComponent<UIElement>(m_skillsContentEntity);
            if (contentUI) {
                contentUI->visible = true;
            }
        }

        // Show skill name text
        if (m_skillsNameEntity != 0 && m_ecsCoordinator) {
            Sprite* nameSprite = m_ecsCoordinator->GetComponent<Sprite>(m_skillsNameEntity);
            if (nameSprite) {
                nameSprite->visible = true;
            }
            UIElement* nameUI = m_ecsCoordinator->GetComponent<UIElement>(m_skillsNameEntity);
            if (nameUI) {
                nameUI->visible = true;
            }
        }

        // Show skill description text
        if (m_skillsDescriptionEntity != 0 && m_ecsCoordinator) {
            Sprite* descSprite = m_ecsCoordinator->GetComponent<Sprite>(m_skillsDescriptionEntity);
            if (descSprite) {
                descSprite->visible = true;
            }
            UIElement* descUI = m_ecsCoordinator->GetComponent<UIElement>(m_skillsDescriptionEntity);
            if (descUI) {
                descUI->visible = true;
            }
        }

        // Show skill cost text
        if (m_skillsCostEntity != 0 && m_ecsCoordinator) {
            Sprite* costSprite = m_ecsCoordinator->GetComponent<Sprite>(m_skillsCostEntity);
            if (costSprite) {
                costSprite->visible = true;
            }
            UIElement* costUI = m_ecsCoordinator->GetComponent<UIElement>(m_skillsCostEntity);
            if (costUI) {
                costUI->visible = true;
            }
        }

        // Show unlock button
        if (m_skillsUnlockButtonEntity != 0 && m_ecsCoordinator) {
            Sprite* buttonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_skillsUnlockButtonEntity);
            if (buttonSprite) {
                buttonSprite->visible = true;
            }
            UIElement* buttonUI = m_ecsCoordinator->GetComponent<UIElement>(m_skillsUnlockButtonEntity);
            if (buttonUI) {
                buttonUI->visible = true;
            }
        }

        // Show left arrow
        if (m_skillsLeftArrowEntity != 0 && m_ecsCoordinator) {
            Sprite* arrowSprite = m_ecsCoordinator->GetComponent<Sprite>(m_skillsLeftArrowEntity);
            if (arrowSprite) {
                arrowSprite->visible = true;
            }
            UIElement* arrowUI = m_ecsCoordinator->GetComponent<UIElement>(m_skillsLeftArrowEntity);
            if (arrowUI) {
                arrowUI->visible = true;
            }
        }

        // Show right arrow
        if (m_skillsRightArrowEntity != 0 && m_ecsCoordinator) {
            Sprite* arrowSprite = m_ecsCoordinator->GetComponent<Sprite>(m_skillsRightArrowEntity);
            if (arrowSprite) {
                arrowSprite->visible = true;
            }
            UIElement* arrowUI = m_ecsCoordinator->GetComponent<UIElement>(m_skillsRightArrowEntity);
            if (arrowUI) {
                arrowUI->visible = true;
            }
        }

        // Update the skill display with current information
        UpdateSkillDisplay(m_currentSkillIndex, m_availableSkills, m_skillSystem, 0, m_playerEntity);

        GN_LOG_INFO("PauseSystem: Skills tab shown");
    }

    void PauseSystem::ShowHatsTab() {
        GN_LOG_INFO("PauseSystem: Showing hats tab");

        // Show hats background
        if (m_hatsBackgroundEntity != 0 && m_ecsCoordinator) {
            // Note: Hats background now uses Sprite with "Pixel" texture, not UIShape
            Sprite* bgSprite = m_ecsCoordinator->GetComponent<Sprite>(m_hatsBackgroundEntity);
            if (bgSprite) {
                bgSprite->visible = true;
            }
            GameCore::UIShape* bgShape = m_ecsCoordinator->GetComponent<GameCore::UIShape>(m_hatsBackgroundEntity);
            if (bgShape) {
                bgShape->visible = true;
            }
            UIElement* bgUI = m_ecsCoordinator->GetComponent<UIElement>(m_hatsBackgroundEntity);
            if (bgUI) {
                bgUI->visible = true;
            }
        }

        // Show hats title
        if (m_hatsTitleEntity != 0 && m_ecsCoordinator) {
            Sprite* titleSprite = m_ecsCoordinator->GetComponent<Sprite>(m_hatsTitleEntity);
            if (titleSprite) {
                titleSprite->visible = true;
            }
            UIElement* titleUI = m_ecsCoordinator->GetComponent<UIElement>(m_hatsTitleEntity);
            if (titleUI) {
                titleUI->visible = true;
            }
        }

        // Show hats grid UI elements created by PauseSystem
        GN_LOG_INFO("PauseSystem: Showing hats grid UI elements");

        // Show all hat frames and icons
        for (auto frameEntity : m_hatFrameEntities) {
            if (frameEntity != 0 && m_ecsCoordinator) {
                Sprite* frameSprite = m_ecsCoordinator->GetComponent<Sprite>(frameEntity);
                if (frameSprite) frameSprite->visible = true;
                UIElement* frameUI = m_ecsCoordinator->GetComponent<UIElement>(frameEntity);
                if (frameUI) frameUI->visible = true;
            }
        }

        for (auto iconEntity : m_hatIconEntities) {
            if (iconEntity != 0 && m_ecsCoordinator) {
                Sprite* iconSprite = m_ecsCoordinator->GetComponent<Sprite>(iconEntity);
                if (iconSprite) iconSprite->visible = true;
                UIElement* iconUI = m_ecsCoordinator->GetComponent<UIElement>(iconEntity);
                if (iconUI) iconUI->visible = true;
            }
        }

        // Show all locked frame entities
        for (auto lockedFrameEntity : m_lockedFrameEntities) {
            if (lockedFrameEntity != 0 && m_ecsCoordinator) {
                Sprite* lockedFrameSprite = m_ecsCoordinator->GetComponent<Sprite>(lockedFrameEntity);
                if (lockedFrameSprite) lockedFrameSprite->visible = true;
                UIElement* lockedFrameUI = m_ecsCoordinator->GetComponent<UIElement>(lockedFrameEntity);
                if (lockedFrameUI) lockedFrameUI->visible = true;
            }
        }

        // Show action button and cost display
        if (m_hatsActionButtonEntity != 0 && m_ecsCoordinator) {
            Sprite* buttonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_hatsActionButtonEntity);
            if (buttonSprite) buttonSprite->visible = true;
            UIElement* buttonUI = m_ecsCoordinator->GetComponent<UIElement>(m_hatsActionButtonEntity);
            if (buttonUI) buttonUI->visible = true;
        }

        if (m_hatsCostDisplayEntity != 0 && m_ecsCoordinator) {
            Sprite* costSprite = m_ecsCoordinator->GetComponent<Sprite>(m_hatsCostDisplayEntity);
            if (costSprite) costSprite->visible = true;
            UIElement* costUI = m_ecsCoordinator->GetComponent<UIElement>(m_hatsCostDisplayEntity);
            if (costUI) costUI->visible = true;
        }

        // Update hat display with current state
        UpdateHatDisplay();

        // NOTE: Removed UpdateSkillDisplayFromCurrentState() call as it was causing
        // skill entities to show in hats tab. Skill display should only update when
        // actually on the skills tab.

        GN_LOG_INFO("PauseSystem: Hats tab shown");
    }

    void PauseSystem::ShowStatsTab() {
        GN_LOG_INFO("PauseSystem: Showing stats tab");

        // Show title
        if (m_statsTitleEntity != 0 && m_ecsCoordinator) {
            UIElement* titleUI = m_ecsCoordinator->GetComponent<UIElement>(m_statsTitleEntity);
            if (titleUI) titleUI->visible = true;

            Sprite* titleSprite = m_ecsCoordinator->GetComponent<Sprite>(m_statsTitleEntity);
            if (titleSprite) titleSprite->visible = true;
        }

        // Show background
        if (m_statsBackgroundEntity != 0 && m_ecsCoordinator) {
            UIShape* bgShape = m_ecsCoordinator->GetComponent<UIShape>(m_statsBackgroundEntity);
            if (bgShape) bgShape->visible = true;

            UIElement* bgUI = m_ecsCoordinator->GetComponent<UIElement>(m_statsBackgroundEntity);
            if (bgUI) bgUI->visible = true;
        }

        // Update and show stats
        RefreshStatsDisplay(m_sessionPipes, m_sessionCoins, m_totalCoins, m_grossTotalCoins, m_totalFlops, m_enemiesKilled, m_totalPipes);

        GN_LOG_INFO("PauseSystem: Stats tab shown");
    }

    void PauseSystem::HandlePauseMenuInput(float touchX, float touchY) {
        GN_LOG_INFO("PauseSystem: Handling pause menu input at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        // Use cached screen dimensions
        GN_LOG_INFO("PauseSystem: Screen dimensions: " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight) +
                   " - Touch coordinates: (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        // Check ribbon button clicks first (these are global, not per-tab)
        GN_LOG_INFO("PauseSystem: Checking ribbon button clicks at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")...");
        if (HandlePauseMenuRibbonClick(touchX, touchY)) {
            // Reset ribbon button debounce timer
            m_lastRibbonButtonPressTime = 0.0f;
            GN_LOG_INFO("PauseSystem: Ribbon button was clicked - switching tabs");
            return; // Ribbon button was clicked, don't process other input
        }

        // Settings button is handled directly by GameplayState, not by PauseSystem
        // This matches the original implementation where CheckSettingsButtonClick
        // is called from both normal gameplay and pause menu input

        // Check content area clicks based on current tab
        GN_LOG_INFO("PauseSystem: Checking content area clicks for tab " + std::to_string(static_cast<int>(m_currentTab)));
        HandlePauseMenuContentClick(touchX, touchY);

        // Note: Tap outside menu area is now handled by GameplayState to avoid conflicts
    }

    void PauseSystem::HandlePauseMenuContentClick(float touchX, float touchY) {
        GN_LOG_INFO("PauseSystem: Handling content click at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        // Route to appropriate tab handler based on current tab
        switch (m_currentTab) {
            case PauseMenuTab::SYSTEM:
                HandleSystemTabClick(touchX, touchY);
                break;
            case PauseMenuTab::SKILLS:
                HandleSkillsTabClick(touchX, touchY);
                break;
            case PauseMenuTab::HATS:
                HandleHatsTabClick(touchX, touchY);
                break;
            case PauseMenuTab::STATS:
                HandleStatsTabClick(touchX, touchY);
                break;
        }
    }

    bool PauseSystem::HandlePauseMenuRibbonClick(float touchX, float touchY) {
        GN_LOG_INFO("PauseSystem: Handling ribbon click at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        // Check debounce timer to prevent rapid clicking
        if (m_lastRibbonButtonPressTime < m_ribbonButtonDebounceDelay) {
            GN_LOG_INFO("PauseSystem: Ribbon button debounced - timer: " + std::to_string(m_lastRibbonButtonPressTime) +
                       ", delay: " + std::to_string(m_ribbonButtonDebounceDelay));
            return false;
        }

        // Use cached screen dimensions (same as used for button creation)
        GN_LOG_INFO("PauseSystem: Using cached screen dimensions: " + std::to_string((int)m_screenWidth) + "x" + std::to_string((int)m_screenHeight));

        // Calculate button positions - MUST MATCH CreateRibbonButtons() logic exactly
        float buttonScale = 6.0f;
        float buttonWidth, buttonHeight;

        if (IsLandscapeMode()) {
            // Landscape mode: use FloppyButtonBlue dimensions (90x16)
            buttonWidth = 90.0f * buttonScale;
            buttonHeight = 16.0f * buttonScale;
        } else {
            // Portrait mode: use ribbon button dimensions (64x21)
            buttonWidth = 64.0f * buttonScale;
            buttonHeight = buttonWidth * 0.62f; // Match ribbon button aspect ratio
        }

        // Check ribbon buttons using actual sprite positions (like main menu)
        GN_LOG_INFO("PauseSystem: Checking " + std::to_string(m_ribbonButtons.size()) + " ribbon buttons using sprite-based collision");

        if (m_ribbonButtons.empty()) {
            GN_LOG_ERROR("PauseSystem: No ribbon buttons found! Cannot detect clicks.");
            return false;
        }


        for (int i = 0; i < m_ribbonButtons.size(); i++) {
            Entity buttonEntity = m_ribbonButtons[i];
            if (buttonEntity == 0) continue;

            // Ribbon buttons are top-left positioned at their transform positions
            if (IsTouchInButtonBounds(touchX, touchY, buttonEntity, false)) {
                GN_LOG_INFO("PauseSystem: Ribbon button " + std::to_string(i) + " clicked");
                // Switch to the appropriate tab
                PauseMenuTab newTab;
                switch (i) {
                    case 0: newTab = PauseMenuTab::SKILLS; break;
                    case 1: newTab = PauseMenuTab::HATS; break;
                    case 2: newTab = PauseMenuTab::STATS; break;
                    case 3: newTab = PauseMenuTab::SYSTEM; break;
                    default: newTab = PauseMenuTab::SYSTEM; break;
                }
                SwitchToTab(newTab);
                return true;
            }

        }

        GN_LOG_INFO("PauseSystem: No ribbon button clicked - touch outside all button bounds");
        return false;
    }

    void PauseSystem::HandleSystemTabClick(float touchX, float touchY) {
        GN_LOG_INFO("PauseSystem: Handling system tab click at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        // Check main menu button click (top-left positioned)
        if (m_mainMenuButtonEntity != 0) {
            if (IsTouchInButtonBounds(touchX, touchY, m_mainMenuButtonEntity, false)) {
                GN_LOG_INFO("PauseSystem: Main Menu button clicked at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ") - triggering transition to main menu");
                // Hide the pause menu UI first
                this->Hide();
                // Call ReturnToMainMenu to trigger state transition (matches original implementation)
                if (m_gameplayState) {
                    m_gameplayState->ReturnToMainMenu();
                }
                return;
            }
        }

        // Check vibration toggle click with debouncing
        if (m_vibrationToggleEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_vibrationToggleEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_vibrationToggleEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_vibrationToggleEntity);
            
            if (transform && sprite && uiElement && uiElement->isEnabled && uiElement->visible) {
                float toggleW = sprite->width * transform->scale.x;
                float toggleH = sprite->height * transform->scale.y;
                
                if (touchX >= transform->position.x && touchX <= transform->position.x + toggleW &&
                    touchY >= transform->position.y && touchY <= transform->position.y + toggleH) {
                    
                    // Check debounce - use 0.3s debounce
                    static float lastToggleTime = 0.0f;
                    float currentTime = 0.0f; // Would need to pass deltaTime accumulator
                    // For now, just log and proceed
                    GN_LOG_INFO("PauseSystem: Vibration toggle clicked!");
                    
                    // Toggle vibration state
                    if (GameCore::GetGame()) {
                        bool currentState = GameCore::GetGame()->GetVibrationsEnabled();
                        bool newState = !currentState;
                        GameCore::GetGame()->SetVibrationsEnabled(newState);
                        
                        GN_LOG_INFO("PauseSystem: Vibration toggled to " + std::string(newState ? "ON" : "OFF"));
                        
                        // Use UISystem to atomically update the toggle button
                        if (auto uiSystem = m_ecsCoordinator->GetSystemManager()->GetUISystem()) {
                            uiSystem->SetToggleState(m_vibrationToggleEntity, newState);
                        }
                        
                        // Save settings
                        if (GameCore::GetGame()) {
                            GameCore::GetGame()->SaveSettings();
                        }
                        
                        // Play haptic ONLY if vibrations are enabled (just turned ON)
                        // This confirms the toggle worked by giving immediate feedback
                        if (newState && m_platformDelegates.haptic.triggerImpact) {
                            m_platformDelegates.haptic.triggerImpact(HapticStyle::LIGHT, 0.5f);
                            GN_LOG_INFO("PauseSystem: Triggered haptic feedback for toggle confirmation (vibrations now ON)");
                        }
                    }
                    return;
                }
            }
        }

        // Check for audio slider knob clicks/drags
        HandleKnobDrag(touchX, touchY, TouchState::PRESSED);

        GN_LOG_INFO("PauseSystem: System tab click handled");
    }
    
    void PauseSystem::HandleSkillsTabClick(float touchX, float touchY) {
        GN_LOG_INFO("PauseSystem: Handling skills tab click at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        // Handle skill unlock button clicks (top-left positioned)
        if (m_skillsUnlockButtonEntity != 0) {
            if (IsTouchInButtonBounds(touchX, touchY, m_skillsUnlockButtonEntity, false)) {
                // Check debounce timer
                if (m_lastSkillButtonPressTime < m_skillButtonDebounceDelay) {
                    GN_LOG_INFO("PauseSystem: Skill unlock button debounced - too soon since last press");
                    return;
                }

                GN_LOG_INFO("PauseSystem: Skill unlock button clicked");

                // Sync player coins to ensure we have the latest coin count
                SyncPlayerCoins();

                // Check if player has enough coins before attempting unlock
                if (!m_availableSkills.empty()) {
                    int validIndex = m_currentSkillIndex;
                    if (validIndex < 0) validIndex = m_availableSkills.size() - 1;
                    if (validIndex >= static_cast<int>(m_availableSkills.size())) validIndex = 0;

                    GameCore::SkillType currentSkill = m_availableSkills[validIndex];
                    int cost = m_skillSystem ? m_skillSystem->GetSkillCost(currentSkill) : 0;

                    if (m_playerCoins < cost) {
                        // Play denied sound for insufficient coins
                        if (GameCore::GetGame()) {
                            GN_LOG_INFO("🎵 Playing denied sound - insufficient coins for skill unlock");
                            GameCore::GetGame()->PlaySFX("denied");
                        }
                        // Reset debounce timer even for failed attempts
                        m_lastSkillButtonPressTime = 0.0f;
                        return;
                    }
                }

                HandleSkillUnlock(m_currentSkillIndex, m_availableSkills, m_skillSystem, m_playerCoins, m_playerEntity);
                return;
            }
        }

        // Handle left arrow clicks (top-left positioned)
        if (m_skillsLeftArrowEntity != 0) {
            if (IsTouchInButtonBounds(touchX, touchY, m_skillsLeftArrowEntity, false)) {
                GN_LOG_INFO("PauseSystem: Skills left arrow clicked");
                HandleSkillLeftArrow(m_currentSkillIndex, m_availableSkills);
                return;
            }
        }

        // Handle right arrow clicks (top-left positioned)
        if (m_skillsRightArrowEntity != 0) {
            if (IsTouchInButtonBounds(touchX, touchY, m_skillsRightArrowEntity, false)) {
                GN_LOG_INFO("PauseSystem: Skills right arrow clicked");
                HandleSkillRightArrow(m_currentSkillIndex, m_availableSkills);
                return;
            }
        }

        GN_LOG_INFO("PauseSystem: Skills tab click handled - no specific element clicked");
    }

    void PauseSystem::HandleHatsTabClick(float touchX, float touchY) {
        GN_LOG_INFO("PauseSystem: Handling hats tab click at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        // Handle hat frame clicks for selection
        if (m_hatsSystem && m_ecsCoordinator) {
            int hatCount = m_hatsSystem->GetHatCount();
            GN_LOG_INFO("PauseSystem: Checking " + std::to_string(hatCount) + " hat frames for click detection");

            for (size_t i = 0; i < m_hatFrameEntities.size() && i < (size_t)hatCount; ++i) {
                auto frameEntity = m_hatFrameEntities[i];
                if (frameEntity != 0) {
            // Hat frames are top-left positioned
            if (IsTouchInButtonBounds(touchX, touchY, frameEntity, false)) {
                        // Convert UI index to game index (i=0-14 -> hatIndex=1-15)
                        int hatIndex = i + 1;
                        GN_LOG_INFO("PauseSystem: Hat frame " + std::to_string(i) + " clicked - selecting hat index " + std::to_string(hatIndex));
                        m_hatsSystem->SelectHat(hatIndex);
                        UpdateHatDisplay();
                        return;
                    }
                }
            }
        }

        // Handle action button clicks (top-left positioned)
        if (m_hatsActionButtonEntity != 0 && m_ecsCoordinator) {
            auto* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_hatsActionButtonEntity);

            if (uiElement && uiElement->visible && uiElement->isEnabled) {
                // Check debounce timer to prevent rapid clicking
                if (m_lastActionButtonPressTime < m_actionButtonDebounceDelay) {
                    GN_LOG_INFO("PauseSystem: Hat action button debounced - too soon since last press");
                    return;
                }

                if (IsTouchInButtonBounds(touchX, touchY, m_hatsActionButtonEntity, false)) {
                    GN_LOG_INFO("PauseSystem: Hats action button clicked");

                    // Sync player coins to ensure we have the latest coin count
                    SyncPlayerCoins();

                    int selectedHatIndex = m_hatsSystem->GetSelectedHatIndex();
                    if (selectedHatIndex >= 0) {
                        // Check if hat is locked and try to buy it
                        if (!m_hatsSystem->IsHatUnlocked(selectedHatIndex)) {
                            auto hatData = m_hatsSystem->GetHatData(selectedHatIndex);
                            if (hatData && m_playerCoins >= hatData->cost) {
                                if (m_hatsSystem->BuySelectedHat(m_playerCoins)) {
                                    // NEW LOGIC: Use ALL session coins first, then stored coins
                                    if (m_playerEntity != 0 && m_ecsCoordinator) {
                                        auto* playerComp = m_ecsCoordinator->GetComponent<PlayerComponent>(m_playerEntity);
                                        if (playerComp) {
                                            int cost = hatData->cost;
                                            int remainingCost = cost;

                                            // Step 1: Use ALL available session coins first
                                            if (playerComp->sessionCoins > 0) {
                                                int sessionDeduction = std::min(remainingCost, playerComp->sessionCoins);
                                                playerComp->sessionCoins -= sessionDeduction;
                                                remainingCost -= sessionDeduction;
                                                GN_LOG_INFO("PauseSystem: Used " + std::to_string(sessionDeduction) + " session coins. Remaining cost: " + std::to_string(remainingCost));
                                            }

                                            // Step 2: Use stored coins for remaining cost
                                            if (remainingCost > 0 && playerComp->totalCoins > 0) {
                                                int storedDeduction = std::min(remainingCost, playerComp->totalCoins);
                                                playerComp->totalCoins -= storedDeduction;
                                                remainingCost -= storedDeduction;
                                                GN_LOG_INFO("PauseSystem: Used " + std::to_string(storedDeduction) + " stored coins. Remaining cost: " + std::to_string(remainingCost));
                                            }

                                            // Step 3: Update GameStats stored coins
                                            if (GameCore::GetGame()) {
                                                GameCore::FloppyTurdGame::GameStats gameStats = GameCore::GetGame()->GetGameStats();
                                                gameStats.storedCoins = playerComp->totalCoins;
                                                GameCore::GetGame()->UpdateGameStats(gameStats);
                                            }

                                            // Verify purchase was successful (remainingCost should be 0)
                                            if (remainingCost == 0) {
                                                GN_LOG_INFO("PauseSystem: Hat purchase successful - sessionCoins: " + std::to_string(playerComp->sessionCoins) + ", totalCoins: " + std::to_string(playerComp->totalCoins));
                                            } else {
                                                GN_LOG_ERROR("PauseSystem: Hat purchase incomplete - " + std::to_string(remainingCost) + " cost remaining!");
                                            }

                                            // Update our cached total
                                            m_playerCoins = playerComp->sessionCoins + playerComp->totalCoins;
                                        }
                                    }
                                    GN_LOG_INFO("PauseSystem: Hat purchased successfully - playing kaching");
                                    // Play kaching sound for successful purchase
                                    if (GameCore::GetGame()) {
                                        GameCore::GetGame()->PlaySFX("kaching");
                                    }
                                    UpdateHatDisplay();

                                    // UPDATE DISPLAYS: Refresh all relevant displays with new coin totals after purchase
                                    if (m_playerEntity != 0 && m_ecsCoordinator) {
                                        auto* playerComp = m_ecsCoordinator->GetComponent<PlayerComponent>(m_playerEntity);
                                        if (playerComp) {
                                            // Get updated total spendable coins (stored + session) and gross total
                                            int updatedTotalSpendable = playerComp->totalCoins + playerComp->sessionCoins;
                                            int updatedGrossTotal = 0;
                                            if (GameCore::GetGame()) {
                                                updatedGrossTotal = GameCore::GetGame()->GetGameStats().totalCoinsCollected;
                                            }

                                            // Update pause system stats data with current session coins
                                            int updatedSessionCoins = playerComp->sessionCoins;
                                            GN_LOG_INFO("📊 Post-purchase data update - totalSpendable: " + std::to_string(updatedTotalSpendable) + ", sessionCoins: " + std::to_string(updatedSessionCoins) + ", grossTotal: " + std::to_string(updatedGrossTotal));
                                            UpdateStatsData(m_sessionPipes, updatedSessionCoins, updatedTotalSpendable, updatedGrossTotal, m_totalFlops, m_enemiesKilled, m_totalPipes);

                                            // Refresh the current tab display immediately
                                            if (m_currentTab == PauseMenuTab::STATS) {
                                                RefreshStatsDisplay(m_sessionPipes, updatedSessionCoins, updatedTotalSpendable, updatedGrossTotal, m_totalFlops, m_enemiesKilled, m_totalPipes);
                                            } else if (m_currentTab == PauseMenuTab::HATS) {
                                                // Update hat display to show new coin count
                                                UpdateHatDisplay();
                                            }
                                        }
                                    }
                                }
                            } else {
                                // Play denied sound for insufficient coins
                                if (GameCore::GetGame()) {
                                    GN_LOG_INFO("🎵 Playing denied sound - insufficient coins for hat");
                                    GameCore::GetGame()->PlaySFX("denied");
                                }
                            }
                        } else {
                            // Hat is unlocked - check if we should equip or unequip
                            if (selectedHatIndex == m_hatsSystem->GetEquippedHatIndex()) {
                                // Unequip the hat
                                m_hatsSystem->UnequipHat();
                                GN_LOG_INFO("PauseSystem: Hat unequipped");
                            } else {
                                // Equip the selected hat
                                m_hatsSystem->EquipSelectedHat();
                                GN_LOG_INFO("PauseSystem: Hat equipped");
                            }
                            UpdateHatDisplay();
                        }
                    }
                    // Reset debounce timer
                    m_lastActionButtonPressTime = 0.0f;
                    return;
                }
            }
        }

        GN_LOG_INFO("PauseSystem: Hats tab click handled - no hat frame or button clicked");
    }

    void PauseSystem::HandleStatsTabClick(float touchX, float touchY) {
        GN_LOG_INFO("PauseSystem: Handling stats tab click at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");
        // Stats tab is currently just for display - no interactive elements
        GN_LOG_INFO("PauseSystem: Stats tab click handled - no interactive elements");
    }

    void PauseSystem::HandleKnobDrag(float touchX, float touchY, TouchState touchState) {
        GN_LOG_INFO("PauseSystem: Handling knob drag at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + ")");

        // Handle RELEASED state to stop dragging
        if (touchState == TouchState::RELEASED) {
            if (m_draggedKnobEntity != 0) {
                GN_LOG_INFO("PauseSystem: RELEASED - stopping drag of knob entity " + std::to_string(m_draggedKnobEntity));
                StopDragging();
            }
            return;
        }

        // Define track positions and hitboxes (matching MainMenuState approach)
        float knobSize = 16.0f * 8.0f; // 128px (same as calculation below)
        float masterTrackY = m_sliderY + 9.0f;
        float musicTrackY = m_sliderY + m_sliderSpacing + 9.0f;
        float sfxTrackY = m_sliderY + m_sliderSpacing * 2 + 9.0f;

        // Precise hitbox for each track (includes track + knob margin)
        float hitLeft = m_sliderX - knobSize * 0.5f;
        float hitRight = m_sliderX + m_sliderW + knobSize * 0.5f;

        // Helper lambda to handle slider interaction (similar to MainMenuState)
        auto handleSlider = [&](Entity knobEntity, float& sliderValue, bool& draggingFlag, int knobIndex, float trackY) {
            float hitTop = trackY - knobSize * 0.5f;
            float hitBottom = trackY + m_sliderH + knobSize * 0.5f;

            // Check if touch is within this track's hitbox
            if (touchX >= hitLeft && touchX <= hitRight && touchY >= hitTop && touchY <= hitBottom) {
                // PRESSED state - start dragging and snap knob to touch position immediately
                if (touchState == TouchState::PRESSED) {
                    if (m_activeDragKnob == -1) {  // Only start if no active drag
                        // Stop any previous drag
                        if (m_draggedKnobEntity != 0) {
                            StopDragging();
                        }

                        // Start dragging this knob
                        m_draggedKnobEntity = knobEntity;
                        m_activeDragKnob = knobIndex;
                        draggingFlag = true;

                        // Absolute mapping: calculate value directly from touch X
                        float normalizedValue = (touchX - m_sliderX) / m_sliderW;
                        normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
                        sliderValue = normalizedValue;

                        // Update knob position immediately
                        if (knobEntity != 0 && m_ecsCoordinator) {
                            auto* transform = m_ecsCoordinator->GetComponent<Transform>(knobEntity);
                            if (transform) {
                                float knobCenterX = m_sliderX + normalizedValue * m_sliderW;
                                transform->position.x = knobCenterX - (knobSize * 0.5f);
                                transform->position.y = trackY + m_sliderH * 0.5f - (knobSize * 0.5f);
                            }
                        }

                        // Apply to audio system
                        if (GameCore::GetGame()) {
                            if (knobIndex == 0) {
                                GameCore::GetGame()->SetMasterVolume(normalizedValue);
                                GN_LOG_INFO("PauseSystem: PRESSED master track - set volume to " + std::to_string(normalizedValue));
                            } else if (knobIndex == 1) {
                                GameCore::GetGame()->SetMusicVolume(normalizedValue);
                                GN_LOG_INFO("PauseSystem: PRESSED music track - set volume to " + std::to_string(normalizedValue));
                            } else if (knobIndex == 2) {
                                GameCore::GetGame()->SetSFXVolume(normalizedValue);
                                GN_LOG_INFO("PauseSystem: PRESSED sfx track - set volume to " + std::to_string(normalizedValue));
                            }
                        }
                        return true;
                    }
                }

                // HELD state - continuous updates during drag
                if (draggingFlag && m_activeDragKnob == knobIndex && (touchState == TouchState::HELD || touchState == TouchState::PRESSED)) {
                    // Map touch X to slider value directly
                    float normalizedValue = (touchX - m_sliderX) / m_sliderW;
                    normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
                    sliderValue = normalizedValue;

                    // Update knob position immediately
                    if (knobEntity != 0 && m_ecsCoordinator) {
                        auto* transform = m_ecsCoordinator->GetComponent<Transform>(knobEntity);
                        if (transform) {
                            float knobCenterX = m_sliderX + normalizedValue * m_sliderW;
                            transform->position.x = knobCenterX - (knobSize * 0.5f);
                            transform->position.y = trackY + m_sliderH * 0.5f - (knobSize * 0.5f);
                        }
                    }

                    // Apply to audio system
                    if (GameCore::GetGame()) {
                        if (knobIndex == 0) {
                            GameCore::GetGame()->SetMasterVolume(normalizedValue);
                            GN_LOG_INFO("PauseSystem: HELD master track - set volume to " + std::to_string(normalizedValue));
                        } else if (knobIndex == 1) {
                            GameCore::GetGame()->SetMusicVolume(normalizedValue);
                            GN_LOG_INFO("PauseSystem: HELD music track - set volume to " + std::to_string(normalizedValue));
                        } else if (knobIndex == 2) {
                            GameCore::GetGame()->SetSFXVolume(normalizedValue);
                            GN_LOG_INFO("PauseSystem: HELD sfx track - set volume to " + std::to_string(normalizedValue));
                        }
                    }
                    return true;
                }
            }
            return false;
        };

        // Try each slider in order (master, music, sfx)
        if (handleSlider(m_masterKnobEntity, m_masterSliderValue, m_draggingMaster, 0, masterTrackY)) return;
        if (handleSlider(m_musicKnobEntity, m_musicSliderValue, m_draggingMusic, 1, musicTrackY)) return;
        if (handleSlider(m_sfxKnobEntity, m_sfxSliderValue, m_draggingSFX, 2, sfxTrackY)) return;

        GN_LOG_INFO("PauseSystem: Touch outside all slider tracks");
    }

    void PauseSystem::StopDragging() {
        GN_LOG_INFO("PauseSystem: Stopping all dragging");

        m_draggingMaster = false;
        m_draggingMusic = false;
        m_draggingSFX = false;
        m_activeDragKnob = -1;
        m_draggedKnobEntity = 0;
        m_dragOffsetX = 0.0f;
    }

    void PauseSystem::UpdateSkillDisplay(int currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills,
                                       SkillSystem* skillSystem, int /*unused*/, Entity playerEntity) {
        GN_LOG_INFO("PauseSystem: Updating skill display with external data");

        if (!skillSystem || availableSkills.empty()) {
            GN_LOG_WARN("PauseSystem: Cannot update skill display - missing skill system or no skills available");
            return;
        }

        // Ensure current index is valid
        int validIndex = currentSkillIndex;
        if (validIndex < 0) {
            validIndex = availableSkills.size() - 1;
        } else if (validIndex >= static_cast<int>(availableSkills.size())) {
            validIndex = 0;
        }

        GameCore::SkillType currentSkill = availableSkills[validIndex];

        // Update skill name
        if (m_skillsNameEntity != 0 && m_ecsCoordinator) {
            UIElement* nameElem = m_ecsCoordinator->GetComponent<UIElement>(m_skillsNameEntity);
            if (nameElem) {
                nameElem->buttonText = skillSystem->GetSkillDisplayName(currentSkill);
                nameElem->visible = true;
            }
        }

        // Update skill description
        if (m_skillsDescriptionEntity != 0 && m_ecsCoordinator) {
            UIElement* descElem = m_ecsCoordinator->GetComponent<UIElement>(m_skillsDescriptionEntity);
            if (descElem) {
                descElem->buttonText = skillSystem->GetSkillDescription(currentSkill);
                descElem->visible = true;
            }
        }

        // Update cost and button state
        if (m_skillsCostEntity != 0 && m_skillsUnlockButtonEntity != 0 && m_ecsCoordinator) {
            UIElement* costElem = m_ecsCoordinator->GetComponent<UIElement>(m_skillsCostEntity);
            UIElement* buttonElem = m_ecsCoordinator->GetComponent<UIElement>(m_skillsUnlockButtonEntity);

            if (costElem && buttonElem) {
                // Sync player coins to ensure we have the latest coin count
                SyncPlayerCoins();

                bool isUnlocked = skillSystem->IsSkillUnlocked(currentSkill);
                int cost = skillSystem->GetSkillCost(currentSkill);

                if (isUnlocked) {
                    costElem->buttonText = "UNLOCKED";
                    costElem->textColor = GNColor(0, 255, 0, 255); // Green
                    buttonElem->buttonText = "UNLOCKED";
                    buttonElem->isEnabled = false;
                } else {
                    costElem->buttonText = "Cost: " + std::to_string(cost) + " coins";
                    costElem->textColor = GNColor(255, 215, 0, 255); // Gold

                    GN_LOG_INFO("PauseSystem: Skill coin check - playerCoins: " + std::to_string(m_playerCoins) + ", cost: " + std::to_string(cost));
                    if (m_playerCoins >= cost) {
                        buttonElem->buttonText = "BUY";
                        buttonElem->isEnabled = true;
                        buttonElem->textColor = GNColor(255, 255, 255, 255); // White for sufficient coins
                        GN_LOG_INFO("PauseSystem: Skill button set to BUY");
                    } else {
                        buttonElem->buttonText = "Not enough coins";
                        buttonElem->isEnabled = false;
                        buttonElem->textColor = GNColor(255, 100, 100, 255); // Red for insufficient coins - match hats tab
                        costElem->textColor = GNColor(255, 100, 100, 255); // Red cost text when insufficient coins
                        GN_LOG_INFO("PauseSystem: Skill button set to NOT ENOUGH COINS");
                    }
                }

                costElem->visible = true;
                buttonElem->visible = true;
            }
        }

        GN_LOG_INFO("PauseSystem: Skill display updated for skill: " + skillSystem->GetSkillDisplayName(currentSkill));
    }

    void PauseSystem::UpdateSkillDisplayFromCurrentState() {
        GN_LOG_INFO("PauseSystem: Updating skill display from current state");
        // Sync player coins to ensure we have the latest coin count
        SyncPlayerCoins();
        UpdateSkillDisplay(m_currentSkillIndex, m_availableSkills, m_skillSystem, 0, m_playerEntity);
    }

    void PauseSystem::HandleSkillLeftArrow(int& currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills) {
        GN_LOG_INFO("PauseSystem: Handling skill left arrow");

        // Check debounce timer to prevent rapid clicking
        if (m_lastSkillButtonPressTime < m_skillButtonDebounceDelay) {
            GN_LOG_INFO("PauseSystem: Skill left arrow debounced - too soon since last press");
            return;
        }

        if (availableSkills.empty()) {
            GN_LOG_WARN("PauseSystem: Cannot navigate skills - no skills available");
            return;
        }

        // Navigate to previous skill
        currentSkillIndex--;
        if (currentSkillIndex < 0) {
            currentSkillIndex = availableSkills.size() - 1;
        }

        // Update skill display
        UpdateSkillDisplay(currentSkillIndex, availableSkills, m_skillSystem, 0, m_playerEntity);

        // Reset debounce timer
        m_lastSkillButtonPressTime = 0.0f;

        GN_LOG_INFO("PauseSystem: Navigated to previous skill (index: " + std::to_string(currentSkillIndex) + ")");
    }

    void PauseSystem::HandleSkillRightArrow(int& currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills) {
        GN_LOG_INFO("PauseSystem: Handling skill right arrow");

        // Check debounce timer to prevent rapid clicking
        if (m_lastSkillButtonPressTime < m_skillButtonDebounceDelay) {
            GN_LOG_INFO("PauseSystem: Skill right arrow debounced - too soon since last press");
            return;
        }

        if (availableSkills.empty()) {
            GN_LOG_WARN("PauseSystem: Cannot navigate skills - no skills available");
            return;
        }

        // Navigate to next skill
        currentSkillIndex++;
        if (currentSkillIndex >= static_cast<int>(availableSkills.size())) {
            currentSkillIndex = 0;
        }

        // Update skill display
        UpdateSkillDisplay(currentSkillIndex, availableSkills, m_skillSystem, 0, m_playerEntity);

        // Reset debounce timer
        m_lastSkillButtonPressTime = 0.0f;

        GN_LOG_INFO("PauseSystem: Navigated to next skill (index: " + std::to_string(currentSkillIndex) + ")");
    }

    void PauseSystem::HandleSkillUnlock(int currentSkillIndex, const std::vector<GameCore::SkillType>& availableSkills,
                                       SkillSystem* skillSystem, int& playerCoins, Entity playerEntity) {
        GN_LOG_INFO("PauseSystem: Handling skill unlock");

        // Check debounce timer to prevent rapid clicking
        if (m_lastSkillButtonPressTime < m_skillButtonDebounceDelay) {
            GN_LOG_INFO("PauseSystem: Skill unlock button debounced - too soon since last press");
            return;
        }

        if (!skillSystem || availableSkills.empty()) {
            GN_LOG_WARN("PauseSystem: Cannot unlock skill - missing skill system or no skills available");
            return;
        }

        // Ensure current index is valid
        int validIndex = currentSkillIndex;
        if (validIndex < 0) {
            validIndex = availableSkills.size() - 1;
        } else if (validIndex >= static_cast<int>(availableSkills.size())) {
            validIndex = 0;
        }

        GameCore::SkillType currentSkill = availableSkills[validIndex];

        if (skillSystem->IsSkillUnlocked(currentSkill)) {
            GN_LOG_INFO("PauseSystem: Skill already unlocked: " + skillSystem->GetSkillDisplayName(currentSkill));
            return; // Already unlocked
        }

        // Try to unlock the skill
        if (skillSystem->UnlockSkill(currentSkill, playerCoins)) {
            GN_LOG_INFO("PauseSystem: Skill unlocked successfully: " + skillSystem->GetSkillDisplayName(currentSkill));

            // Deduct coins from player - properly deduct from stored coins first
            int cost = skillSystem->GetSkillCost(currentSkill);
            if (playerEntity != 0 && m_ecsCoordinator) {
                auto* playerComp = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
                if (playerComp) {
                    int remainingCost = cost;

                    // NEW LOGIC: Use ALL session coins first, then stored coins
                    // Step 1: Use ALL available session coins first
                    if (playerComp->sessionCoins > 0) {
                        int sessionDeduction = std::min(remainingCost, playerComp->sessionCoins);
                        playerComp->sessionCoins -= sessionDeduction;
                        remainingCost -= sessionDeduction;
                        GN_LOG_INFO("PauseSystem: Used " + std::to_string(sessionDeduction) + " session coins for skill. Remaining cost: " + std::to_string(remainingCost));
                    }

                    // Step 2: Use stored coins for remaining cost
                    if (remainingCost > 0 && playerComp->totalCoins > 0) {
                        int storedDeduction = std::min(remainingCost, playerComp->totalCoins);
                        playerComp->totalCoins -= storedDeduction;
                        remainingCost -= storedDeduction;
                        GN_LOG_INFO("PauseSystem: Used " + std::to_string(storedDeduction) + " stored coins for skill. Remaining cost: " + std::to_string(remainingCost));
                    }

                    // Step 3: Update GameStats stored coins
                    if (GameCore::GetGame()) {
                        GameCore::FloppyTurdGame::GameStats gameStats = GameCore::GetGame()->GetGameStats();
                        gameStats.storedCoins = playerComp->totalCoins;
                        GameCore::GetGame()->UpdateGameStats(gameStats);
                    }

                    // Verify purchase was successful (remainingCost should be 0)
                    if (remainingCost == 0) {
                        GN_LOG_INFO("PauseSystem: Skill purchase successful - sessionCoins: " + std::to_string(playerComp->sessionCoins) + ", totalCoins: " + std::to_string(playerComp->totalCoins));
                    } else {
                        GN_LOG_ERROR("PauseSystem: Skill purchase incomplete - " + std::to_string(remainingCost) + " cost remaining!");
                    }

                    // Update the local playerCoins variable for consistency
                    playerCoins = playerComp->sessionCoins + playerComp->totalCoins;
                    // Update our cached total
                    m_playerCoins = playerCoins;
                }
            }

            // Play unlock sound
            if (GameCore::GetGame()) {
                GN_LOG_INFO("🎵 Playing kaching sound for skill unlock");
                GameCore::GetGame()->PlaySFX("kaching");
            }

            // UPDATE DISPLAYS: Refresh all relevant displays with new coin totals after purchase
            if (playerEntity != 0 && m_ecsCoordinator) {
                auto* playerComp = m_ecsCoordinator->GetComponent<PlayerComponent>(playerEntity);
                if (playerComp) {
                    // Get updated total spendable coins (stored + session) and gross total
                    int updatedTotalSpendable = playerComp->totalCoins + playerComp->sessionCoins;
                    int updatedGrossTotal = 0;
                    if (GameCore::GetGame()) {
                        updatedGrossTotal = GameCore::GetGame()->GetGameStats().totalCoinsCollected;
                    }

                    // Update pause system stats data with current session coins
                    int updatedSessionCoins = playerComp->sessionCoins;
                    GN_LOG_INFO("📊 Post-skill-purchase data update - totalSpendable: " + std::to_string(updatedTotalSpendable) + ", sessionCoins: " + std::to_string(updatedSessionCoins) + ", grossTotal: " + std::to_string(updatedGrossTotal));
                    UpdateStatsData(m_sessionPipes, updatedSessionCoins, updatedTotalSpendable, updatedGrossTotal, m_totalFlops, m_enemiesKilled, m_totalPipes);

                    // Refresh the current tab display immediately
                    if (m_currentTab == PauseMenuTab::STATS) {
                        RefreshStatsDisplay(m_sessionPipes, updatedSessionCoins, updatedTotalSpendable, updatedGrossTotal, m_totalFlops, m_enemiesKilled, m_totalPipes);
                    } else if (m_currentTab == PauseMenuTab::SKILLS) {
                        // Update skill display to show new coin count and unlock status
                        UpdateSkillDisplay(m_currentSkillIndex, m_availableSkills, m_skillSystem, playerComp->sessionCoins + playerComp->totalCoins, playerEntity);
                    }
                }
            }

            // Display update is now handled above with the new coin totals
        } else {
            GN_LOG_WARN("PauseSystem: Failed to unlock skill: " + skillSystem->GetSkillDisplayName(currentSkill));

            // Play denied sound when unlock fails (insufficient coins)
            if (GameCore::GetGame()) {
                GN_LOG_INFO("🎵 Playing denied sound - insufficient coins");
                GameCore::GetGame()->PlaySFX("denied");
            }
        }

        // Reset debounce timer
        m_lastSkillButtonPressTime = 0.0f;
    }

    void PauseSystem::UpdateGameStatsFromSession(int sessionPipes, int sessionCoins, int totalCoins, int grossTotalCoins, int totalFlops,
                                               int enemiesKilled, int totalPipes) {
        GN_LOG_INFO("PauseSystem: Updating game stats from session");
        GN_LOG_INFO("PauseSystem: Session data - Pipes: " + std::to_string(sessionPipes) +
                   ", Coins: " + std::to_string(sessionCoins) + ", Total Coins: " + std::to_string(totalCoins) +
                   ", Flops: " + std::to_string(totalFlops) + ", Enemies: " + std::to_string(enemiesKilled) +
                   ", Total Pipes: " + std::to_string(totalPipes));

        RefreshStatsDisplay(sessionPipes, sessionCoins, totalCoins, grossTotalCoins, totalFlops, enemiesKilled, totalPipes);
    }

    void PauseSystem::RefreshStatsDisplay(int sessionPipes, int sessionCoins, int totalCoins, int grossTotalCoins, int totalFlops,
                                         int enemiesKilled, int totalPipes) {
        GN_LOG_INFO("PauseSystem: Refreshing stats display with real data");

        // Update each stat entity with current values
        // For Level 6 (Rat King), show "Session Timer" instead of "Session Pipes"
        if (m_currentSessionTextEntity != 0 && m_ecsCoordinator) {
            UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(m_currentSessionTextEntity);
            if (uiElem) {
                if (m_gameplayState && m_gameplayState->GetCurrentLevelId() == 6) {
                    // Show Session Timer for Rat King level
                    float currentTimer = m_gameplayState->GetBossLevelTimer();
                    int minutes = static_cast<int>(currentTimer) / 60;
                    int seconds = static_cast<int>(currentTimer) % 60;
                    char timerBuffer[32];
                    snprintf(timerBuffer, sizeof(timerBuffer), "Session Timer: %02d:%02d", minutes, seconds);
                    uiElem->buttonText = timerBuffer;
                } else {
                    uiElem->buttonText = "Session Pipes: " + std::to_string(sessionPipes);
                }
                uiElem->visible = true;
            }
        }

        if (m_sessionCoinsTextEntity != 0 && m_ecsCoordinator) {
            UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(m_sessionCoinsTextEntity);
            if (uiElem) {
                uiElem->buttonText = "Session Coins: " + std::to_string(sessionCoins);
                uiElem->visible = true;
            }
        }

        if (m_totalCoinsTextEntity != 0 && m_ecsCoordinator) {
            UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(m_totalCoinsTextEntity);
            if (uiElem) {
                // Show total spendable coins (stored + session)
                uiElem->buttonText = "Current Total Coins: " + std::to_string(totalCoins);
                uiElem->visible = true;
                GN_LOG_INFO("💰 Display - Current Total (spendable): " + std::to_string(totalCoins));
            }
        }

        if (m_grossTotalCoinsTextEntity != 0 && m_ecsCoordinator) {
            UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(m_grossTotalCoinsTextEntity);
            if (uiElem) {
                uiElem->buttonText = "Gross Total Coins: " + std::to_string(grossTotalCoins);
                uiElem->visible = true;
                GN_LOG_INFO("💰 Display - Gross Total: " + std::to_string(grossTotalCoins));
            }
        }

        if (m_totalFlopsTextEntity != 0 && m_ecsCoordinator) {
            UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(m_totalFlopsTextEntity);
            if (uiElem) {
                uiElem->buttonText = "Total Flops: " + std::to_string(totalFlops);
                uiElem->visible = true;
            }
        }

        if (m_enemiesKilledTextEntity != 0 && m_ecsCoordinator) {
            UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(m_enemiesKilledTextEntity);
            if (uiElem) {
                uiElem->buttonText = "Enemies Defeated: " + std::to_string(enemiesKilled);
                uiElem->visible = true;
            }
        }

        if (m_totalPipesTextEntity != 0 && m_ecsCoordinator) {
            UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(m_totalPipesTextEntity);
            if (uiElem) {
                uiElem->buttonText = "Total Pipes: " + std::to_string(totalPipes);
                uiElem->visible = true;
            }
        }

        // Update Rat King Boss Record (Level 6 only) - shows best time, not current
        if (m_bossTimerTextEntity != 0 && m_ecsCoordinator && m_gameplayState) {
            if (m_gameplayState->GetCurrentLevelId() == 6) {
                UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(m_bossTimerTextEntity);
                if (uiElem) {
                    // Get best time from level stats
                    const auto& levelStats = GameCore::GetGame()->GetLevelStats(6);
                    float bestTime = levelStats.bestBossTime;
                    
                    if (bestTime > 0.0f) {
                        // Format as MM:SS
                        int minutes = static_cast<int>(bestTime) / 60;
                        int seconds = static_cast<int>(bestTime) % 60;
                        
                        char timerBuffer[32];
                        snprintf(timerBuffer, sizeof(timerBuffer), "Rat King Record: %02d:%02d", minutes, seconds);
                        uiElem->buttonText = timerBuffer;
                    } else {
                        uiElem->buttonText = "Rat King Record: --:--";
                    }
                    uiElem->visible = true;
                }
            } else {
                // Hide timer if not on Level 6
                UIElement* uiElem = m_ecsCoordinator->GetComponent<UIElement>(m_bossTimerTextEntity);
                if (uiElem) {
                    uiElem->visible = false;
                }
            }
        }

        // Update level high scores
        // Hide level high scores section for Level 6 (Rat King) in landscape mode
        bool isLevel6 = (m_gameplayState && m_gameplayState->GetCurrentLevelId() == 6);
        bool isLandscape = IsLandscapeMode();
        bool hideLevelScores = (isLevel6 && isLandscape);
        
        if (!m_levelHighScoreTextEntities.empty() && GameCore::GetGame()) {
            const char* levelNames[] = {"Park", "Sewer", "Desert", "Snow", "Castle", "Boss"};
            
            // First entity is the title - hide for Level 6 landscape
            if (m_levelHighScoreTextEntities[0] != 0) {
                UIElement* titleElem = m_ecsCoordinator->GetComponent<UIElement>(m_levelHighScoreTextEntities[0]);
                if (titleElem) {
                    titleElem->visible = !hideLevelScores;
                }
            }
            
            // Update each level's high score (entities 1-6 correspond to levels 1-6)
            for (int i = 0; i < 6; i++) {
                int entityIndex = i + 1; // +1 to skip title entity
                if (entityIndex < m_levelHighScoreTextEntities.size() && m_levelHighScoreTextEntities[entityIndex] != 0) {
                    UIElement* levelElem = m_ecsCoordinator->GetComponent<UIElement>(m_levelHighScoreTextEntities[entityIndex]);
                    if (levelElem) {
                        int levelId = i + 1; // Level IDs are 1-based
                        
                        if (levelId == 6) {
                            // Boss level - show time in MM:SS format
                            const auto& levelStats = GameCore::GetGame()->GetLevelStats(6);
                            float bossTime = levelStats.bestBossTime;
                            
                            if (bossTime > 0.0f) {
                                int minutes = static_cast<int>(bossTime) / 60;
                                int seconds = static_cast<int>(bossTime) % 60;
                                char timeStr[16];
                                snprintf(timeStr, sizeof(timeStr), "%d:%02d", minutes, seconds);
                                levelElem->buttonText = std::string("Boss: ") + timeStr;
                            } else {
                                levelElem->buttonText = "Boss: --:--";
                            }
                        } else {
                            // Regular levels - show pipes
                            int highScore = GameCore::GetGame()->GetLevelHighScore(levelId);
                            levelElem->buttonText = std::string(levelNames[i]) + ": " + std::to_string(highScore) + " pipes";
                        }
                        // Hide all level scores for Level 6 landscape
                        levelElem->visible = !hideLevelScores;
                    }
                }
            }
            
            GN_LOG_INFO("PauseSystem: Level high scores refreshed");
        }

        GN_LOG_INFO("PauseSystem: Stats display refreshed with real data");
    }

    void PauseSystem::IncrementDeathCounter() {
        GN_LOG_INFO("PauseSystem: Incrementing death counter");

        if (!GameCore::GetGame()) {
            GN_LOG_WARN("PauseSystem: No game instance available for death counter increment");
            return;
        }

        // Get current game stats
        GameCore::FloppyTurdGame::GameStats currentStats = GameCore::GetGame()->GetGameStats();

        // Increment death count
        currentStats.totalDeaths++;

        // Update the game stats
        GameCore::GetGame()->UpdateGameStats(currentStats);

        GN_LOG_INFO("PauseSystem: Death counter incremented to: " + std::to_string(currentStats.totalDeaths));
    }

    bool PauseSystem::IsTapOutsideMenuArea(float touchX, float touchY) const {
        GN_LOG_INFO("PauseSystem: Checking if tap is outside menu area");

        // Define menu area bounds to match the actual pause menu background dimensions
        float originalWidth, originalHeight, bgScale;

        if (IsLandscapeMode()) {
            // Landscape mode: background is rotated 90 degrees (300x160 scaled 7x = 2100x1120)
            originalWidth = 300.0f;   // Rotated: was 160, now 300
            originalHeight = 160.0f;  // Rotated: was 300, now 160
            bgScale = 7.0f;
        } else {
            // Portrait mode: 160x300 scaled 7x = 1120x2100
            originalWidth = 160.0f;   // Original texture width
            originalHeight = 300.0f;  // Original texture height
            bgScale = 7.0f;
        }

        // SCALE FIRST, then center: Use same logic as background creation
        float scaledWidth = originalWidth * bgScale;
        float scaledHeight = originalHeight * bgScale;
        GNVector2 bgPosition = CenterObjectAtPosition(m_screenWidth * 0.5f, m_screenHeight * 0.5f + 32.0f, scaledWidth, scaledHeight);
        float menuLeft = bgPosition.x;
        float menuRight = bgPosition.x + scaledWidth;
        float menuTop = bgPosition.y;
        float menuBottom = bgPosition.y + scaledHeight;

        // For landscape mode: exclude the top margin where tab buttons are located
        float topMarginExclusion = 0.0f;
        if (IsLandscapeMode()) {
            // Tab buttons are positioned at 8% from top, with height of ~96 pixels (16*6)
            // Add some padding around them
            topMarginExclusion = m_screenHeight * 0.08f + 120.0f; // Exclude top 8% + 120px padding
        }

        // Use actual background dimensions to determine what's outside the menu
        // Calculate the actual background bounds that are visible
        float bgWidth, bgHeight;
        if (IsLandscapeMode()) {
            // Landscape: background is rotated 90 degrees (300x160 scaled 7x = 2100x1120)
            bgWidth = 300.0f * 7.0f;   // 2100 pixels wide
            bgHeight = 160.0f * 7.0f;  // 1120 pixels tall
        } else {
            // Portrait: 160x300 scaled 7x = 1120x2100
            bgWidth = 160.0f * 7.0f;   // 1120 pixels wide
            bgHeight = 300.0f * 7.0f;  // 2100 pixels tall
        }

        // Calculate actual background bounds (centered on screen)
        float bgLeft = (m_screenWidth - bgWidth) * 0.5f;
        float bgRight = bgLeft + bgWidth;
        float bgTop = (m_screenHeight - bgHeight) * 0.5f + 32.0f;
        float bgBottom = bgTop + bgHeight;

        // DISABLED: No tap outside logic - only settings button can close menu in landscape
        // This prevents accidental closes while allowing deliberate settings button closes
        bool outsideMenu = false;

        // EXCLUDE the settings button area from "outside menu" check
        if (IsTapInSettingsButtonArea(touchX, touchY)) {
            GN_LOG_INFO("PauseSystem: Tap is in settings button area - not outside menu");
            return false;
        }

        GN_LOG_INFO("PauseSystem: Tap outside check - Touch: (" + std::to_string(touchX) + ", " + std::to_string(touchY) +
                   ") - Outside: " + std::to_string(outsideMenu) +
                   " (closes if x < 200 or x > 2300)");

        return outsideMenu;
    }

    bool PauseSystem::IsTapInSettingsButtonArea(float touchX, float touchY) const {
        GN_LOG_INFO("PauseSystem: Checking if tap is in settings button area");

        // Delegate to GameplayState's version which uses the actual button position
        // This ensures we check the correct area regardless of orientation
        if (m_gameplayState) {
            bool isInArea = m_gameplayState->IsTapInSettingsButtonArea(touchX, touchY);
            GN_LOG_INFO("PauseSystem: Delegating to GameplayState - result: " + std::to_string(isInArea));
        return isInArea;
        } else {
            GN_LOG_WARN("PauseSystem: No GameplayState reference available for settings button check");
            return false;
        }
    }


// Helper function for consistent button collision detection
bool PauseSystem::IsTouchInButtonBounds(float touchX, float touchY, Entity buttonEntity, bool isCentered) {
    if (buttonEntity == 0 || !m_ecsCoordinator) {
        return false;
    }

    auto transform = m_ecsCoordinator->GetComponent<Transform>(buttonEntity);
    auto sprite = m_ecsCoordinator->GetComponent<Sprite>(buttonEntity);

    if (!transform || !sprite) {
        return false;
    }

    float scaleX = transform->scale.x;
    float scaleY = transform->scale.y;
    float width = sprite->width * scaleX;
    float height = sprite->height * scaleY;

    float left, top;

    if (isCentered) {
        // Centered positioning: transform position is center
        left = transform->position.x - (width * 0.5f);
        top = transform->position.y - (height * 0.5f);
    } else {
        // Top-left positioning: transform position is top-left corner
        left = transform->position.x;
        top = transform->position.y;
    }

    float right = left + width;
    float bottom = top + height;

    GN_LOG_INFO("PauseSystem: Button bounds check - touch(" + std::to_string(touchX) + "," + std::to_string(touchY) +
               ") vs bounds(" + std::to_string(left) + "," + std::to_string(top) + "," +
               std::to_string(right) + "," + std::to_string(bottom) + ") [centered=" + std::to_string(isCentered) + "]");

    return (touchX >= left && touchX <= right && touchY >= top && touchY <= bottom);
}

} // namespace GameCore
