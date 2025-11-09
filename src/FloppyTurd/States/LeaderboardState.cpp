#include "LeaderboardState.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/Utility/Utils.h"
#include "../Input/InputManager.h"
#include <sstream>
#include <iomanip>

namespace GameCore {

    // Static instance for callbacks
    LeaderboardState* LeaderboardState::s_instance = nullptr;

    LeaderboardState::LeaderboardState(ECS* ecsSystem, PlatformDelegates* platformDelegates)
        : m_ecsSystem(ecsSystem)
        , m_platformDelegates(platformDelegates)
        , m_game(nullptr)
        , m_finished(false)
        , m_initialized(false)
        , m_currentPage(LeaderboardPage::LEVEL_1_PARK)
        , m_screenWidth(0.0f)
        , m_screenHeight(0.0f)
        , m_uiScale(1.0f)
        , m_overlayX(0.0f)
        , m_overlayY(0.0f)
        , m_overlayWidth(0.0f)
        , m_overlayHeight(0.0f)
        , m_backgroundEntity(0)
        , m_overlayBackgroundEntity(0)
        , m_titleEntity(0)
        , m_backButtonEntity(0)
        , m_leftArrowEntity(0)
        , m_rightArrowEntity(0)
        , m_pageTitleEntity(0)
        , m_localScoreEntity(0)
        , m_lastArrowPressTime(0.0f)
        , m_lastButtonPressTime(0.0f)
    {
        m_game = GetGame();
        s_instance = this;  // Set static instance for callbacks
        GN_LOG_INFO("LeaderboardState created");
    }

    LeaderboardState::~LeaderboardState() {
        s_instance = nullptr;  // Clear static instance
        GN_LOG_INFO("LeaderboardState destroyed");
    }

