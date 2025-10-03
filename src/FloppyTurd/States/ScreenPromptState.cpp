#include "ScreenPromptState.h"
#include "../Game/FloppyTurdGame.h"
#include "../../Engine/Utility/Utils.h"

namespace GameCore {

    ScreenPromptState::ScreenPromptState(Gnosis::ECS* ecsSystem, GameCore::PlatformDelegates* platformDelegates, bool waitForLandscape)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_finished(false)
        , m_displayTime(0.0f)
        , m_fadeTimer(0.0f)
        , m_fadingOut(false)
        , m_backgroundEntity(0)
        , m_textEntity(0)
        , m_arrowsEntity(0)
        , m_uiInitialized(false)
        , m_landscapeDetectedTime(0.0f)
        , m_hasSeenPortrait(false)
        , m_waitForLandscape(waitForLandscape)
        , m_currentOrientation(waitForLandscape ? true : false)  // Start with opposite of target orientation
        , m_currentScreenWidth(1179.0f)
        , m_currentScreenHeight(2556.0f)
    {
        // Cache global game pointer like MainMenuState does
        extern FloppyTurdGame* g_Game;
        m_game = g_Game;

        GN_LOG_INFO("ScreenPromptState created");
    }

    ScreenPromptState::~ScreenPromptState() {
        GN_LOG_INFO("ScreenPromptState destroyed");
    }

    void ScreenPromptState::Enter() {
        GN_LOG_INFO("Entering Screen Prompt State");

        // Reset state for clean initialization
        m_finished = false;
        m_displayTime = 0.0f;
        m_fadeTimer = 0.0f;
        m_fadingOut = false;
        m_uiInitialized = false;
        m_landscapeDetectedTime = 0.0f;
        m_hasSeenPortrait = false;

        // Ensure we start with clean entity IDs
        m_backgroundEntity = 0;
        m_textEntity = 0;
        m_arrowsEntity = 0;

        // Log initial orientation state for debugging
        auto& configManager = ConfigManager::Instance();
        const ScreenInfo& initialScreenInfo = configManager.GetCurrentScreenInfo();
        GN_LOG_INFO("ScreenPromptState: Initial screen info - %.0f x %.0f pixels, logical: %.0f x %.0f, isPortrait: %s, device: %s",
                   initialScreenInfo.pixelWidth, initialScreenInfo.pixelHeight,
                   initialScreenInfo.logicalWidth, initialScreenInfo.logicalHeight,
                   initialScreenInfo.isPortrait ? "true" : "false",
                   initialScreenInfo.deviceModel.c_str());

        std::string targetOrientation = m_waitForLandscape ? "LANDSCAPE" : "PORTRAIT";
        std::string currentOrientation = initialScreenInfo.isPortrait ? "PORTRAIT" : "LANDSCAPE";

        GN_LOG_INFO("ScreenPromptState: Waiting for " + targetOrientation + " orientation (currently " + currentOrientation + ")");

        if (m_waitForLandscape) {
            // Waiting for landscape - should start in portrait
            if (!initialScreenInfo.isPortrait) {
                GN_LOG_WARN("⚠️  ScreenPromptState: Device already in LANDSCAPE mode but waiting for landscape!");
                GN_LOG_WARN("   Forcing landscape mode requirement - will wait for actual rotation gesture");
                // Reset detection to require actual rotation
                m_landscapeDetectedTime = 0.0f;
            } else {
                GN_LOG_INFO("✅ ScreenPromptState: Device correctly in PORTRAIT mode - will wait for landscape rotation");
            }
        } else {
            // Waiting for portrait - should start in landscape
            if (initialScreenInfo.isPortrait) {
                GN_LOG_WARN("⚠️  ScreenPromptState: Device already in PORTRAIT mode but waiting for portrait!");
                GN_LOG_WARN("   Forcing portrait mode requirement - will wait for actual rotation gesture");
            } else {
                GN_LOG_INFO("✅ ScreenPromptState: Device correctly in LANDSCAPE mode - will wait for portrait rotation");
            }
        }

        // Note: ScreenPromptState uses polling approach for orientation detection
        // (callback system is used by GameplayState for ongoing UI repositioning)

        GN_LOG_INFO("ScreenPromptState: Entered, will initialize UI on first update");
    }