    void LeaderboardState::Enter() {
        GN_LOG_INFO("Entering Leaderboard State");
        m_finished = false;

        // Get enhanced screen info using pixel dimensions
        ScreenInfo screenInfo;
        if (m_platformDelegates && m_platformDelegates->renderer.getScreenInfo) {
            m_platformDelegates->renderer.getScreenInfo(&screenInfo);
            m_screenWidth = screenInfo.pixelWidth;
            m_screenHeight = screenInfo.pixelHeight;
            
            // VALIDATION: Check for invalid/uninitialized screen dimensions
            if (m_screenWidth <= 0.0f || m_screenHeight <= 0.0f) {
                GN_LOG_ERROR("❌ LeaderboardState: Invalid screen dimensions from delegate: " + 
                           std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
                GN_LOG_ERROR("Screen info likely not properly initialized - forcing iPhone 16 fallback");
                m_screenWidth = 1179.0f;
                m_screenHeight = 2556.0f;
            }
            
            GN_LOG_INFO("LeaderboardState: Screen info - pixel: " + 
                       std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + 
                       ", logical: " + std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight) +
                       ", scale: " + std::to_string(screenInfo.scaleFactor));
        } else if (m_platformDelegates && m_platformDelegates->renderer.getScreenSize) {
            m_platformDelegates->renderer.getScreenSize(&m_screenWidth, &m_screenHeight);
            
            // VALIDATION: Check for invalid dimensions
            if (m_screenWidth <= 0.0f || m_screenHeight <= 0.0f) {
                GN_LOG_ERROR("❌ LeaderboardState: Invalid screen size from legacy delegate: " + 
                           std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
                m_screenWidth = 1179.0f;
                m_screenHeight = 2556.0f;
            }
            
            GN_LOG_INFO("LeaderboardState: Screen size (fallback) - " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
        } else {
            GN_LOG_WARN("⚠️ LeaderboardState: No screen info delegates available - using iPhone 16 defaults");
            m_screenWidth = 1179.0f;
            m_screenHeight = 2556.0f;
        }

        // Set UI scale based on platform
        m_uiScale = IsMobilePlatform() ? 8.0f : 1.0f;

        CreateUI();
        m_initialized = true;

        // Load Game Center leaderboard data for current page
        LoadLeaderboardData();

        // NOTE: We do NOT start music here - let it continue from main menu
        GN_LOG_INFO("LeaderboardState entered - Screen: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
    }

    void LeaderboardState::Exit() {
        GN_LOG_INFO("🚪 ========== EXITING LEADERBOARD STATE ==========");
        GN_LOG_INFO("🧹 Starting entity cleanup...");

        // Clean up all entities
        DestroyPageContent();

        if (m_backgroundEntity != 0) {
            GN_LOG_INFO("🧹 Destroying background entity: " + std::to_string(m_backgroundEntity));
            m_ecsSystem->DestroyEntity(m_backgroundEntity);
            m_backgroundEntity = 0;
        }
        if (m_overlayBackgroundEntity != 0) {
            GN_LOG_INFO("🧹 Destroying overlay background entity: " + std::to_string(m_overlayBackgroundEntity));
            m_ecsSystem->DestroyEntity(m_overlayBackgroundEntity);
            m_overlayBackgroundEntity = 0;
        }
        // Title entity is now destroyed in DestroyPageContent()
        if (m_backButtonEntity != 0) {
            GN_LOG_INFO("🧹 Destroying back button entity: " + std::to_string(m_backButtonEntity));
            m_ecsSystem->DestroyEntity(m_backButtonEntity);
            m_backButtonEntity = 0;
        }
        if (m_leftArrowEntity != 0) {
            GN_LOG_INFO("🧹 Destroying LEFT arrow entity: " + std::to_string(m_leftArrowEntity));
            m_ecsSystem->DestroyEntity(m_leftArrowEntity);
            m_leftArrowEntity = 0;
        }
        if (m_rightArrowEntity != 0) {
            GN_LOG_INFO("🧹 Destroying RIGHT arrow entity: " + std::to_string(m_rightArrowEntity));
            m_ecsSystem->DestroyEntity(m_rightArrowEntity);
            m_rightArrowEntity = 0;
        }

        GN_LOG_INFO("✅ All leaderboard entities destroyed");
        // NOTE: We do NOT stop music here - let it continue playing in main menu
        m_initialized = false;
        GN_LOG_INFO("🚪 ========== LEADERBOARD EXIT COMPLETE ==========");
    }

    void LeaderboardState::Pause() {
        GN_LOG_INFO("Leaderboard State paused");
    }

    void LeaderboardState::Resume() {
        GN_LOG_INFO("Leaderboard State resumed");
    }

    void LeaderboardState::Update(float deltaTime) {
        // Update InputManager (CRITICAL - without this, input won't work!)
        InputManager* inputManager = InputManager::GetInstance();
        if (inputManager) {
            inputManager->Update(deltaTime);
        }
        
        // Update debounce timers
        if (m_lastArrowPressTime > 0.0f) {
            m_lastArrowPressTime -= deltaTime;
            if (m_lastArrowPressTime < 0.0f) {
                m_lastArrowPressTime = 0.0f;
            }
        }
        if (m_lastButtonPressTime > 0.0f) {
            m_lastButtonPressTime -= deltaTime;
            if (m_lastButtonPressTime < 0.0f) {
                m_lastButtonPressTime = 0.0f;
            }
        }
    }

    void LeaderboardState::Render() {
        // Rendering is handled by the ECS render system
    }

    void LeaderboardState::HandleInput() {
        if (!m_ecsSystem) {
            return;
        }

        // Use InputManager singleton (same pattern as MainMenuState)
        InputManager* inputManager = InputManager::GetInstance();
        if (!inputManager) {
            GN_LOG_ERROR("🎮 LeaderboardState: InputManager singleton is NULL!");
            return;
        }

        GN_LOG_INFO("🎮 LeaderboardState: HandleInput() called - frame " + std::to_string(inputManager->GetCurrentFrameNumber()));

        // Get active touches from InputManager
        auto touches = inputManager->GetActiveTouches();
        GN_LOG_INFO("🎮 LeaderboardState: Processing " + std::to_string(touches.size()) + " touches");

        for (const auto& touch : touches) {
            GN_LOG_INFO("🎯 LeaderboardState: Touch " + std::to_string(touch.touchId) + 
                        " state=" + std::to_string((int)touch.state) + 
                        " pixel(" + std::to_string(touch.rawX) + ", " + std::to_string(touch.rawY) + ")");

            // Only process RELEASED touches to avoid double-triggering
            if (touch.state == TouchState::RELEASED) {
                // TouchData.rawX/rawY are already in pixel coordinates
                float pixelX = touch.rawX;
                float pixelY = touch.rawY;

                GN_LOG_INFO("🎮 LeaderboardState: Processing RELEASED touch at pixel(" +
                           std::to_string(pixelX) + ", " + std::to_string(pixelY) + ")");

                // Check back button (top-left based positioning)
                if (m_backButtonEntity != 0) {
                    auto* transform = m_ecsSystem->GetComponent<Transform>(m_backButtonEntity);
                    auto* sprite = m_ecsSystem->GetComponent<Sprite>(m_backButtonEntity);
                    
                    if (transform && sprite) {
                        float btnX = transform->position.x;
                        float btnY = transform->position.y;
                        float btnW = sprite->width * transform->scale.x;
                        float btnH = sprite->height * transform->scale.y;

                        GN_LOG_INFO("Back button bounds: x=" + std::to_string(btnX) + " to " + std::to_string(btnX + btnW) + 
                                   ", y=" + std::to_string(btnY) + " to " + std::to_string(btnY + btnH));

                        if (pixelX >= btnX && pixelX <= btnX + btnW &&
                            pixelY >= btnY && pixelY <= btnY + btnH) {
                            GN_LOG_INFO("✅ Back button clicked!");
                            OnBackButtonPressed();
                            return;
                        }
                    }
                }

                // Check left arrow
                if (m_leftArrowEntity != 0) {
                    auto* transform = m_ecsSystem->GetComponent<Transform>(m_leftArrowEntity);
                    auto* sprite = m_ecsSystem->GetComponent<Sprite>(m_leftArrowEntity);
                    
                    if (transform && sprite) {
                        float btnX = transform->position.x;
                        float btnY = transform->position.y;
                        float btnW = sprite->width * transform->scale.x;
                        float btnH = sprite->height * transform->scale.y;

                        GN_LOG_INFO("Left arrow bounds: x=" + std::to_string(btnX) + " to " + std::to_string(btnX + btnW) + 
                                   ", y=" + std::to_string(btnY) + " to " + std::to_string(btnY + btnH));

                        if (pixelX >= btnX && pixelX <= btnX + btnW &&
                            pixelY >= btnY && pixelY <= btnY + btnH) {
                            GN_LOG_INFO("✅ Left arrow clicked!");
                            OnLeftArrowPressed();
                            return;
                        }
                    }
                }

                // Check right arrow
                if (m_rightArrowEntity != 0) {
                    auto* transform = m_ecsSystem->GetComponent<Transform>(m_rightArrowEntity);
                    auto* sprite = m_ecsSystem->GetComponent<Sprite>(m_rightArrowEntity);
                    
                    if (transform && sprite) {
                        float btnX = transform->position.x;
                        float btnY = transform->position.y;
                        float btnW = sprite->width * transform->scale.x;
                        float btnH = sprite->height * transform->scale.y;

                        GN_LOG_INFO("➡️ Right arrow ACTUAL POSITION CHECK:");
                        GN_LOG_INFO("   Transform position: (" + std::to_string(btnX) + ", " + std::to_string(btnY) + ")");
                        GN_LOG_INFO("   Sprite size: " + std::to_string(sprite->width) + "x" + std::to_string(sprite->height));
                        GN_LOG_INFO("   Transform scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y));
                        GN_LOG_INFO("   Scaled size: " + std::to_string(btnW) + "x" + std::to_string(btnH));
                        GN_LOG_INFO("   Right edge: " + std::to_string(btnX + btnW) + " (screen width: " + std::to_string(m_screenWidth) + ")");
                        GN_LOG_INFO("   Distance from screen right edge: " + std::to_string(m_screenWidth - (btnX + btnW)) + " pixels");
                        GN_LOG_INFO("   Bounds: x=" + std::to_string(btnX) + " to " + std::to_string(btnX + btnW) + 
                                   ", y=" + std::to_string(btnY) + " to " + std::to_string(btnY + btnH));

                        if (pixelX >= btnX && pixelX <= btnX + btnW &&
                            pixelY >= btnY && pixelY <= btnY + btnH) {
                            GN_LOG_INFO("✅ Right arrow clicked!");
                            OnRightArrowPressed();
                            return;
                        }
                    }
                }
            }
        }
    }

    void LeaderboardState::CreateUI() {
        CreateBackground();
        CreateNavigationButtons();
        CreatePageContent();
    }

    void LeaderboardState::CreateBackground() {
        GN_LOG_INFO("LeaderboardState: Creating backgrounds");

        // 1. Create full-screen main menu background (like MainMenuState - NO SCALING)
        m_backgroundEntity = m_ecsSystem->CreateEntity();
        
        // Use MainMenuMobile for mobile platforms
        std::string bgTexture = IsMobilePlatform() ? "MainMenuMobile" : "MainMenu";
        
        // Get actual texture dimensions (MainMenuMobile is 393x852)
        float bgTextureWidth = 393.0f;
        float bgTextureHeight = 852.0f;
        
        // Calculate scale to fill screen - USE PIXEL DIMENSIONS
        float bgScaleX = m_screenWidth / bgTextureWidth;
        float bgScaleY = m_screenHeight / bgTextureHeight;
        
        // Background positioned at (0,0) top-left
        Transform bgTransform(GNVector2(0.0f, 0.0f), 0.0f, GNVector2(bgScaleX, bgScaleY));
        m_ecsSystem->AddComponent<Transform>(m_backgroundEntity, bgTransform);

        Sprite bgSprite(bgTexture, bgTextureWidth, bgTextureHeight);
        bgSprite.layer = 0;
        bgSprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(m_backgroundEntity, bgSprite);
        
        GN_LOG_INFO("LeaderboardState: Created full-screen background: " + bgTexture + 
                   " at (0,0) with scale (" + std::to_string(bgScaleX) + "x" + std::to_string(bgScaleY) + ")");

        // 2. Create centered pause menu overlay background (EXACT pattern from PauseSystem)
        // PauseMenuBackgroundMobile is 160x300 and uses 7.0f scale
        m_overlayBackgroundEntity = m_ecsSystem->CreateEntity();
        
        float overlayTextureWidth = 160.0f;
        float overlayTextureHeight = 300.0f;
        float overlayScale = 7.0f;
        
        // SCALE FIRST, then center: Calculate final rendered dimensions
        float scaledWidth = overlayTextureWidth * overlayScale;   // 160 * 7 = 1120
        float scaledHeight = overlayTextureHeight * overlayScale; // 300 * 7 = 2100
        
        // Center on screen using CenterObjectAtPosition helper
        float centerX = m_screenWidth * 0.5f;
        float centerY = m_screenHeight * 0.5f;
        GNVector2 overlayPosition = CenterObjectAtPosition(centerX, centerY, scaledWidth, scaledHeight);
        
        Transform overlayTransform(overlayPosition, 0.0f, GNVector2(overlayScale, overlayScale));
        m_ecsSystem->AddComponent<Transform>(m_overlayBackgroundEntity, overlayTransform);

        Sprite overlaySprite("PauseMenuBackgroundMobile", (int)overlayTextureWidth, (int)overlayTextureHeight);
        overlaySprite.layer = 5;
        overlaySprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(m_overlayBackgroundEntity, overlaySprite);
        
        GN_LOG_INFO("LeaderboardState: Created overlay background at (" + std::to_string(overlayPosition.x) + ", " + std::to_string(overlayPosition.y) + 
                   ") with scale " + std::to_string(overlayScale) + " (size: " + std::to_string(scaledWidth) + "x" + std::to_string(scaledHeight) + ")");

        // Store overlay bounds for positioning UI elements within it
        m_overlayX = overlayPosition.x;
        m_overlayY = overlayPosition.y;
        m_overlayWidth = scaledWidth;
        m_overlayHeight = scaledHeight;
    }

    void LeaderboardState::CreateNavigationButtons() {
        GN_LOG_INFO("🔧 LeaderboardState: Creating navigation buttons");
        GN_LOG_INFO("📐 Screen dimensions: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));

        // Calculate screen center for positioning (SAME as MainMenuState pattern)
        float centerX = m_screenWidth * 0.5f;
        
        // Arrow button scale (larger for mobile)
        float arrowScale = IsMobilePlatform() ? 6.0f : 2.0f;
        float arrowTextureSize = 32.0f;
        float arrowScaledSize = arrowTextureSize * arrowScale;
        
        GN_LOG_INFO("🎯 Arrow scale: " + std::to_string(arrowScale) + ", texture size: " + std::to_string(arrowTextureSize) + ", scaled size: " + std::to_string(arrowScaledSize));

        // Position arrows much lower on screen (near bottom, above back button)
        float arrowCenterY = m_screenHeight * 0.85f;  // 85% from top (was 50%)
        
        // Left arrow - positioned with LEFT EDGE very close to left screen edge (match level select)
        m_leftArrowEntity = m_ecsSystem->CreateEntity();
        float leftArrowLeftEdge = m_screenWidth * 0.005f;  // 0.5% from left edge (match level select pattern)
        float leftArrowFinalX = leftArrowLeftEdge;  // Top-left positioning
        float leftArrowFinalY = arrowCenterY - (arrowScaledSize / 2.0f);  // Center vertically
        
        GN_LOG_INFO("⬅️ Left arrow: screenWidth=" + std::to_string(m_screenWidth) + 
                    ", leftEdge=" + std::to_string(leftArrowLeftEdge) + " (0.5%), arrowSize=" + 
                    std::to_string(arrowScaledSize) + ", finalPos(" + std::to_string(leftArrowFinalX) + 
                    "," + std::to_string(leftArrowFinalY) + ")");
        
        Transform leftTransform(GNVector2(leftArrowFinalX, leftArrowFinalY), 0.0f, GNVector2(arrowScale, arrowScale));
        m_ecsSystem->AddComponent<Transform>(m_leftArrowEntity, leftTransform);

        Sprite leftSprite("LeftArrow", arrowTextureSize, arrowTextureSize);
        leftSprite.layer = 15;
        leftSprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(m_leftArrowEntity, leftSprite);
        
        GN_LOG_INFO("LeaderboardState: Created left arrow at (" + std::to_string(leftArrowFinalX) + ", " + std::to_string(leftArrowFinalY) + ")");

        // Right arrow - SIMPLIFIED: just position at 90% without any edge correction
        m_rightArrowEntity = m_ecsSystem->CreateEntity();
        float rightArrowFinalX = m_screenWidth * 0.90f;  // Simply 90% from left edge - NO CORRECTION
        float rightArrowFinalY = arrowCenterY - (arrowScaledSize / 2.0f);  // Center vertically
        
        GN_LOG_INFO("➡️ ========== RIGHT ARROW SIMPLIFIED POSITIONING ==========");
        GN_LOG_INFO("   screenWidth=" + std::to_string(m_screenWidth));
        GN_LOG_INFO("   arrowScaledSize=" + std::to_string(arrowScaledSize));
        GN_LOG_INFO("   finalX (90% of screen)=" + std::to_string(rightArrowFinalX));
        GN_LOG_INFO("   finalY=" + std::to_string(rightArrowFinalY));
        GN_LOG_INFO("   Arrow will span from X=" + std::to_string(rightArrowFinalX) + " to X=" + std::to_string(rightArrowFinalX + arrowScaledSize));
        GN_LOG_INFO("   Actual right edge will be at: " + std::to_string(rightArrowFinalX + arrowScaledSize));
        GN_LOG_INFO("   Distance from screen right edge: " + std::to_string(m_screenWidth - (rightArrowFinalX + arrowScaledSize)) + " pixels");
        
        Transform rightTransform(GNVector2(rightArrowFinalX, rightArrowFinalY), 0.0f, GNVector2(arrowScale, arrowScale));
        m_ecsSystem->AddComponent<Transform>(m_rightArrowEntity, rightTransform);
        
        // VERIFY: Log what we just set
        GN_LOG_INFO("✅ RIGHT ARROW TRANSFORM SET:");
        GN_LOG_INFO("   Position: (" + std::to_string(rightTransform.position.x) + ", " + std::to_string(rightTransform.position.y) + ")");
        GN_LOG_INFO("   Scale: (" + std::to_string(rightTransform.scale.x) + ", " + std::to_string(rightTransform.scale.y) + ")");
        GN_LOG_INFO("➡️ ========================================================");

        Sprite rightSprite("RightArrow", arrowTextureSize, arrowTextureSize);
        rightSprite.layer = 15;
        rightSprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(m_rightArrowEntity, rightSprite);
        
        // VERIFY: Read back the transform to confirm it was set correctly
        if (auto* verifyTransform = m_ecsSystem->GetComponent<Transform>(m_rightArrowEntity)) {
            GN_LOG_INFO("🔍 VERIFICATION - Reading back right arrow transform from ECS:");
            GN_LOG_INFO("   Stored Position: (" + std::to_string(verifyTransform->position.x) + ", " + std::to_string(verifyTransform->position.y) + ")");
            GN_LOG_INFO("   Stored Scale: (" + std::to_string(verifyTransform->scale.x) + ", " + std::to_string(verifyTransform->scale.y) + ")");
            float verifyRightEdge = verifyTransform->position.x + (arrowTextureSize * verifyTransform->scale.x);
            GN_LOG_INFO("   Calculated right edge: " + std::to_string(verifyRightEdge) + " (screen: " + std::to_string(m_screenWidth) + ")");
            GN_LOG_INFO("   Calculated margin from edge: " + std::to_string(m_screenWidth - verifyRightEdge) + " pixels");
        } else {
            GN_LOG_ERROR("❌ FAILED to read back transform component for right arrow!");
        }
        
        GN_LOG_INFO("LeaderboardState: Created right arrow at (" + std::to_string(rightArrowFinalX) + ", " + std::to_string(rightArrowFinalY) + ")");

        // Back button - EXACT same pattern as MainMenuState level select back button
        m_backButtonEntity = m_ecsSystem->CreateEntity();
        
        float buttonScale = IsMobilePlatform() ? 10.0f : 4.0f;
        float buttonTexWidth = 90.0f;
        float buttonTexHeight = 16.0f;
        float buttonWidth = buttonTexWidth * buttonScale;
        float buttonHeight = buttonTexHeight * buttonScale;
        
        // Position near bottom like MainMenuState (0.93f)
        float buttonCenterY = m_screenHeight * 0.93f;
        GNVector2 backButtonPosition = CenterObjectAtPosition(centerX, buttonCenterY, buttonWidth, buttonHeight);
        
        Transform backTransform(backButtonPosition, 0.0f, GNVector2(buttonScale, buttonScale));
        m_ecsSystem->AddComponent<Transform>(m_backButtonEntity, backTransform);

        Sprite backSprite("FloppyButtonBlue", buttonTexWidth, buttonTexHeight);
        backSprite.layer = 15;
        backSprite.visible = true;
        m_ecsSystem->AddComponent<Sprite>(m_backButtonEntity, backSprite);
        
        UIElement backUI("BACK", "FloppyButtonBlue", "FloppyButtonBlueHover");
        backUI.fontSize = IsMobilePlatform() ? 88.0f : 21.0f;
        backUI.textColor = GNColor(255, 255, 255, 255);
        backUI.centerTextHorizontally = true;
        backUI.centerTextVertically = true;
        backUI.visible = true;
        m_ecsSystem->AddComponent<UIElement>(m_backButtonEntity, backUI);
        
        GN_LOG_INFO("LeaderboardState: Created back button at (" + std::to_string(backButtonPosition.x) + ", " + std::to_string(backButtonPosition.y) + ")");
    }

    void LeaderboardState::CreatePageContent() {
        DestroyPageContent();

        GN_LOG_INFO("LeaderboardState: Creating page content for page " + std::to_string(static_cast<int>(m_currentPage)));

        // Use screen center for text positioning (UIElement uses centerTextHorizontally)
        float centerX = m_screenWidth * 0.5f;
        
        // Title at top of screen (LEADERBOARDS label)
        m_titleEntity = m_ecsSystem->CreateEntity();
        float titleY = m_screenHeight * 0.17f;  // 17% from top (was 12%, brought down 5%)
        
        Transform titleTransform(GNVector2(centerX, titleY), 0.0f, GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_titleEntity, titleTransform);

        UIElement titleUI;
        titleUI.buttonText = "LEADERBOARDS";
        titleUI.fontSize = IsMobilePlatform() ? 72.0f : 48.0f;
        titleUI.textColor = GNColor(255, 255, 255, 255);
        titleUI.centerTextHorizontally = true;
        titleUI.visible = true;
        titleUI.textLayer = 10;
        m_ecsSystem->AddComponent<UIElement>(m_titleEntity, titleUI);

        // Page title (level name) - RIGHT UNDER the LEADERBOARDS label
        m_pageTitleEntity = m_ecsSystem->CreateEntity();
        float pageTitleY = m_screenHeight * 0.25f;  // 25% from top (close to title)
        
        Transform pageTitleTransform(GNVector2(centerX, pageTitleY), 0.0f, GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(m_pageTitleEntity, pageTitleTransform);

        UIElement pageTitleUI;
        pageTitleUI.buttonText = GetPageTitle(m_currentPage);
        pageTitleUI.fontSize = IsMobilePlatform() ? 56.0f : 36.0f;
        pageTitleUI.textColor = GNColor(255, 215, 0, 255); // Gold
        pageTitleUI.centerTextHorizontally = true;
        pageTitleUI.visible = true;
        pageTitleUI.textLayer = 12;
        m_ecsSystem->AddComponent<UIElement>(m_pageTitleEntity, pageTitleUI);

        // Create top 10 leaderboard display with classic arcade-style placeholder slots
        float startY = m_screenHeight * 0.33f;  // Start below the page title
        float lineHeight = IsMobilePlatform() ? 95.0f : 60.0f;  // EVEN MORE vertical spacing
        float fontSize = IsMobilePlatform() ? 36.0f : 24.0f;
        
        // Display columns: Rank | Name | Score
        // For now, show placeholder entries since we need to integrate with actual leaderboard data
        for (int i = 0; i < 10; i++) {
            Entity rowEntity = m_ecsSystem->CreateEntity();
            
            float rowY = startY + (i * lineHeight);
            Transform rowTransform(GNVector2(centerX, rowY), 0.0f, GNVector2(1.0f, 1.0f));
            m_ecsSystem->AddComponent<Transform>(rowEntity, rowTransform);
            
            UIElement rowUI;
            // Format: "01.          -------          -----" (even more horizontal spacing)
            std::string rankStr = (i + 1 < 10) ? ("0" + std::to_string(i + 1)) : std::to_string(i + 1);
            rowUI.buttonText = rankStr + ".          -------          -----";
            rowUI.fontSize = fontSize;
            rowUI.textColor = GNColor(200, 200, 200, 255);  // Light gray for empty slots
            rowUI.centerTextHorizontally = true;
            rowUI.visible = true;
            rowUI.textLayer = 12;
            m_ecsSystem->AddComponent<UIElement>(rowEntity, rowUI);
            
            m_contentEntities.push_back(rowEntity);
        }
        
        // Add "Your Best" label row right above the center of the arrows (85% - half arrow height)
        // This shows player's actual score/rank
        Entity playerRankEntity = m_ecsSystem->CreateEntity();
        float arrowCenterY = m_screenHeight * 0.85f;  // Arrow center position
        float arrowScaledSize = 32.0f * (IsMobilePlatform() ? 6.0f : 2.0f);
        float playerRankY = arrowCenterY - (arrowScaledSize * 0.5f) - (lineHeight * 0.5f);  // Just above arrow center
        
        Transform playerRankTransform(GNVector2(centerX, playerRankY), 0.0f, GNVector2(1.0f, 1.0f));
        m_ecsSystem->AddComponent<Transform>(playerRankEntity, playerRankTransform);
        
        UIElement playerRankUI;
        playerRankUI.buttonText = GetLocalScoreText(m_currentPage);  // This will show player's actual rank
        playerRankUI.fontSize = fontSize;
        playerRankUI.textColor = GNColor(255, 215, 0, 255);  // Gold for player's rank
        playerRankUI.centerTextHorizontally = true;
        playerRankUI.visible = true;
        playerRankUI.textLayer = 12;
        m_ecsSystem->AddComponent<UIElement>(playerRankEntity, playerRankUI);
        
        m_contentEntities.push_back(playerRankEntity);
        
        GN_LOG_INFO("LeaderboardState: Created top 10 leaderboard display starting at y=" + std::to_string(startY));
        
        // NOTE: Commenting out auto-show GameCenter to prevent crash
        // The crash occurs when showLeaderboard is called with corrupted function pointer
        // User can still manually access leaderboards from main menu
        /*
        #ifdef PLATFORM_IOS
        if (m_platformDelegates && m_platformDelegates->gameCenter.showLeaderboard) {
            std::string leaderboardID = GetLeaderboardID(m_currentPage);
            if (!leaderboardID.empty()) {
                m_platformDelegates->gameCenter.showLeaderboard(leaderboardID.c_str());
            }
        }
        #endif
        */
    }

    void LeaderboardState::DestroyPageContent() {
        // Destroy LEADERBOARDS title
        if (m_titleEntity != 0) {
            m_ecsSystem->DestroyEntity(m_titleEntity);
            m_titleEntity = 0;
        }
        // Destroy page title (level name)
        if (m_pageTitleEntity != 0) {
            m_ecsSystem->DestroyEntity(m_pageTitleEntity);
            m_pageTitleEntity = 0;
        }
        if (m_localScoreEntity != 0) {
            m_ecsSystem->DestroyEntity(m_localScoreEntity);
            m_localScoreEntity = 0;
        }

        for (auto entity : m_contentEntities) {
            m_ecsSystem->DestroyEntity(entity);
        }
        m_contentEntities.clear();
    }

    void LeaderboardState::UpdatePageContent() {
        CreatePageContent();
    }

    void LeaderboardState::OnLeftArrowPressed() {
        // Debounce to prevent spam clicking
        if (m_lastArrowPressTime > 0.0f) {
            GN_LOG_INFO("⏱️ Left arrow debounced - too soon");
            return;
        }
        m_lastArrowPressTime = BUTTON_DEBOUNCE_DELAY;
        
        int pageIndex = static_cast<int>(m_currentPage);
        pageIndex--;
        if (pageIndex < 0) {
            pageIndex = static_cast<int>(LeaderboardPage::COUNT) - 1;
        }
        m_currentPage = static_cast<LeaderboardPage>(pageIndex);

        GN_LOG_INFO("Leaderboard: Navigate to previous page " + std::to_string(pageIndex));
        UpdatePageContent();
        
        // NOTE: GameCenter auto-show disabled to prevent crash - see Enter() method
        GN_LOG_INFO("Leaderboard: Skipping GameCenter show on page change");
    }

    void LeaderboardState::OnRightArrowPressed() {
        // Debounce to prevent spam clicking
        if (m_lastArrowPressTime > 0.0f) {
            GN_LOG_INFO("⏱️ Right arrow debounced - too soon");
            return;
        }
        m_lastArrowPressTime = BUTTON_DEBOUNCE_DELAY;
        
        int pageIndex = static_cast<int>(m_currentPage);
        pageIndex++;
        if (pageIndex >= static_cast<int>(LeaderboardPage::COUNT)) {
            pageIndex = 0;
        }
        m_currentPage = static_cast<LeaderboardPage>(pageIndex);

        GN_LOG_INFO("Leaderboard: Navigate to next page " + std::to_string(pageIndex));
        UpdatePageContent();
        
        // NOTE: GameCenter auto-show disabled to prevent crash - see Enter() method
        GN_LOG_INFO("Leaderboard: Skipping GameCenter show on page change");
    }

    void LeaderboardState::OnBackButtonPressed() {
        // Debounce to prevent spam clicking
        if (m_lastButtonPressTime > 0.0f) {
            GN_LOG_INFO("⏱️ Back button debounced - too soon");
            return;
        }
        m_lastButtonPressTime = BUTTON_DEBOUNCE_DELAY;
        
        GN_LOG_INFO("Leaderboard: Back button pressed - returning to main menu");
        m_finished = true;
    }


    std::string LeaderboardState::GetPageTitle(LeaderboardPage page) const {
        switch (page) {
            case LeaderboardPage::LEVEL_1_PARK:
                return "A Flop in the Park";
            case LeaderboardPage::LEVEL_2_SEWER:
                return "Home Sweet Home";
            case LeaderboardPage::LEVEL_3_DESERT:
                return "The Good, The Bad,\nand the Stinky";
            case LeaderboardPage::LEVEL_4_SNOW:
                return "Polar Pandemonium";
            case LeaderboardPage::LEVEL_5_CASTLE:
                return "Dung in the Dungeon";
            case LeaderboardPage::LEVEL_6_BOSS:
                return "Curtains for Crap";
            case LeaderboardPage::TOTAL_ENEMIES:
                return "Total Enemies Defeated";
            case LeaderboardPage::TOTAL_COINS:
                return "Total Coins Collected";
            case LeaderboardPage::TOTAL_PIPES:
                return "Total Pipes Cleared";
            default:
                return "Leaderboard";
        }
    }

    std::string LeaderboardState::GetLocalScoreText(LeaderboardPage page) const {
        if (!m_game) {
            return "No data available";
        }

        std::ostringstream oss;

        switch (page) {
            case LeaderboardPage::LEVEL_1_PARK:
            case LeaderboardPage::LEVEL_2_SEWER:
            case LeaderboardPage::LEVEL_3_DESERT:
            case LeaderboardPage::LEVEL_4_SNOW:
            case LeaderboardPage::LEVEL_5_CASTLE: {
                int levelId = static_cast<int>(page) + 1;
                int highScore = m_game->GetLevelHighScore(levelId);
                oss << "Your Best: " << highScore << " pipes";
                break;
            }
            case LeaderboardPage::LEVEL_6_BOSS: {
                const auto& levelStats = m_game->GetLevelStats(6);
                if (levelStats.bestBossTime > 0.0f) {
                    oss << "Your Best Time: " << FormatTime(levelStats.bestBossTime);
                } else {
                    oss << "Not yet completed";
                }
                break;
            }
            case LeaderboardPage::TOTAL_ENEMIES: {
                const auto& stats = m_game->GetGameStats();
                oss << "Total Defeated: " << stats.totalEnemiesKilled;
                break;
            }
            case LeaderboardPage::TOTAL_COINS: {
                const auto& stats = m_game->GetGameStats();
                oss << "Total Collected: " << stats.totalCoinsCollected;
                break;
            }
            case LeaderboardPage::TOTAL_PIPES: {
                const auto& stats = m_game->GetGameStats();
                oss << "Total Cleared: " << stats.totalPipesCleared;
                break;
            }
            default:
                return "No data available";
        }

        return oss.str();
    }

    std::string LeaderboardState::GetLeaderboardID(LeaderboardPage page) const {
        switch (page) {
            case LeaderboardPage::LEVEL_1_PARK:
                return "com.floppyturd.park";
            case LeaderboardPage::LEVEL_2_SEWER:
                return "com.floppyturd.sewer";
            case LeaderboardPage::LEVEL_3_DESERT:
                return "com.floppyturd.desert";
            case LeaderboardPage::LEVEL_4_SNOW:
                return "com.floppyturd.snow";
            case LeaderboardPage::LEVEL_5_CASTLE:
                return "com.floppyturd.castle";
            case LeaderboardPage::LEVEL_6_BOSS:
                return "com.floppyturd.ratking.time"; // Boss is speedrun time only
            case LeaderboardPage::TOTAL_ENEMIES:
                return "com.floppyturd.totalenemies";
            case LeaderboardPage::TOTAL_COINS:
                return "com.floppyturd.totalcoins";
            case LeaderboardPage::TOTAL_PIPES:
                return "com.floppyturd.totalpipes";
            default:
                return "";
        }
    }

    std::string LeaderboardState::FormatTime(float seconds) const {
        int minutes = static_cast<int>(seconds) / 60;
        int secs = static_cast<int>(seconds) % 60;
        int millis = static_cast<int>((seconds - static_cast<int>(seconds)) * 100);

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << minutes << ":"
            << std::setfill('0') << std::setw(2) << secs << "."
            << std::setfill('0') << std::setw(2) << millis;
        return oss.str();
    }

    bool LeaderboardState::IsMobilePlatform() const {
        if (m_game) {
            return m_game->IsIOSPlatform();
        }
        return true; // Default to mobile
    }
    
    std::string LeaderboardState::FormatScore(int64_t score) const {
        return std::to_string(score);
    }
    
    void LeaderboardState::UpdateLeaderboardUI(const LeaderboardEntry* entries, int count) {
        if (!entries || count <= 0) {
            GN_LOG_WARN("⚠️ LeaderboardState::UpdateLeaderboardUI - No entries to display");
            return;
        }
        
        GN_LOG_INFO("🎨 LeaderboardState: Updating UI with " + std::to_string(count) + " entries");
        
        // The first 10 entities in m_contentEntities are the leaderboard rows
        int rowsToUpdate = std::min(count, 10);
        rowsToUpdate = std::min(rowsToUpdate, static_cast<int>(m_contentEntities.size()));
        
        for (int i = 0; i < rowsToUpdate; ++i) {
            Entity rowEntity = m_contentEntities[i];
            auto* uiElement = m_ecsSystem->GetComponent<UIElement>(rowEntity);
            
            if (uiElement) {
                // Format: "01. PlayerName  12345"
                std::string rankStr = (entries[i].rank < 10) ? ("0" + std::to_string(entries[i].rank)) : std::to_string(entries[i].rank);
                std::string playerName = entries[i].playerName;
                std::string scoreStr = FormatScore(entries[i].score);
                
                // Truncate player name if too long (max 20 chars)
                if (playerName.length() > 20) {
                    playerName = playerName.substr(0, 17) + "...";
                }
                
                // Format with spacing: "01. PlayerName          12345"
                uiElement->buttonText = rankStr + ". " + playerName;
                // Pad to align scores
                while (uiElement->buttonText.length() < 30) {
                    uiElement->buttonText += " ";
                }
                uiElement->buttonText += scoreStr;
                
                // Highlight top 3 with gold/silver/bronze colors
                if (entries[i].rank == 1) {
                    uiElement->textColor = GNColor(255, 215, 0, 255); // Gold
                } else if (entries[i].rank == 2) {
                    uiElement->textColor = GNColor(192, 192, 192, 255); // Silver
                } else if (entries[i].rank == 3) {
                    uiElement->textColor = GNColor(205, 127, 50, 255); // Bronze
                } else {
                    uiElement->textColor = GNColor(255, 255, 255, 255); // White
                }
                
                GN_LOG_INFO("  Updated row " + std::to_string(i) + ": " + uiElement->buttonText);
            }
        }
        
        // Clear any remaining placeholder rows
        for (int i = rowsToUpdate; i < std::min(10, static_cast<int>(m_contentEntities.size())); ++i) {
            Entity rowEntity = m_contentEntities[i];
            auto* uiElement = m_ecsSystem->GetComponent<UIElement>(rowEntity);
            
            if (uiElement) {
                std::string rankStr = (i + 1 < 10) ? ("0" + std::to_string(i + 1)) : std::to_string(i + 1);
                uiElement->buttonText = rankStr + ".          -------          -----";
                uiElement->textColor = GNColor(100, 100, 100, 255); // Dark gray for empty slots
            }
        }
        
        GN_LOG_INFO("✅ LeaderboardState: UI updated successfully");
    }
    
    void LeaderboardState::UpdateLocalPlayerUI(int rank, int64_t score) {
        GN_LOG_INFO("🎨 LeaderboardState: Updating local player UI - Rank: " + std::to_string(rank) + ", Score: " + std::to_string(score));
        
        // The last entity in m_contentEntities is the "Your Best" row
        if (m_contentEntities.empty()) {
            GN_LOG_WARN("⚠️ No content entities to update");
            return;
        }
        
        Entity playerRankEntity = m_contentEntities.back();
        auto* uiElement = m_ecsSystem->GetComponent<UIElement>(playerRankEntity);
        
        if (uiElement) {
            std::string rankStr = (rank > 0) ? std::to_string(rank) : "--";
            std::string scoreStr = FormatScore(score);
            
            uiElement->buttonText = "Your Best: Rank #" + rankStr + "          Score: " + scoreStr;
            uiElement->textColor = GNColor(255, 215, 0, 255); // Gold
            
            GN_LOG_INFO("  Local player UI: " + uiElement->buttonText);
        }
    }
    
    void LeaderboardState::LoadLeaderboardData() {
        GN_LOG_INFO("📊 LeaderboardState: Loading Game Center leaderboard data...");
        
        // Check if Game Center is available and authenticated
        if (!ThreadingProxy::isGameCenterAuthenticated()) {
            GN_LOG_WARN("⚠️ LeaderboardState: Not authenticated to Game Center - showing local scores only");
            return;
        }
        
        GN_LOG_INFO("✅ LeaderboardState: Game Center authenticated - loading leaderboard entries");
        
        // Get the leaderboard ID for the current page
        std::string leaderboardID = GetLeaderboardID(m_currentPage);
        if (leaderboardID.empty()) {
            GN_LOG_WARN("⚠️ LeaderboardState: No leaderboard ID for current page");
            return;
        }
        
        // Load leaderboard entries (top 25) using ThreadingProxy
        GN_LOG_INFO("📊 LeaderboardState: Requesting top 25 entries for: " + leaderboardID);
        ThreadingProxy::enqueueGameCenterLoadLeaderboardEntries(
            leaderboardID.c_str(),
            &LeaderboardState::OnLeaderboardEntriesLoaded
        );
        
        // Load local player's entry using ThreadingProxy
        GN_LOG_INFO("📊 LeaderboardState: Requesting local player entry for: " + leaderboardID);
        ThreadingProxy::enqueueGameCenterLoadLocalPlayerEntry(
            leaderboardID.c_str(),
            &LeaderboardState::OnLocalPlayerEntryLoaded
        );
    }

    // Static callback functions for Game Center
    void LeaderboardState::OnLeaderboardEntriesLoaded(const LeaderboardEntry* entries, int count, bool success) {
        if (s_instance) {
            if (success && entries && count > 0) {
                GN_LOG_INFO("✅ LeaderboardState: Received " + std::to_string(count) + " leaderboard entries");
                for (int i = 0; i < count; ++i) {
                    GN_LOG_INFO("  [" + std::to_string(i + 1) + "] Rank: " + std::to_string(entries[i].rank) +
                               ", Score: " + std::to_string(entries[i].score) +
                               ", Player: " + std::string(entries[i].playerName));
                }
                // Update UI with fetched entries
                s_instance->UpdateLeaderboardUI(entries, count);
            } else {
                GN_LOG_WARN("⚠️ LeaderboardState: Failed to load leaderboard entries or no data available");
            }
        }
    }

    void LeaderboardState::OnLocalPlayerEntryLoaded(int rank, int64_t score, bool success) {
        if (s_instance) {
            if (success) {
                GN_LOG_INFO("✅ LeaderboardState: Local player - Rank: " + std::to_string(rank) +
                           ", Score: " + std::to_string(score));
                // Update UI with local player's rank
                s_instance->UpdateLocalPlayerUI(rank, score);
            } else {
                GN_LOG_INFO("⚠️ LeaderboardState: No entry found for local player on this leaderboard");
            }
        }
    }

} // namespace GameCore