    void ScreenPromptState::Exit() {
        GN_LOG_INFO("Exiting Screen Prompt State");
        DestroyPromptUI();
    }

    void ScreenPromptState::Pause() {
        // No-op for prompt state
    }

    void ScreenPromptState::Resume() {
        // No-op for prompt state
    }

    void ScreenPromptState::Update(float deltaTime) {
        // Initialize UI on first update to ensure systems are ready
        if (!m_uiInitialized) {
            GN_LOG_INFO("ScreenPromptState: Initializing UI on first update");
            CreatePromptUI();
            m_uiInitialized = true;
            return; // Skip other updates on first frame
        }

        m_displayTime += deltaTime;

        // Check orientation every frame (polling approach works well for ScreenPromptState)
        CheckOrientation();

        // Force additional orientation check every 0.1 seconds (simulator workaround)
        static float lastForcedCheck = 0.0f;
        if (m_displayTime - lastForcedCheck >= 0.1f) {
            lastForcedCheck = m_displayTime;
            ForceOrientationCheck();
        }

        // Handle fade out (only when landscape is confirmed)
        if (m_fadingOut) {
            m_fadeTimer += deltaTime;
            if (m_fadeTimer >= 1.0f) { // 1 second fade out
                m_finished = true;
            }
        }
    }

    void ScreenPromptState::Render() {
        // The UI elements handle their own rendering
    }

    void ScreenPromptState::HandleInput() {
        // No touch input handling - we wait for actual device rotation
        // The orientation check happens in Update() via CheckOrientation()
    }

    void ScreenPromptState::CreatePromptUI() {
        // Get current screen dimensions from ConfigManager (single source of truth)
        auto& configManager = ConfigManager::Instance();
        const ScreenInfo& screenInfo = configManager.GetCurrentScreenInfo();

        GN_LOG_INFO("ScreenPromptState: Got screen info from ConfigManager: %.0f x %.0f pixels, portrait: %s",
                   screenInfo.pixelWidth, screenInfo.pixelHeight,
                   screenInfo.isPortrait ? "true" : "false");

        GN_LOG_INFO("ScreenPromptState: Creating UI with screen dimensions %.0f x %.0f",
                   screenInfo.pixelWidth, screenInfo.pixelHeight);

        // Calculate center coordinates using pixel dimensions for consistency
        float centerX = screenInfo.pixelWidth / 2.0f;
        float centerY = screenInfo.pixelHeight / 2.0f;

        // Update cached screen info for orientation change detection
        m_currentOrientation = screenInfo.isPortrait;
        m_currentScreenWidth = screenInfo.pixelWidth;
        m_currentScreenHeight = screenInfo.pixelHeight;

        GN_LOG_INFO("ScreenPromptState: Calculated center position: (" + std::to_string(centerX) + ", " + std::to_string(centerY) + ")");

        // Create black background covering the full screen
        if (!m_ecsSystem) {
            GN_LOG_ERROR("ScreenPromptState: ECS system is null!");
            return;
        }

        // Skip background - using Metal clear color instead
        m_backgroundEntity = 0;

        // Create text entity for the rotation prompt
        Gnosis::Entity textEntity = m_ecsSystem->CreateEntity();
        if (textEntity == 0) {
            GN_LOG_ERROR("ScreenPromptState: Failed to create text entity!");
            return;
        }

        // Create UIElement component with rotation instruction (text-only, like MainMenuState)
        UIElement promptText("Please rotate your device\n90 degrees to continue", "", "");
        promptText.fontSize = 48.0f;
        promptText.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        promptText.centerTextHorizontally = true;
        promptText.centerTextVertically = true;
        promptText.visible = true;
        promptText.textLayer = 11; // Above arrows so text appears in the center

        // Position text at exact center of screen (text handles its own centering via UIElement properties)
        Transform textTransform(Gnosis::GNVector2(centerX, centerY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(textEntity, textTransform);

        // Text-only UIElement - no Sprite component needed (like MainMenuState)
        m_ecsSystem->AddComponent<UIElement>(textEntity, promptText);

        // Store the text entity for cleanup
        m_textEntity = textEntity;
        
        GN_LOG_INFO("ScreenPromptState: Text positioned at (" + std::to_string(centerX) + ", " + std::to_string(centerY) + ")");

        // Create rotating arrows sprite at the exact same center point
        Gnosis::Entity arrowsEntity = m_ecsSystem->CreateEntity();
        if (arrowsEntity == 0) {
            GN_LOG_ERROR("ScreenPromptState: Failed to create arrows entity!");
            return;
        }

        // Calculate scaled dimensions for proper centering (64x64 texture * 12x scale = 768x768)
        float arrowsScale = 12.0f;
        float arrowsTextureWidth = 64.0f;
        float arrowsTextureHeight = 64.0f;
        float arrowsScaledWidth = arrowsTextureWidth * arrowsScale;  // 768
        float arrowsScaledHeight = arrowsTextureHeight * arrowsScale; // 768

        // Use proper centering utility function for sprites (top-left positioning)
        Gnosis::GNVector2 arrowsPosition = GameCore::CenterObjectAtPosition(centerX, centerY, arrowsScaledWidth, arrowsScaledHeight);
        Transform arrowsTransform(arrowsPosition, 0.0f, Gnosis::GNVector2(arrowsScale, arrowsScale));
        m_ecsSystem->AddComponent<Transform>(arrowsEntity, arrowsTransform);

        Sprite arrowsSprite("arrowsturning", arrowsTextureWidth, arrowsTextureHeight);
        arrowsSprite.layer = 10; // Below text so text appears on top
        arrowsSprite.visible = true;
        arrowsSprite.color = Gnosis::GNColor(1.0f, 1.0f, 1.0f, 1.0f); // White
        m_ecsSystem->AddComponent<Sprite>(arrowsEntity, arrowsSprite);

        // Store the arrows entity for cleanup
        m_arrowsEntity = arrowsEntity;
        
        GN_LOG_INFO("ScreenPromptState: Arrows positioned at (" + std::to_string(arrowsPosition.x) + ", " + std::to_string(arrowsPosition.y) + ") with " + std::to_string(static_cast<int>(arrowsScale)) + "x scale (centered at " + std::to_string(centerX) + ", " + std::to_string(centerY) + ")");

        GN_LOG_INFO("ScreenPromptState: UI created successfully with entities " + 
                   std::to_string(static_cast<unsigned int>(textEntity)) + " (text) and " + 
                   std::to_string(static_cast<unsigned int>(arrowsEntity)) + " (arrows)");
    }

    void ScreenPromptState::DestroyPromptUI() {
        // Properly destroy the entities we created
        if (m_textEntity != 0) {
            GN_LOG_INFO("ScreenPromptState: Destroying text entity %u", static_cast<unsigned int>(m_textEntity));
            m_ecsSystem->DestroyEntity(m_textEntity);
            m_textEntity = 0;
        }

        if (m_arrowsEntity != 0) {
            GN_LOG_INFO("ScreenPromptState: Destroying arrows entity %u", static_cast<unsigned int>(m_arrowsEntity));
            m_ecsSystem->DestroyEntity(m_arrowsEntity);
            m_arrowsEntity = 0;
        }

        // Background entity is not created (using Metal clear color)
        m_backgroundEntity = 0;
    }

    void ScreenPromptState::CheckOrientation() {
        // Get screen info from ConfigManager (single source of truth)
        auto& configManager = ConfigManager::Instance();
        const ScreenInfo& screenInfo = configManager.GetCurrentScreenInfo();

        // Check if orientation changed and reposition UI elements
        bool orientationChanged = (screenInfo.isPortrait != m_currentOrientation);
        bool dimensionsChanged = (screenInfo.pixelWidth != m_currentScreenWidth ||
                                 screenInfo.pixelHeight != m_currentScreenHeight);

        if (orientationChanged || dimensionsChanged) {
            GN_LOG_INFO("🔄 ScreenPromptState: Orientation/Dimensions changed - repositioning UI");
            RepositionUIElements(screenInfo);

            // Update cached values
            m_currentOrientation = screenInfo.isPortrait;
            m_currentScreenWidth = screenInfo.pixelWidth;
            m_currentScreenHeight = screenInfo.pixelHeight;
        }

        // Log current screen info for debugging (but not every frame to reduce log spam)
        static float lastLogTime = -1.0f;
        static bool lastPortraitState = true; // Track state changes
        static float lastPixelWidth = 0.0f;
        static float lastPixelHeight = 0.0f;
        bool stateChanged = (screenInfo.isPortrait != lastPortraitState);
        bool logDimensionsChanged = (screenInfo.pixelWidth != lastPixelWidth || screenInfo.pixelHeight != lastPixelHeight);

        if (lastLogTime == -1.0f || (m_displayTime - lastLogTime) >= 0.5f || stateChanged || logDimensionsChanged) {
            std::string orientationMsg = std::string("ScreenPromptState - Orientation: ") +
                                       std::to_string(static_cast<int>(screenInfo.pixelWidth)) + " x " +
                                       std::to_string(static_cast<int>(screenInfo.pixelHeight)) + " pixels, " +
                                       "isPortrait: " + (screenInfo.isPortrait ? "true" : "false");

            if (stateChanged) orientationMsg += " [ORIENTATION CHANGED!]";
            if (logDimensionsChanged) orientationMsg += " [DIMENSIONS CHANGED!]";

            GN_LOG_INFO(orientationMsg.c_str());
            lastLogTime = m_displayTime;
            lastPortraitState = screenInfo.isPortrait;
            lastPixelWidth = screenInfo.pixelWidth;
            lastPixelHeight = screenInfo.pixelHeight;
        }

        if (m_waitForLandscape) {
            // Waiting for landscape rotation (entering boss level)
            if (screenInfo.isPortrait) {
                // Device is in portrait mode
                m_hasSeenPortrait = true;

                // Reset landscape detection if we were in landscape
                if (m_landscapeDetectedTime > 0.0f) {
                    GN_LOG_INFO("🔄 Device rotated back to PORTRAIT - resetting landscape detection");
                    m_landscapeDetectedTime = 0.0f;
                }

                // Only log occasionally to reduce spam
                static float lastPortraitLog = -1.0f;
                if (lastPortraitLog == -1.0f || (m_displayTime - lastPortraitLog) >= 1.0f) {
                    std::string portraitMsg = std::string("📱 PORTRAIT MODE - Waiting for landscape rotation (") +
                                            std::to_string(m_displayTime) + "s elapsed)";
                    GN_LOG_INFO(portraitMsg.c_str());
                    lastPortraitLog = m_displayTime;
                }
            } else {
                // Device is in landscape mode
                // Only proceed if we've seen portrait first and enough time has passed for stability
                if (m_hasSeenPortrait && m_displayTime > 0.5f) {
                    if (m_landscapeDetectedTime == 0.0f) {
                        // First time detecting landscape - start the timer
                        m_landscapeDetectedTime = m_displayTime;
                        GN_LOG_INFO("🎯 LANDSCAPE DETECTED! Starting 2-second confirmation countdown");
                        std::string dimsMsg = std::string("   📐 Screen dimensions: ") +
                                            std::to_string(static_cast<int>(screenInfo.pixelWidth)) + " x " +
                                            std::to_string(static_cast<int>(screenInfo.pixelHeight)) + " pixels";
                        GN_LOG_INFO(dimsMsg.c_str());
                        std::string timerMsg = std::string("   ⏱️  Timer started at: ") +
                                             std::to_string(m_displayTime) + " seconds";
                        GN_LOG_INFO(timerMsg.c_str());
                    } else {
                        // Check if 2 seconds have passed since landscape was first detected
                        float timeSinceLandscapeDetected = m_displayTime - m_landscapeDetectedTime;
                        if (timeSinceLandscapeDetected >= 2.0f) {
                            std::string confirmMsg = std::string("🎮 LANDSCAPE CONFIRMED! Starting boss level (") +
                                                   std::to_string(m_displayTime) + " seconds total)";
                            GN_LOG_INFO(confirmMsg.c_str());
                            m_finished = true; // This will trigger state transition in FloppyTurdGame
                        } else {
                            // Log progress occasionally
                            static float lastProgressLog = -1.0f;
                            if (lastProgressLog == -1.0f || (m_displayTime - lastProgressLog) >= 0.5f) {
                                std::string progressMsg = std::string("⏳ LANDSCAPE DETECTED - Confirming for ") +
                                                        std::to_string(2.0f - timeSinceLandscapeDetected) + " more seconds (" +
                                                        std::to_string(timeSinceLandscapeDetected) + "/2.0)";
                                GN_LOG_INFO(progressMsg.c_str());
                                lastProgressLog = m_displayTime;
                            }
                        }
                    }
                } else {
                    // Log why we're not proceeding with landscape detection
                    static float lastIgnoreLog = -1.0f;
                    if (lastIgnoreLog == -1.0f || (m_displayTime - lastIgnoreLog) >= 1.0f) {
                        if (!m_hasSeenPortrait) {
                            GN_LOG_INFO("🚫 LANDSCAPE DETECTED BUT IGNORING - Need to see portrait mode first");
                        } else {
                            std::string earlyMsg = std::string("🚫 LANDSCAPE DETECTED BUT IGNORING - Too early (") +
                                                  std::to_string(m_displayTime) + "s < 0.5s)";
                            GN_LOG_INFO(earlyMsg.c_str());
                        }
                        lastIgnoreLog = m_displayTime;
                    }
                }
            }
        } else {
            // Waiting for portrait rotation (exiting boss level)
            if (!screenInfo.isPortrait) {
                // Device is in landscape mode
                // Reset portrait detection if we were in portrait
                if (m_landscapeDetectedTime > 0.0f) {
                    GN_LOG_INFO("🔄 Device rotated back to LANDSCAPE - resetting portrait detection");
                    m_landscapeDetectedTime = 0.0f;
                }

                // Only log occasionally to reduce spam
                static float lastLandscapeLog = -1.0f;
                if (lastLandscapeLog == -1.0f || (m_displayTime - lastLandscapeLog) >= 1.0f) {
                    std::string landscapeMsg = std::string("📱 LANDSCAPE MODE - Waiting for portrait rotation (") +
                                             std::to_string(m_displayTime) + "s elapsed)";
                    GN_LOG_INFO(landscapeMsg.c_str());
                    lastLandscapeLog = m_displayTime;
                }
            } else {
                // Device is in portrait mode
                // Only proceed if we've seen landscape first and enough time has passed for stability
                if (m_displayTime > 0.5f) {
                    if (m_landscapeDetectedTime == 0.0f) {
                        // First time detecting portrait - start the timer (reuse landscape timer variable)
                        m_landscapeDetectedTime = m_displayTime;
                        GN_LOG_INFO("🎯 PORTRAIT DETECTED! Starting quick transition");
                        std::string dimsMsg = std::string("   📐 Screen dimensions: ") +
                                            std::to_string(static_cast<int>(screenInfo.pixelWidth)) + " x " +
                                            std::to_string(static_cast<int>(screenInfo.pixelHeight)) + " pixels";
                        GN_LOG_INFO(dimsMsg.c_str());
                        std::string timerMsg = std::string("   ⏱️  Timer started at: ") +
                                             std::to_string(m_displayTime) + " seconds";
                        GN_LOG_INFO(timerMsg.c_str());
                    } else {
                        // Quick transition - only wait 0.5 seconds for portrait
                        float timeSincePortraitDetected = m_displayTime - m_landscapeDetectedTime;
                        if (timeSincePortraitDetected >= 0.5f) {
                            std::string confirmMsg = std::string("🎮 PORTRAIT CONFIRMED! Returning to main menu (") +
                                                   std::to_string(m_displayTime) + " seconds total)";
                            GN_LOG_INFO(confirmMsg.c_str());

                            // Lock orientation to portrait before finishing
                            if (m_platformDelegates && m_platformDelegates->renderer.lockToPortrait) {
                                GN_LOG_INFO("ScreenPromptState: Locking orientation to portrait");
                                m_platformDelegates->renderer.lockToPortrait();
                            }

                            m_finished = true; // This will trigger state transition in FloppyTurdGame
                        } else {
                            // Log progress occasionally
                            static float lastProgressLog = -1.0f;
                            if (lastProgressLog == -1.0f || (m_displayTime - lastProgressLog) >= 0.2f) {
                                std::string progressMsg = std::string("⏳ PORTRAIT DETECTED - Confirming for ") +
                                                        std::to_string(0.5f - timeSincePortraitDetected) + " more seconds (" +
                                                        std::to_string(timeSincePortraitDetected) + "/0.5)";
                                GN_LOG_INFO(progressMsg.c_str());
                                lastProgressLog = m_displayTime;
                            }
                        }
                    }
                }
            }
        }
    }

    void ScreenPromptState::ForceOrientationCheck() {
        // Force a screen info update and check for orientation changes
        // This is a workaround for iOS Simulator not triggering viewWillTransition properly

        ScreenInfo screenInfo;
        if (m_ecsSystem && m_ecsSystem->GetSystemManager() && m_ecsSystem->GetSystemManager()->GetRenderSystem()) {
            auto* renderSystem = m_ecsSystem->GetSystemManager()->GetRenderSystem();
            screenInfo = renderSystem->GetScreenInfo();
        } else {
            // Fallback to ConfigManager
            auto& configManager = ConfigManager::Instance();
            screenInfo = configManager.GetCurrentScreenInfo();
        }

        // Check if orientation changed
        bool orientationChanged = (screenInfo.isPortrait != m_currentOrientation);
        bool dimensionsChanged = (screenInfo.pixelWidth != m_currentScreenWidth ||
                                 screenInfo.pixelHeight != m_currentScreenHeight);

        if (orientationChanged || dimensionsChanged) {
            GN_LOG_INFO("🎯 FORCE CHECK: Orientation/Dimensions changed - isPortrait: " +
                       std::string(screenInfo.isPortrait ? "true" : "false") +
                       ", dimensions: " + std::to_string(static_cast<int>(screenInfo.pixelWidth)) + "x" +
                       std::to_string(static_cast<int>(screenInfo.pixelHeight)));

            // Update cached values
            m_currentOrientation = screenInfo.isPortrait;
            m_currentScreenWidth = screenInfo.pixelWidth;
            m_currentScreenHeight = screenInfo.pixelHeight;

            // Reposition UI elements
            RepositionUIElements(screenInfo);

            // Force RenderSystem to update its screen info to stay in sync
            if (m_ecsSystem && m_ecsSystem->GetSystemManager() && m_ecsSystem->GetSystemManager()->GetRenderSystem()) {
                auto* renderSystem = m_ecsSystem->GetSystemManager()->GetRenderSystem();
                renderSystem->UpdateScreenInfo();
            }
        }
    }

    void ScreenPromptState::RepositionUIElements(const ScreenInfo& screenInfo) {
        if (!m_ecsSystem || !m_textEntity || !m_arrowsEntity) {
            GN_LOG_WARN("ScreenPromptState: Cannot reposition UI - missing entities or ECS system");
            return;
        }

        // Calculate new center coordinates
        float centerX = screenInfo.pixelWidth / 2.0f;
        float centerY = screenInfo.pixelHeight / 2.0f;

        // Reposition text entity
        if (m_textEntity != 0) {
            Transform* textTransform = m_ecsSystem->GetComponent<Transform>(m_textEntity);
            if (textTransform) {
                textTransform->position = Gnosis::GNVector2(centerX, centerY);
                std::string textPosMsg = std::string("🔄 Repositioned text to center: (") +
                                       std::to_string(centerX) + ", " + std::to_string(centerY) + ")";
                GN_LOG_INFO(textPosMsg.c_str());
            }
        }

        // Reposition arrows entity
        if (m_arrowsEntity != 0) {
            // Calculate scaled dimensions for proper centering
            float arrowsScale = 10.0f;
            float arrowsTextureWidth = 64.0f;
            float arrowsTextureHeight = 64.0f;
            float arrowsScaledWidth = arrowsTextureWidth * arrowsScale;
            float arrowsScaledHeight = arrowsTextureHeight * arrowsScale;

            // Use proper centering utility function for sprites
            Gnosis::GNVector2 arrowsPosition = GameCore::CenterObjectAtPosition(centerX, centerY, arrowsScaledWidth, arrowsScaledHeight);

            Transform* arrowsTransform = m_ecsSystem->GetComponent<Transform>(m_arrowsEntity);
            if (arrowsTransform) {
                arrowsTransform->position = arrowsPosition;
                std::string arrowsPosMsg = std::string("🔄 Repositioned arrows to: (") +
                                         std::to_string(arrowsPosition.x) + ", " + std::to_string(arrowsPosition.y) + ")" +
                                         " (center at " + std::to_string(centerX) + ", " + std::to_string(centerY) + ")";
                GN_LOG_INFO(arrowsPosMsg.c_str());
            }
        }

        GN_LOG_INFO("✅ ScreenPromptState: UI repositioning complete");
    }

} // namespace GameCore
