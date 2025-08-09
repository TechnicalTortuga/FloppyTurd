#include "MainMenuState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/AssetPaths.h"
#include "../../Engine/Utility/Utils.h"
#include "../Components/GameComponents.h"
#include "../Game/FloppyTurdGame.h"
#include <iostream>
#include <random>
#include <cmath>

namespace GameCore {

    MainMenuState::MainMenuState(Gnosis::ECS* ecsCoordinator, GameCore::PlatformDelegates* platformDelegates)
        : m_ecsCoordinator(ecsCoordinator)
        , m_platformDelegates(platformDelegates)
        , m_finished(false)
        , m_selectedOption(0)
        , m_animationTimer(0.0f)
        , m_isMobile(false)
        , m_currentMode(MenuMode::MAIN_MENU)
        , m_screenWidth(800.0f)
        , m_screenHeight(600.0f)
        , m_backgroundEntity(0)
        , m_logoEntity(0)
        , m_fButtonEntity(0)
        , m_playButtonEntity(0)
        , m_optionsButtonEntity(0)
        , m_quickPlayButtonEntity(0)
        , m_quitButtonEntity(0)
        , m_currentLevelIndex(0)
        , m_selectedLevelIndex(-1)
        , m_backButtonEntity(0)
        , m_leftArrowButtonEntity(0)
        , m_rightArrowButtonEntity(0)
        , m_levelPlayButtonEntity(0)
        , m_isSwiping(false)
        , m_swipeThreshold(50.0f)
        , m_swipeAnimationTimer(0.0f)
        , m_swipeAnimationDuration(0.3f)
        , m_swipeDirection(SwipeDirection::NONE)
        , m_targetOffsetX(0.0f)
        , m_currentOffsetX(0.0f)
        , m_lastArrowPressTime(0.0f)
        , m_arrowDebounceDelay(0.3f)  // 300ms debounce delay
        , m_fontLoaded(false)
        , m_assetsLoaded(false) {
        
        // Detect if we're on a mobile platform
        m_isMobile = IsMobilePlatform();
        
        // Initialize systems once in constructor
        if (m_platformDelegates) {
            // Create sprite system for texture dimension queries
            m_spriteSystem = std::make_unique<SpriteSystem>(m_ecsCoordinator, *m_platformDelegates);
            GN_LOG_INFO("MainMenuState: SpriteSystem initialized (rendering disabled, used for texture queries only)");
            if (m_spriteSystem) {
                m_spriteSystem->SetTextureBasePath("mainmenu/");
            }
            
            // Create render system for screen info access
            m_renderSystem = std::make_unique<RenderSystem>(m_ecsCoordinator, *m_platformDelegates);
            
            GN_LOG_INFO("MainMenuState systems initialized in constructor");
        } else {
            GN_LOG_ERROR("MainMenuState: PlatformDelegates is null in constructor!");
        }
        
        // Initialize level data
        InitializeLevels();
    }

    MainMenuState::~MainMenuState() {
        // UI cleanup will be handled by platform-specific rendering system
    }

    void MainMenuState::Enter() {
        GN_LOG_INFO("Entering Main Menu State");
        m_finished = false;
        m_selectedOption = 0;
        m_animationTimer = 0.0f;
        m_assetsLoaded = false;
        
        // Start playing main menu music using delegate system
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
            // Check if music is cached using new delegate
            if (delegates.asset.isCached) {
                bool cached = delegates.asset.isCached("FloppyTurdMenu", 1); // 1 = audio type
                GN_LOG_INFO("FloppyTurdMenu cached status: " + std::string(cached ? "true" : "false"));
            }
            // Play music using existing delegate
            if (delegates.audio.playMusic) {
                delegates.audio.playMusic("FloppyTurdMenu", 0.7f, -1); // -1 = infinite loop
                GN_LOG_INFO("Started main menu music: FloppyTurdMenu.mp3");
            }
        }
        
        // Load the Whacky Joe font for text rendering
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
            if (delegates.asset.loadFont) {
                // For now, just load the font without callback to test
                delegates.asset.loadFont("fonts/Whacky_Joe", 32, nullptr, nullptr);
                GN_LOG_INFO("Requested Whacky Joe font loading with path: fonts/Whacky_Joe");
                
                // Set font as loaded after a short delay to allow loading
                m_fontLoaded = true;
            } else {
                GN_LOG_INFO("❌ Font loading delegate not available");
            }
        } else {
            GN_LOG_INFO("❌ Game instance not available for font loading");
        }
        
        // Create the appropriate layout based on platform
        if (m_isMobile) {
            CreateMobileLayout();
        } else {
            CreateDesktopLayout();
        }
        
        // Create UI elements (buttons with integrated text)
        CreateUIElements();
        
        // Create level select layout (hidden initially)
        CreateLevelSelectLayout();
        
        m_assetsLoaded = true;
        GN_LOG_INFO("Main Menu State fully initialized");
    }

    void MainMenuState::Exit() {
        GN_LOG_INFO("Exiting Main Menu State");
        
        // Stop menu music using delegate system
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
            if (delegates.audio.stopMusic) {
                delegates.audio.stopMusic();
                GN_LOG_INFO("Stopped main menu music");
            }
        }
        
        // Cleanup UI entities
        if (m_ecsCoordinator && m_assetsLoaded) {
            if (m_backgroundEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_backgroundEntity);
            }
            if (m_logoEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_logoEntity);
            }
            if (m_fButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_fButtonEntity);
            }
            if (m_playButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_playButtonEntity);
            }
            if (m_optionsButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_optionsButtonEntity);
            }
            if (m_quickPlayButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_quickPlayButtonEntity);
            }
            if (m_quitButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_quitButtonEntity);
            }
            
            // Cleanup level select entities
            if (m_backButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_backButtonEntity);
            }
            if (m_levelPlayButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_levelPlayButtonEntity);
            }
            if (m_leftArrowButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_leftArrowButtonEntity);
            }
            if (m_rightArrowButtonEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_rightArrowButtonEntity);
            }
            
            for (Gnosis::Entity entity : m_levelPaintingEntities) {
                if (entity != 0) {
                    m_ecsCoordinator->DestroyEntity(entity);
                }
            }
            
            for (Gnosis::Entity entity : m_levelFrameEntities) {
                if (entity != 0) {
                    m_ecsCoordinator->DestroyEntity(entity);
                }
            }
            
            for (Gnosis::Entity entity : m_levelTextEntities) {
                if (entity != 0) {
                    m_ecsCoordinator->DestroyEntity(entity);
                }
            }
            
            if (m_lockedIndicatorEntity != 0) {
                m_ecsCoordinator->DestroyEntity(m_lockedIndicatorEntity);
            }
            
            GN_LOG_INFO("Cleaned up main menu and level select UI entities");
        }
    }

    void MainMenuState::Pause() {
        // Main menu can be paused if needed
        GN_LOG_INFO("Main Menu State paused");
    }

    void MainMenuState::Resume() {
        // Resume main menu
        GN_LOG_INFO("Main Menu State resumed");
    }

    void MainMenuState::Update(float deltaTime) {
        m_animationTimer += deltaTime;
        
        // Update arrow button debounce timer
        m_lastArrowPressTime += deltaTime;
        
        // Update menu animations (logo bobbing, button highlights, etc.)
        UpdateMenuAnimations(deltaTime);
        
        // Handle menu selection changes
        UpdateMenuSelection();
        
        // Update swipe animation if in level select mode
        if (m_currentMode == MenuMode::LEVEL_SELECT) {
            AnimateSwipe(deltaTime);
        }
        
        // Update ECS systems
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Update(deltaTime);
        }
    }

    void MainMenuState::Render() {
        // Render through ECS system (sprites, buttons, text, etc.)
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Render();
        }
        
        // Debug: Draw button bounds rectangles
        extern FloppyTurdGame* g_Game;
        if (g_Game && g_Game->GetPlatformDelegates().renderer.drawRectangle) {
            // Draw debug rectangles for each button
            DrawButtonDebugRectangles();
            
            // Draw level select debug info
            if (m_currentMode == MenuMode::LEVEL_SELECT) {
                DrawLevelSelectDebugInfo();
            }
        }
    }

    void MainMenuState::HandleInput() {
        if (!m_ecsCoordinator || !m_assetsLoaded) {
            return;
        }
        
        // Get input from platform delegates
        extern FloppyTurdGame* g_Game;
        if (!g_Game) {
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        
        // Debug: Check if input delegates are properly set
        if (!delegates.input.isPrimaryInputJustPressed) {
            GN_LOG_ERROR("MainMenuState: isPrimaryInputJustPressed delegate is NULL!");
            return;
        }
        
        // Handle input based on current mode
        if (m_currentMode == MenuMode::MAIN_MENU) {
            HandleMainMenuInput(delegates);
        } else if (m_currentMode == MenuMode::LEVEL_SELECT) {
            HandleLevelSelectInput();
        }
    }
    
    void MainMenuState::HandleMainMenuInput(const PlatformDelegates& delegates) {
        // Handle touch/click input for F button
        bool inputPressed = delegates.input.isPrimaryInputJustPressed();
        if (inputPressed) {
            GN_LOG_INFO("🎮 MainMenuState: Input detected! Checking F button bounds...");
        }
        
        if (inputPressed) {
            float touchX, touchY;
            if (delegates.input.getPrimaryInputPosition) {
                delegates.input.getPrimaryInputPosition(&touchX, &touchY);
                
                // Check if touch/click is within F button bounds
                if (m_fButtonEntity != 0) {
                    Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
                    Sprite* fButtonSprite = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity);
                    
                    if (fButtonTransform && fButtonSprite) {
                        // F button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = fButtonSprite->width * fButtonTransform->scale.x;
                        float buttonHeight = fButtonSprite->height * fButtonTransform->scale.y;
                        float buttonLeft = fButtonTransform->position.x;
                        float buttonRight = fButtonTransform->position.x + buttonWidth;
                        float buttonTop = fButtonTransform->position.y;
                        float buttonBottom = fButtonTransform->position.y + buttonHeight;
                        
                        // Check if touch is within bounds
                        GN_LOG_INFO("🎯 MainMenuState: Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                                   "), F button bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                                   " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom));
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            GN_LOG_INFO("🎉 MainMenuState: F BUTTON HIT! Playing fart sound...");
                            OnFButtonPressed();
                        } else {
                            GN_LOG_INFO("❌ MainMenuState: Touch missed F button");
                            
                            // Check menu buttons
                            CheckMenuButtonClicks(touchX, touchY);
                        }
                    }
                }
            }
        }
        
        // TODO: Handle menu navigation (up/down arrows) for desktop
        // TODO: Handle selection (enter/space) for menu options
    }

    void MainMenuState::CreateDesktopLayout() {
        GN_LOG_INFO("Creating desktop main menu layout");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateDesktopLayout");
            return;
        }
        
        // Get enhanced screen information from render system (like GameplayState)
        ScreenInfo screenInfo;
        if (m_renderSystem) {
            screenInfo = m_renderSystem->GetScreenInfo();
            GN_LOG_INFO("Desktop enhanced screen info: logical=" + 
                       std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight) + 
                       ", pixel=" + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + 
                       ", scale=" + std::to_string(screenInfo.scaleFactor));
        } else {
            // Fallback to platform delegates if render system not available
            extern FloppyTurdGame* g_Game;
            if (g_Game) {
                const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
                if (delegates.renderer.getScreenInfo) {
                    delegates.renderer.getScreenInfo(&screenInfo);
                    GN_LOG_INFO("Desktop enhanced screen info (delegate): logical=" + 
                               std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight) + 
                               ", pixel=" + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + 
                               ", scale=" + std::to_string(screenInfo.scaleFactor));
                } else {
                    // Fallback to legacy screen size if enhanced info not available
                    if (delegates.renderer.getScreenSize) {
                        delegates.renderer.getScreenSize(&screenInfo.logicalWidth, &screenInfo.logicalHeight);
                        screenInfo.pixelWidth = screenInfo.logicalWidth;
                        screenInfo.pixelHeight = screenInfo.logicalHeight;
                        screenInfo.scaleFactor = 1.0f;
                        GN_LOG_INFO("Desktop fallback screen dimensions: " + std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight));
                    } else {
                        // Ultimate fallback
                        screenInfo.logicalWidth = 800.0f;
                        screenInfo.logicalHeight = 600.0f;
                        screenInfo.pixelWidth = 800.0f;
                        screenInfo.pixelHeight = 600.0f;
                        screenInfo.scaleFactor = 1.0f;
                    }
                }
            }
        }
        
        // Store screen dimensions for consistent use across the class - USE PIXEL DIMENSIONS for proper scaling
        m_screenWidth = screenInfo.pixelWidth;
        m_screenHeight = screenInfo.pixelHeight;
        
        float centerX = screenInfo.pixelWidth / 2.0f;
        float centerY = screenInfo.pixelHeight / 2.0f;
        
        // 1. Create Background Entity (MainMenu.png) - FULL SCREEN SCALING
        m_backgroundEntity = m_ecsCoordinator->CreateEntity();
        
        // Load texture to get actual dimensions
        m_spriteSystem->LoadTexture("MainMenu", "MainMenu.png");
        auto bgDimensions = m_spriteSystem->GetTextureDimensions("MainMenu");
        float textureWidth = bgDimensions.first > 0 ? bgDimensions.first : 320.0f;   // Use actual width or fallback
        float textureHeight = bgDimensions.second > 0 ? bgDimensions.second : 180.0f; // Use actual height or fallback
        
        // Calculate scale to fill screen - USE PIXEL DIMENSIONS
        float scaleX = screenInfo.pixelWidth / textureWidth;
        float scaleY = screenInfo.pixelHeight / textureHeight;
        
        // Position at top-left (0,0) since we now render from top-left
        Transform bgTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(scaleX, scaleY));
        
        // Create sprite with actual texture dimensions, scale will be applied by transform
        Sprite bgSprite("MainMenu", textureWidth, textureHeight);
        bgSprite.layer = 0; // Background layer
        bgSprite.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_backgroundEntity, bgTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_backgroundEntity, bgSprite);
        GN_LOG_INFO("Created full-screen background entity: MainMenu.png (texture: " + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) + ", scale: " + std::to_string(scaleX) + "x" + std::to_string(scaleY) + ", screen: " + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + ")");
        
        // 2. Create Logo Entity (FloppyLogo.png) - Desktop scaling
        m_logoEntity = m_ecsCoordinator->CreateEntity();
        
        // Load texture to get actual dimensions
        m_spriteSystem->LoadTexture("FloppyLogo", "FloppyLogo.png");
        auto logoDimensions = m_spriteSystem->GetTextureDimensions("FloppyLogo");
        float logoWidth = logoDimensions.first > 0 ? logoDimensions.first : 112.0f;   // Use actual width or fallback
        float logoHeight = logoDimensions.second > 0 ? logoDimensions.second : 80.0f; // Use actual height or fallback
        float logoScale = 2.0f; // 2x scale for desktop
        
        // Calculate logo position - use same x,y for both logo and F button - USE PIXEL DIMENSIONS
        float logoX = centerX - 150.0f; // Position to the left of center
        float logoY = screenInfo.pixelHeight * 0.35f; // 35% down from top
        
        // Calculate scaled dimensions using helper
        auto logoScaledDimensions = GetScaledDimensions(logoWidth, logoHeight, logoScale);
        float scaledLogoWidth = logoScaledDimensions.first;
        float scaledLogoHeight = logoScaledDimensions.second;
        
        // Logo position is already top-left based
        float logoTopLeftX = logoX;  // Already top-left for logo
        float logoTopLeftY = logoY;  // Already top-left for logo
        
        Transform logoTransform(Gnosis::GNVector2(logoTopLeftX, logoTopLeftY), 0.0f, Gnosis::GNVector2(logoScale, logoScale));
        Sprite logoSprite("FloppyLogo", logoWidth, logoHeight); // Use actual texture dimensions
        logoSprite.layer = 1; // Logo layer
        logoSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_logoEntity, logoTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_logoEntity, logoSprite);
        GN_LOG_INFO("Created scaled logo entity: FloppyLogo.png (2x scale)");
        
        // 3. Create Interactive F Button Entity (F.png) - Desktop scaling
        m_fButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Load texture to get actual dimensions
        m_spriteSystem->LoadTexture("F", "F.png");
        auto fButtonDimensions = m_spriteSystem->GetTextureDimensions("F");
        float fButtonTextureWidth = fButtonDimensions.first > 0 ? fButtonDimensions.first : 28.0f;   // Use actual width or fallback
        float fButtonTextureHeight = fButtonDimensions.second > 0 ? fButtonDimensions.second : 40.0f; // Use actual height or fallback
        
        // Position F button using SAME x,y coordinates as logo for perfect alignment
        float fButtonX = logoX; // Use same X as logo
        float fButtonY = logoY; // Use same Y as logo
        
        // Scale F button to match logo scale
        float fButtonScale = logoScale; // Use same scale as logo
        auto fButtonScaledDimensions = GetScaledDimensions(fButtonTextureWidth, fButtonTextureHeight, fButtonScale);
        float fButtonWidth = fButtonScaledDimensions.first;
        float fButtonHeight = fButtonScaledDimensions.second;
        
        // Position F button at same top-left coordinates as logo
        float fButtonTopLeftX = fButtonX;
        float fButtonTopLeftY = fButtonY;
        
        // Update logo transform to match F button exactly 
        logoTransform = Transform(Gnosis::GNVector2(logoTopLeftX, logoTopLeftY), 0.0f, Gnosis::GNVector2(logoScale, logoScale));
        m_ecsCoordinator->AddComponent<Transform>(m_logoEntity, logoTransform);
        
        Transform fButtonTransform(Gnosis::GNVector2(fButtonTopLeftX, fButtonTopLeftY), 0.0f, Gnosis::GNVector2(fButtonScale, fButtonScale));
        Sprite fButtonSprite("F", fButtonTextureWidth, fButtonTextureHeight); // Use actual texture dimensions
        fButtonSprite.layer = 2; // F button layer
        fButtonSprite.visible = true;
        m_ecsCoordinator->AddComponent<Transform>(m_fButtonEntity, fButtonTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_fButtonEntity, fButtonSprite);
        GN_LOG_INFO("Created F button entity: F.png (" + std::to_string(fButtonScale) + "x scale, actual size: " + std::to_string(fButtonTextureWidth) + "x" + std::to_string(fButtonTextureHeight) + ")");
        
        // Create menu buttons for desktop
        CreateMenuButtons();
        
        GN_LOG_INFO("Desktop layout created: Background, Logo, F Button, and Menu Button entities");
    }

    void MainMenuState::CreateMobileLayout() {
        GN_LOG_INFO("Creating mobile main menu layout");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMobileLayout");
            return;
        }
        
        // Setup platform-specific layout first
        SetupLayout();
        
        // Get enhanced screen information from render system (like GameplayState)
        ScreenInfo screenInfo;
        if (m_renderSystem) {
            screenInfo = m_renderSystem->GetScreenInfo();
            GN_LOG_INFO("Mobile enhanced screen info: logical=" + 
                       std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight) + 
                       ", pixel=" + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + 
                       ", scale=" + std::to_string(screenInfo.scaleFactor));
        } else {
            // Fallback to platform delegates if render system not available
            extern FloppyTurdGame* g_Game;
            if (g_Game) {
                const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
                if (delegates.renderer.getScreenInfo) {
                    delegates.renderer.getScreenInfo(&screenInfo);
                    GN_LOG_INFO("Mobile enhanced screen info (delegate): logical=" + 
                               std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight) + 
                               ", pixel=" + std::to_string(screenInfo.pixelWidth) + "x" + std::to_string(screenInfo.pixelHeight) + 
                               ", scale=" + std::to_string(screenInfo.scaleFactor));
                } else {
                    // Fallback to legacy screen size if enhanced info not available
                    if (delegates.renderer.getScreenSize) {
                        delegates.renderer.getScreenSize(&screenInfo.logicalWidth, &screenInfo.logicalHeight);
                        screenInfo.pixelWidth = screenInfo.logicalWidth;
                        screenInfo.pixelHeight = screenInfo.logicalHeight;
                        screenInfo.scaleFactor = 1.0f;
                        GN_LOG_INFO("Mobile fallback screen dimensions: " + std::to_string(screenInfo.logicalWidth) + "x" + std::to_string(screenInfo.logicalHeight));
                    } else {
                        // Ultimate fallback to iPhone 16 logical dimensions
                        screenInfo.logicalWidth = 393.0f;
                        screenInfo.logicalHeight = 852.0f;
                        screenInfo.pixelWidth = 1179.0f;
                        screenInfo.pixelHeight = 2556.0f;
                        screenInfo.scaleFactor = 3.0f;
                    }
                }
            }
        }
        
        // Store screen dimensions for consistent use across the class - USE PIXEL DIMENSIONS for proper scaling
        m_screenWidth = screenInfo.pixelWidth;
        m_screenHeight = screenInfo.pixelHeight;
        
        // === SIMPLIFIED MOBILE LAYOUT POSITIONING === //
        
        // 1. Create Background Entity (MainMenuMobile.png) - FULL SCREEN SCALING
        m_backgroundEntity = m_ecsCoordinator->CreateEntity();
        
        // Load texture to get actual dimensions
        m_spriteSystem->LoadTexture("MainMenuMobile", "MainMenuMobile.png");
        auto bgDimensions = m_spriteSystem->GetTextureDimensions("MainMenuMobile");
        float bgTextureWidth = bgDimensions.first > 0 ? bgDimensions.first : 393.0f;
        float bgTextureHeight = bgDimensions.second > 0 ? bgDimensions.second : 852.0f;
        
        // Calculate scale to fill screen - USE PIXEL DIMENSIONS
        float bgScaleX = screenInfo.pixelWidth / bgTextureWidth;
        float bgScaleY = screenInfo.pixelHeight / bgTextureHeight;
        
        // Background positioned at (0,0) top-left
        Transform bgTransform(Gnosis::GNVector2(0.0f, 0.0f), 0.0f, Gnosis::GNVector2(bgScaleX, bgScaleY));
        Sprite bgSprite("MainMenuMobile", bgTextureWidth, bgTextureHeight);
        bgSprite.layer = 0;
        bgSprite.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_backgroundEntity, bgTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_backgroundEntity, bgSprite);
        GN_LOG_INFO("✅ Created background: MainMenuMobile.png at (0,0) with scale (" + std::to_string(bgScaleX) + "x" + std::to_string(bgScaleY) + ")");
        
        // 2. Create Logo Entity - SIMPLE POSITIONING
        m_logoEntity = m_ecsCoordinator->CreateEntity();
        
        // Load logo texture dimensions
        auto logoDimensions = m_spriteSystem->GetTextureDimensions("FloppyLogo");
        float logoTextureWidth = logoDimensions.first > 0 ? logoDimensions.first : 112.0f;
        float logoTextureHeight = logoDimensions.second > 0 ? logoDimensions.second : 80.0f;
        
        // Logo positioning: Center horizontally, 20% down from top
        float logoScale = 8.0f;  // Fixed scale for mobile
        auto logoScaledDimensions = GetScaledDimensions(logoTextureWidth, logoTextureHeight, logoScale);
        float logoScaledWidth = logoScaledDimensions.first;
        float logoScaledHeight = logoScaledDimensions.second;
        
        // For top-left rendering, position logo so it's centered on screen but accounting for its size
        // Calculate position so logo appears centered but renders from top-left - USE PIXEL DIMENSIONS
        float logoX = (screenInfo.pixelWidth - logoScaledWidth) / 2.0f;  // Center horizontally with top-left rendering
        float logoY = screenInfo.pixelHeight * 0.15f;  // 15% from top for top-left rendering
        
        Transform logoTransform(Gnosis::GNVector2(logoX, logoY), 0.0f, Gnosis::GNVector2(logoScale, logoScale));
        Sprite logoSprite("FloppyLogo", logoTextureWidth, logoTextureHeight);
        logoSprite.layer = 1;
        logoSprite.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_logoEntity, logoTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_logoEntity, logoSprite);
        GN_LOG_INFO("✅ Created logo: FloppyLogo.png at (" + std::to_string(logoX) + "," + std::to_string(logoY) + ") with scale " + std::to_string(logoScale) + "x" + std::to_string(logoScale));
        GN_LOG_INFO("🎯 LOGO DEBUG: logoTextureWidth=" + std::to_string(logoTextureWidth) + ", logoTextureHeight=" + std::to_string(logoTextureHeight) + ", logoScaledWidth=" + std::to_string(logoScaledWidth) + ", logoScaledHeight=" + std::to_string(logoScaledHeight));
        
        // 3. Create F Button Entity - OVERLAID ON LOGO
        m_fButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Load F button texture dimensions
        auto fButtonDimensions = m_spriteSystem->GetTextureDimensions("F");
        float fButtonTextureWidth = fButtonDimensions.first > 0 ? fButtonDimensions.first : 28.0f;
        float fButtonTextureHeight = fButtonDimensions.second > 0 ? fButtonDimensions.second : 40.0f;
        
        // F button uses SAME position and scale as logo for perfect overlay
        float fButtonScale = logoScale;  // Match logo scale exactly
        
        Transform fButtonTransform(Gnosis::GNVector2(logoX, logoY), 0.0f, Gnosis::GNVector2(fButtonScale, fButtonScale));
        Sprite fButtonSprite("F", fButtonTextureWidth, fButtonTextureHeight);
        fButtonSprite.layer = 2; // Above logo
        fButtonSprite.visible = true;
        
        m_ecsCoordinator->AddComponent<Transform>(m_fButtonEntity, fButtonTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_fButtonEntity, fButtonSprite);
        GN_LOG_INFO("✅ Created F button: F.png at (" + std::to_string(logoX) + "," + std::to_string(logoY) + ") with scale " + std::to_string(fButtonScale) + "x" + std::to_string(fButtonScale) + " (overlaid on logo)");
        GN_LOG_INFO("🎯 F BUTTON DEBUG: fButtonTextureWidth=" + std::to_string(fButtonTextureWidth) + ", fButtonTextureHeight=" + std::to_string(fButtonTextureHeight) + ", using EXACT same coordinates as logo");
        
        // 4. Create menu buttons for mobile
        CreateMobileMenuButtons();
        
        GN_LOG_INFO("✅ Mobile layout created successfully with simplified positioning");
    }

    void MainMenuState::UpdateMenuSelection() {
        // TODO: Handle menu selection logic
        // - Update selected option based on input
        // - Update visual highlights
        // - Handle selection confirmation
        
        // Placeholder for menu selection updates
    }

    void MainMenuState::UpdateMenuAnimations(float deltaTime) {
        if (!m_ecsCoordinator || !m_assetsLoaded) {
            return;
        }
        
        // Logo floating animation
        if (m_logoEntity != 0) {
            Transform* logoTransform = m_ecsCoordinator->GetComponent<Transform>(m_logoEntity);
            if (logoTransform) {
                // Create a gentle floating effect
                float logoFloat = sin(m_animationTimer * 1.5f) * 8.0f; // 8 pixel float amplitude
                
                // Update logo Y position (preserve original Y + float offset)
                // Use the SAME calculation as in CreateMobileLayout for consistency
                float originalLogoY = m_screenHeight * 0.15f; // 15% from top (matches CreateMobileLayout)
                logoTransform->position.y = originalLogoY + logoFloat;
                
                GN_LOG_INFO("🌊 Logo animation: originalY=" + std::to_string(originalLogoY) + ", float=" + std::to_string(logoFloat) + ", finalY=" + std::to_string(logoTransform->position.y));
            }
        }
        
        // F Button follows logo animation perfectly - same position + gentle pulsing
        if (m_fButtonEntity != 0) {
            Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
            if (fButtonTransform) {
                // Make F button follow logo's floating animation exactly
                float logoFloat = sin(m_animationTimer * 1.5f) * 8.0f; // Same float as logo
                float originalLogoY = m_screenHeight * 0.15f; // Same Y calculation as logo
                fButtonTransform->position.y = originalLogoY + logoFloat; // Follow logo's Y position exactly
                
                // Create a subtle pulsing scale effect - use consistent 8x scale for mobile
                float baseScale = m_isMobile ? 8.0f : 2.5f; // Fixed mobile scale to 8.0f
                float pulseScale = baseScale + sin(m_animationTimer * 2.5f) * 0.3f;
                fButtonTransform->scale.x = pulseScale;
                fButtonTransform->scale.y = pulseScale;
                
                GN_LOG_INFO("🔄 F Button follows logo: Y=" + std::to_string(fButtonTransform->position.y) + ", scale=" + std::to_string(pulseScale));
            }
        }
    }

    void MainMenuState::OnMenuOptionSelected(MenuOption option) {
        GN_LOG_INFO("Menu option selected: " + std::to_string(static_cast<int>(option)));
        
        switch (option) {
            case MenuOption::PLAYING:
                GN_LOG_INFO("Starting game...");
                // TODO: Transition to game state
                m_finished = true;
                break;
                
            case MenuOption::OPTIONS:
                GN_LOG_INFO("Opening options menu...");
                // TODO: Transition to options state
                break;
                
            case MenuOption::QUICK_PLAY:
                GN_LOG_INFO("Starting quick play...");
                // TODO: Transition to quick play state
                m_finished = true;
                break;
                
            case MenuOption::QUIT:
                GN_LOG_INFO("Quitting game...");
                // TODO: Quit application
                m_finished = true;
                break;
                
            default:
                GN_LOG_WARN("Unknown menu option selected");
                break;
        }
    }

    void MainMenuState::OnFButtonPressed() {
        GN_LOG_INFO("F button pressed - playing random fart sound!");
        
        // Play random fart sound (fart1.mp3 through fart11.mp3)
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(1, 11);
        
        int randomFartNumber = dis(gen);
        std::string fartSoundName = "fart" + std::to_string(randomFartNumber);
        
        // Use delegate system to play sound
        extern FloppyTurdGame* g_Game;
        if (g_Game) {
            const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
            if (delegates.audio.playSound) {
                delegates.audio.playSound(fartSoundName.c_str(), 0.8f); // 80% volume
                GN_LOG_INFO("Playing fart sound: %s.mp3", fartSoundName.c_str());
            }
        }
        
        // Add visual feedback - make F button briefly larger
        if (m_fButtonEntity != 0 && m_ecsCoordinator) {
            Transform* fButtonTransform = m_ecsCoordinator->GetComponent<Transform>(m_fButtonEntity);
            if (fButtonTransform) {
                // Temporarily scale up the F button for feedback - based on 8x scale for mobile
                float feedbackScale = m_isMobile ? 9.0f : 3.0f; // Bigger feedback for mobile 8x base scale
                fButtonTransform->scale.x = feedbackScale;
                fButtonTransform->scale.y = feedbackScale;
                // Note: This will be smoothed back by the pulsing animation
            }
        }
    }

    const char* MainMenuState::GetMenuOptionText(int optionIndex) const {
        switch (static_cast<MenuOption>(optionIndex)) {
            case MenuOption::PLAYING:    return "Playing";
            case MenuOption::OPTIONS:    return "Options";
            case MenuOption::QUICK_PLAY: return "Quick Play";
            case MenuOption::QUIT:       return "Quit";
            default:                     return "Unknown";
        }
    }

    int MainMenuState::GetMenuOptionCount() const {
        return static_cast<int>(MenuOption::COUNT);
    }

    bool MainMenuState::IsMobilePlatform() const {
        // TODO: Implement platform detection
        // This should check the actual platform we're running on
        // For now, return false (desktop) as default
        
        #ifdef __APPLE__
            #include "TargetConditionals.h"
            #if TARGET_OS_IPHONE
                return true;  // iOS
            #else
                return false; // macOS
            #endif
        #elif defined(__ANDROID__)
            return true;  // Android
        #else
            return false; // Desktop (Windows, Linux, etc.)
        #endif
    }

    void MainMenuState::CreateMenuButtons() {
        GN_LOG_INFO("Creating desktop menu buttons");
        
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMenuButtons");
            return;
        }
        
        // Load button texture to get actual dimensions
        m_spriteSystem->LoadTexture("FloppyButtonBlue", "FloppyButtonBlue.png");
        auto buttonDimensions = m_spriteSystem->GetTextureDimensions("FloppyButtonBlue");
        float buttonTextureWidth = buttonDimensions.first > 0 ? buttonDimensions.first : 90.0f;   // Use actual width or fallback
        float buttonTextureHeight = buttonDimensions.second > 0 ? buttonDimensions.second : 16.0f; // Use actual height or fallback
        
        GN_LOG_INFO("Button texture dimensions: " + std::to_string(buttonTextureWidth) + "x" + std::to_string(buttonTextureHeight));
        
        float centerX = m_screenWidth / 2.0f;
        float buttonY = m_screenHeight * 0.55f; // Position buttons higher up
        float buttonSpacing = 150.0f; // Much more spacing between buttons
        float buttonScale = 10.0f; // Perfect mobile button size (scale factor fix applied)
        
        // Create Play Button
        m_playButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Calculate scaled dimensions using helper
        auto playButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float playButtonWidth = playButtonScaledDimensions.first;
        float playButtonHeight = playButtonScaledDimensions.second;
        
        // Use positioning helper to center button
        Gnosis::GNVector2 playButtonPosition = CenterObjectAtPosition(centerX, buttonY, playButtonWidth, playButtonHeight);
        float playButtonTopLeftX = playButtonPosition.x;
        float playButtonTopLeftY = playButtonPosition.y;
        
        Transform playTransform(Gnosis::GNVector2(playButtonTopLeftX, playButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite playSprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight); // Use actual texture dimensions
        playSprite.layer = 2; // Button layer (lower than text)
        playSprite.visible = true;
        UIElement playButton("PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        playButton.fontSize = 18.0f; // Reduced for better fit in button
        playButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Play button with text: '%s' (length: %zu)", playButton.buttonText.c_str(), playButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_playButtonEntity, playTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_playButtonEntity, playSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_playButtonEntity, playButton);
        
        // Create Options Button
        m_optionsButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Calculate scaled dimensions using helper
        auto optionsButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float optionsButtonWidth = optionsButtonScaledDimensions.first;
        float optionsButtonHeight = optionsButtonScaledDimensions.second;
        
        // Use positioning helper to center button
        float optionsButtonCenterY = buttonY + buttonSpacing;
        Gnosis::GNVector2 optionsButtonPosition = CenterObjectAtPosition(centerX, optionsButtonCenterY, optionsButtonWidth, optionsButtonHeight);
        float optionsButtonTopLeftX = optionsButtonPosition.x;
        float optionsButtonTopLeftY = optionsButtonPosition.y;
        
        Transform optionsTransform(Gnosis::GNVector2(optionsButtonTopLeftX, optionsButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite optionsSprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight); // Use actual texture dimensions
        optionsSprite.layer = 2; // Button layer (lower than text)
        optionsSprite.visible = true;
        UIElement optionsButton("OPTIONS", "FloppyButtonBlue", "FloppyButtonBlueHover");
        optionsButton.fontSize = 18.0f; // Reduced for better fit in button
        optionsButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Options button with text: '%s' (length: %zu)", optionsButton.buttonText.c_str(), optionsButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_optionsButtonEntity, optionsTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_optionsButtonEntity, optionsSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_optionsButtonEntity, optionsButton);
        
        // Create Quick Play Button
        m_quickPlayButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Calculate scaled dimensions using helper
        auto quickPlayButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float quickPlayButtonWidth = quickPlayButtonScaledDimensions.first;
        float quickPlayButtonHeight = quickPlayButtonScaledDimensions.second;
        
        // Use positioning helper to center button
        float quickPlayButtonCenterY = buttonY + buttonSpacing * 2;
        Gnosis::GNVector2 quickPlayButtonPosition = CenterObjectAtPosition(centerX, quickPlayButtonCenterY, quickPlayButtonWidth, quickPlayButtonHeight);
        float quickPlayButtonTopLeftX = quickPlayButtonPosition.x;
        float quickPlayButtonTopLeftY = quickPlayButtonPosition.y;
        
        Transform quickPlayTransform(Gnosis::GNVector2(quickPlayButtonTopLeftX, quickPlayButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quickPlaySprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight); // Use actual texture dimensions
        quickPlaySprite.layer = 2; // Button layer (lower than text)
        quickPlaySprite.visible = true;
        UIElement quickPlayButton("QUICK PLAY", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quickPlayButton.fontSize = 16.0f; // Reduced significantly to fit longer text
        quickPlayButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Quick Play button with text: '%s' (length: %zu)", quickPlayButton.buttonText.c_str(), quickPlayButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quickPlayButtonEntity, quickPlayTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quickPlayButtonEntity, quickPlaySprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_quickPlayButtonEntity, quickPlayButton);
        
        // Create Quit Button
        m_quitButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Calculate scaled dimensions using helper
        auto quitButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float quitButtonWidth = quitButtonScaledDimensions.first;
        float quitButtonHeight = quitButtonScaledDimensions.second;
        
        // Use positioning helper to center button
        float quitButtonCenterY = buttonY + buttonSpacing * 3;
        Gnosis::GNVector2 quitButtonPosition = CenterObjectAtPosition(centerX, quitButtonCenterY, quitButtonWidth, quitButtonHeight);
        float quitButtonTopLeftX = quitButtonPosition.x;
        float quitButtonTopLeftY = quitButtonPosition.y;
        
        Transform quitTransform(Gnosis::GNVector2(quitButtonTopLeftX, quitButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite quitSprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight); // Use actual texture dimensions
        quitSprite.layer = 2; // Button layer (lower than text)
        quitSprite.visible = true;
        UIElement quitButton("QUIT", "FloppyButtonBlue", "FloppyButtonBlueHover");
        quitButton.fontSize = 18.0f; // Reduced for better fit in button
        quitButton.textColor = Gnosis::GNColor(255, 255, 255, 255); // White text
        GN_LOG_INFO("Created Quit button with text: '%s' (length: %zu)", quitButton.buttonText.c_str(), quitButton.buttonText.length());
        
        m_ecsCoordinator->AddComponent<Transform>(m_quitButtonEntity, quitTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_quitButtonEntity, quitSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_quitButtonEntity, quitButton);
        
        GN_LOG_INFO("Created desktop menu buttons: Play, Options, Quick Play, Quit");
    }

    void MainMenuState::CreateMobileMenuButtons() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in CreateMobileMenuButtons");
            return;
        }
        
        // Load button texture dimensions once
        auto buttonDimensions = m_spriteSystem->GetTextureDimensions("FloppyButtonBlue");
        float buttonTextureWidth = buttonDimensions.first > 0 ? buttonDimensions.first : 90.0f;
        float buttonTextureHeight = buttonDimensions.second > 0 ? buttonDimensions.second : 16.0f;
        
        GN_LOG_INFO("Mobile button texture dimensions: " + std::to_string(buttonTextureWidth) + "x" + std::to_string(buttonTextureHeight));
        
        // === SIMPLIFIED MOBILE BUTTON POSITIONING === //
        
        float buttonScale = 10.0f;  // Perfect mobile button size (scale factor fix applied)
        auto buttonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float buttonScaledWidth = buttonScaledDimensions.first;
        float buttonScaledHeight = buttonScaledDimensions.second;
        
        // Center buttons horizontally, start at 50% down from top
        float centerX = m_screenWidth / 2.0f;
        float startY = m_screenHeight * 0.50f;  // Start at 50% down from top
        float buttonSpacing = buttonScaledHeight + 100.0f;  // Spacing between buttons (button height + gap)
        
        GN_LOG_INFO("📱 Creating mobile buttons: scale=" + std::to_string(buttonScale) + ", size=" + std::to_string(buttonScaledWidth) + "x" + std::to_string(buttonScaledHeight) + ", centerX=" + std::to_string(centerX) + ", startY=" + std::to_string(startY) + ", spacing=" + std::to_string(buttonSpacing));
        
        // Helper lambda for creating buttons with consistent positioning
        auto createButton = [&](Gnosis::Entity& entity, const std::string& text, int buttonIndex) {
            entity = m_ecsCoordinator->CreateEntity();
            
            // Calculate button position using positioning helper
            float buttonCenterY = startY + (buttonIndex * buttonSpacing);
            Gnosis::GNVector2 buttonPosition = CenterObjectHorizontally(m_screenWidth, buttonCenterY, 
                                                                       buttonScaledWidth, buttonScaledHeight);
            float buttonX = buttonPosition.x;
            float buttonY = buttonPosition.y;
            
            Transform transform(Gnosis::GNVector2(buttonX, buttonY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
            Sprite sprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight);
            sprite.layer = 2;
            sprite.visible = true;
            
            UIElement uiElement("", "FloppyButtonBlue", "FloppyButtonBlueHover");
            uiElement.buttonText = text;
            uiElement.fontSize = 96.0f;  // Large text for mobile
            uiElement.textColor = Gnosis::GNColor(255, 255, 255, 255);  // White text
            uiElement.centerTextHorizontally = true;
            uiElement.centerTextVertically = true;
            // NO OFFSET - text should be perfectly centered as requested
            uiElement.textOffsetY = 0.0f;  // No offset for perfect centering
            
            m_ecsCoordinator->AddComponent<Transform>(entity, transform);
            m_ecsCoordinator->AddComponent<Sprite>(entity, sprite);
            m_ecsCoordinator->AddComponent<UIElement>(entity, uiElement);
            
            GN_LOG_INFO("✅ Created '" + text + "' button at (" + std::to_string(buttonX) + "," + std::to_string(buttonY) + ")");
        };
        
        // Create all buttons using consistent positioning
        createButton(m_playButtonEntity, "PLAY", 0);
        createButton(m_optionsButtonEntity, "OPTIONS", 1);
        createButton(m_quickPlayButtonEntity, "QUICK PLAY", 2);
        createButton(m_quitButtonEntity, "QUIT", 3);
        
        GN_LOG_INFO("✅ Created mobile menu buttons with simplified positioning");
    }

    void MainMenuState::DrawButtonDebugRectangles() {
        extern FloppyTurdGame* g_Game;
        if (!g_Game || !m_ecsCoordinator) {
            GN_LOG_INFO("DrawButtonDebugRectangles: No game or ECS coordinator");
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        if (!delegates.renderer.drawRectangle) {
            GN_LOG_INFO("DrawButtonDebugRectangles: No drawRectangle delegate");
            return;
        }
        
        GN_LOG_INFO("DrawButtonDebugRectangles: Drawing debug rectangles...");
        
        // Draw debug rectangles for each button
        std::vector<Gnosis::Entity> buttons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_quitButtonEntity};
        
        for (Gnosis::Entity buttonEntity : buttons) {
            if (buttonEntity == 0) continue;
            
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(buttonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(buttonEntity);
            
            if (transform && sprite) {
                // Calculate button bounds using actual scaled dimensions
                float buttonWidth = sprite->width;
                float buttonHeight = sprite->height;
                float buttonLeft = transform->position.x - (buttonWidth / 2.0f);
                float buttonTop = transform->position.y - (buttonHeight / 2.0f);
                
                GN_LOG_INFO("DrawButtonDebugRectangles: Drawing rectangle at (" + std::to_string(buttonLeft) + ", " + std::to_string(buttonTop) + ") size (" + std::to_string(buttonWidth) + ", " + std::to_string(buttonHeight) + ")");
                
                // Draw red outline rectangle
                delegates.renderer.drawRectangle(
                    buttonLeft, buttonTop, 
                    buttonWidth, buttonHeight, 
                    1.0f, 0.0f, 0.0f, 0.5f  // Red with 50% alpha
                );
            }
        }
    }

    void MainMenuState::OnPlayButtonPressed() {
        GN_LOG_INFO("Play button pressed - showing level select");
        ShowLevelSelect();
    }

    void MainMenuState::OnOptionsButtonPressed() {
        GN_LOG_INFO("Options button pressed - transitioning to options menu");
        OnMenuOptionSelected(MenuOption::OPTIONS);
    }

    void MainMenuState::OnQuickPlayButtonPressed() {
        GN_LOG_INFO("Quick Play button pressed - starting level 1");
        OnMenuOptionSelected(MenuOption::QUICK_PLAY);
    }

    void MainMenuState::OnQuitButtonPressed() {
        GN_LOG_INFO("Quit button pressed - exiting game");
        OnMenuOptionSelected(MenuOption::QUIT);
    }

    void MainMenuState::CheckMenuButtonClicks(float touchX, float touchY) {
        if (!m_ecsCoordinator || !m_assetsLoaded) {
            return;
        }
        
        // Reset all button states first
        ResetAllButtonStates();
        
        // Check Play Button
        if (m_playButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_playButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_playButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_playButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Button is now positioned at top-left, so collision detection uses top-left based bounds
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x;
                float buttonRight = transform->position.x + buttonWidth;
                float buttonTop = transform->position.y;
                float buttonBottom = transform->position.y + buttonHeight;
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 PLAY Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: PLAY BUTTON HIT!");
                    // Set button to pressed state for visual feedback
                    uiElement->isPressed = true;
                    uiElement->isHovered = true;
                    UpdateButtonSprite(m_playButtonEntity, *uiElement);
                    OnPlayButtonPressed();
                    return;
                }
            }
        }
        
        // Check Options Button
        if (m_optionsButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_optionsButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_optionsButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_optionsButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Button is now positioned at top-left, so collision detection uses top-left based bounds
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x;
                float buttonRight = transform->position.x + buttonWidth;
                float buttonTop = transform->position.y;
                float buttonBottom = transform->position.y + buttonHeight;
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 OPTIONS Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: OPTIONS BUTTON HIT!");
                    // Set button to pressed state for visual feedback
                    uiElement->isPressed = true;
                    uiElement->isHovered = true;
                    UpdateButtonSprite(m_optionsButtonEntity, *uiElement);
                    OnOptionsButtonPressed();
                    return;
                }
            }
        }
        
        // Check Quick Play Button
        if (m_quickPlayButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_quickPlayButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_quickPlayButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_quickPlayButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Button is now positioned at top-left, so collision detection uses top-left based bounds
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x;
                float buttonRight = transform->position.x + buttonWidth;
                float buttonTop = transform->position.y;
                float buttonBottom = transform->position.y + buttonHeight;
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 QUICK PLAY Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: QUICK PLAY BUTTON HIT!");
                    // Set button to pressed state for visual feedback
                    uiElement->isPressed = true;
                    uiElement->isHovered = true;
                    UpdateButtonSprite(m_quickPlayButtonEntity, *uiElement);
                    OnQuickPlayButtonPressed();
                    return;
                }
            }
        }
        
        // Check Quit Button
        if (m_quitButtonEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_quitButtonEntity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_quitButtonEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_quitButtonEntity);
            
            if (transform && sprite && uiElement) {
                // Button is now positioned at top-left, so collision detection uses top-left based bounds
                float buttonWidth = 64.0f * transform->scale.x * 0.8f; // 80% of actual button texture size
                float buttonHeight = 16.0f * transform->scale.y * 0.8f; // 80% of actual button texture size
                float buttonLeft = transform->position.x;
                float buttonRight = transform->position.x + buttonWidth;
                float buttonTop = transform->position.y;
                float buttonBottom = transform->position.y + buttonHeight;
                
                // Debug logging for button bounds
                GN_LOG_INFO("🎯 QUIT Button - Touch at (" + std::to_string(touchX) + ", " + std::to_string(touchY) + 
                           "), bounds: L=" + std::to_string(buttonLeft) + " R=" + std::to_string(buttonRight) + 
                           " T=" + std::to_string(buttonTop) + " B=" + std::to_string(buttonBottom) + 
                           " (size: " + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                           ", scale: " + std::to_string(transform->scale.x) + "x" + std::to_string(transform->scale.y) + ")");
                
                if (touchX >= buttonLeft && touchX <= buttonRight &&
                    touchY >= buttonTop && touchY <= buttonBottom) {
                    GN_LOG_INFO("🎮 MainMenuState: QUIT BUTTON HIT!");
                    // Set button to pressed state for visual feedback
                    uiElement->isPressed = true;
                    uiElement->isHovered = true;
                    UpdateButtonSprite(m_quitButtonEntity, *uiElement);
                    OnQuitButtonPressed();
                    return;
                }
            }
        }
        
        GN_LOG_INFO("❌ MainMenuState: Touch missed all buttons");
    }

    void MainMenuState::ResetAllButtonStates() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        // Reset all button states to normal
        std::vector<Gnosis::Entity> buttonEntities = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_quitButtonEntity};
        
        for (Gnosis::Entity entity : buttonEntities) {
            if (entity != 0) {
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElement) {
                    uiElement->isPressed = false;
                    uiElement->isHovered = false;
                    UpdateButtonSprite(entity, *uiElement);
                }
            }
        }
    }

    void MainMenuState::UpdateButtonSprite(Gnosis::Entity entity, const UIElement& uiElement) {
        if (!m_ecsCoordinator) {
            return;
        }
        
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        if (!sprite) {
            return;
        }
        
        // Choose the appropriate texture based on button state
        std::string textureId;
        if (uiElement.isPressed) {
            textureId = uiElement.pressedTextureId;
        } else if (uiElement.isHovered) {
            textureId = uiElement.hoverTextureId;
        } else {
            textureId = uiElement.normalTextureId;
        }
        
        // Update the sprite texture
        sprite->textureId = textureId;
        GN_LOG_DEBUG("Updated button sprite to: " + textureId);
    }



    void MainMenuState::CreateUIElements() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("CreateUIElements: ECS coordinator is null");
            return;
        }
        
        GN_LOG_INFO("Creating UI elements (buttons with integrated text)...");
        
        // The UI elements are already created in CreateDesktopLayout() and CreateMobileMenuButtons()
        // This method is now just for any additional UI setup if needed in the future
        
        GN_LOG_INFO("UI elements created successfully");
    }

    // Level Select Implementation
    void MainMenuState::InitializeLevels() {
        m_levels.clear();
        
        // Add all levels with their painting textures
        m_levels.push_back({"A Flop in the Park", "ParkLevelPainting", "LockedPainting", true, 1});      // Park is unlocked
        m_levels.push_back({"Home Sweet Home", "SewerLevelPainting", "LockedPainting", true, 2});        // Sewer UNLOCKED for testing
        m_levels.push_back({"The Good, The Bad,\nand the Stinky", "DesertLevelPainting", "LockedPainting", true, 3}); // Desert UNLOCKED for testing
        m_levels.push_back({"Polar Pandemonium", "SnowLevelPainting", "LockedPainting", true, 4});       // Snow UNLOCKED for testing
        m_levels.push_back({"Dung in the Dungeon", "CastleLevelPainting", "LockedPainting", true, 5});   // Castle UNLOCKED for testing
        m_levels.push_back({"Curtains for Crap", "RatKingPainting", "LockedPainting", true, 6});        // Boss UNLOCKED for testing
        
        m_currentLevelIndex = 0;
        GN_LOG_INFO("Initialized " + std::to_string(m_levels.size()) + " levels");
    }

    void MainMenuState::CreateLevelSelectLayout() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("CreateLevelSelectLayout: ECS coordinator is null");
            return;
        }
        
        GN_LOG_INFO("Creating level select layout...");
        
        CreateLevelPaintings();
        CreateBackButton();
        CreateLevelPlayButton();
        CreateArrowButtons();
        CreateLockedIndicator();
        
        // Initially hide all level select elements
        HideLevelSelect();
        
        GN_LOG_INFO("Level select layout created");
    }

    void MainMenuState::CreateArrowButtons() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        // Create left arrow button
        m_leftArrowButtonEntity = m_ecsCoordinator->CreateEntity();
        float buttonY = m_screenHeight / 2.0f;     // Center vertically
        float buttonScale = m_isMobile ? 10.0f : 5.0f;  // Perfect mobile button size (scale factor fix applied)
        
        // Load texture to get actual dimensions
        m_spriteSystem->LoadTexture("LeftArrow", "LeftArrow.png");
        auto leftArrowDimensions = m_spriteSystem->GetTextureDimensions("LeftArrow");
        float leftArrowWidth = leftArrowDimensions.first > 0 ? leftArrowDimensions.first : 16.0f;  // Correct size 16x16
        float leftArrowHeight = leftArrowDimensions.second > 0 ? leftArrowDimensions.second : 16.0f; // Correct size 16x16
        
        // Use positioning helper for left arrow
        auto leftButtonScaledDimensions = GetScaledDimensions(leftArrowWidth, leftArrowHeight, buttonScale);
        float leftButtonWidth = leftButtonScaledDimensions.first;
        float leftButtonHeight = leftButtonScaledDimensions.second;
        
        // Position left button with its LEFT EDGE closer to screen left edge  
        float leftButtonLeftEdge = m_screenWidth * 0.005f;  // 0.5% from left edge (closer)
        float leftButtonFinalX = leftButtonLeftEdge;  // Simple: top-left X is the left edge
        float leftButtonFinalY = buttonY - (leftButtonHeight / 2.0f);  // Center vertically around buttonY
        
        GN_LOG_INFO("⬅️ Left arrow: screenWidth=" + std::to_string(m_screenWidth) + 
                    ", leftEdge=" + std::to_string(leftButtonLeftEdge) + "(0.5%), buttonSize(" + 
                    std::to_string(leftButtonWidth) + "," + std::to_string(leftButtonHeight) + 
                    "), finalTopLeft(" + std::to_string(leftButtonFinalX) + "," + std::to_string(leftButtonFinalY) + ")");
        
        Transform leftTransform(Gnosis::GNVector2(leftButtonFinalX, leftButtonFinalY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite leftSprite("LeftArrow", leftArrowWidth, leftArrowHeight);
        leftSprite.layer = 5; // Top layer
        leftSprite.visible = false;
        UIElement leftButton("", "LeftArrow", "LeftArrowHover"); // Remove text, keep arrow sprite
        leftButton.fontSize = m_isMobile ? 116.0f : 58.0f;
        leftButton.textColor = Gnosis::GNColor(255, 255, 255, 255);
        leftButton.visible = false;
        m_ecsCoordinator->AddComponent<Transform>(m_leftArrowButtonEntity, leftTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_leftArrowButtonEntity, leftSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_leftArrowButtonEntity, leftButton);
        
        // Create right arrow button
        m_rightArrowButtonEntity = m_ecsCoordinator->CreateEntity();
        
        // Load texture to get actual dimensions
        m_spriteSystem->LoadTexture("RightArrow", "RightArrow.png");
        auto rightArrowDimensions = m_spriteSystem->GetTextureDimensions("RightArrow");
        float rightArrowWidth = rightArrowDimensions.first > 0 ? rightArrowDimensions.first : 16.0f;  // Correct size 16x16
        float rightArrowHeight = rightArrowDimensions.second > 0 ? rightArrowDimensions.second : 16.0f; // Correct size 16x16
        
        // Use positioning helper for right arrow - right edge at 2% from right screen edge
        auto rightButtonScaledDimensions = GetScaledDimensions(rightArrowWidth, rightArrowHeight, buttonScale);
        float rightButtonWidth = rightButtonScaledDimensions.first;
        float rightButtonHeight = rightButtonScaledDimensions.second;
        
        // Position right button with its RIGHT EDGE closer to screen right edge
        float rightButtonRightEdge = m_screenWidth * 0.995f;  // 99.5% from left = 0.5% from right (closer)
        float rightButtonFinalX = rightButtonRightEdge - rightButtonWidth;  // Subtract full width to get left edge
        float rightButtonFinalY = buttonY - (rightButtonHeight / 2.0f);  // Center vertically around buttonY
        
        GN_LOG_INFO("➡️ Right arrow: screenWidth=" + std::to_string(m_screenWidth) + 
                    ", rightEdge=" + std::to_string(rightButtonRightEdge) + "(99.5%), buttonSize(" + 
                    std::to_string(rightButtonWidth) + "," + std::to_string(rightButtonHeight) + 
                    "), finalTopLeft(" + std::to_string(rightButtonFinalX) + "," + std::to_string(rightButtonFinalY) + ")");
        
        Transform rightTransform(Gnosis::GNVector2(rightButtonFinalX, rightButtonFinalY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite rightSprite("RightArrow", rightArrowWidth, rightArrowHeight);
        rightSprite.layer = 5; // Top layer
        rightSprite.visible = false;
        UIElement rightButton("", "RightArrow", "RightArrowHover"); // Remove text, keep arrow sprite
        rightButton.fontSize = m_isMobile ? 116.0f : 58.0f;
        rightButton.textColor = Gnosis::GNColor(255, 255, 255, 255);
        rightButton.visible = false;
        m_ecsCoordinator->AddComponent<Transform>(m_rightArrowButtonEntity, rightTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_rightArrowButtonEntity, rightSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_rightArrowButtonEntity, rightButton);
        
        GN_LOG_INFO("Created arrow navigation buttons");
    }

    void MainMenuState::CreateLockedIndicator() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        float centerX = m_screenWidth / 2.0f;
        float centerY = m_screenHeight / 2.0f;
        float paintingScale = m_isMobile ? 6.0f : 2.0f;
        
        m_lockedIndicatorEntity = m_ecsCoordinator->CreateEntity();
        float yPos = centerY - (paintingScale * 100.0f); // Position above the painting
        
        Transform indicatorTransform(Gnosis::GNVector2(centerX, yPos), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
        UIElement indicatorElement("[Locked!]", "", "");
        indicatorElement.fontSize = m_isMobile ? 92.0f : 46.0f; // Big red text
        indicatorElement.textColor = Gnosis::GNColor(255, 0, 0, 255); // Red text
        indicatorElement.visible = false; // Initially hidden
        
        m_ecsCoordinator->AddComponent<Transform>(m_lockedIndicatorEntity, indicatorTransform);
        m_ecsCoordinator->AddComponent<UIElement>(m_lockedIndicatorEntity, indicatorElement);
        
        GN_LOG_INFO("🔒 Created single locked indicator");
    }

    void MainMenuState::CreateLevelPaintings() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        // === SIMPLIFIED LEVEL PAINTING POSITIONING === //
        
        float screenCenterX = m_screenWidth / 2.0f;
        float screenCenterY = m_screenHeight / 2.0f;
        float paintingSpacing = m_isMobile ? 400.0f : (m_screenWidth * 0.8f); // Much smaller spacing for mobile
        float paintingScale = m_isMobile ? 6.0f : 2.0f;
        
        m_levelPaintingEntities.clear();
        m_levelFrameEntities.clear();
        m_levelTextEntities.clear();
        
        GN_LOG_INFO("🎨 Creating level paintings: screenCenter=(" + std::to_string(screenCenterX) + "," + std::to_string(screenCenterY) + "), spacing=" + std::to_string(paintingSpacing) + ", scale=" + std::to_string(paintingScale));
        
        for (size_t i = 0; i < m_levels.size(); ++i) {
            // All paintings start at center position - UpdateLevelVisibility will handle positioning and visibility
            float paintingCenterX = screenCenterX;
            float paintingCenterY = screenCenterY;
            
            // === Create painting entity === //
            Gnosis::Entity paintingEntity = m_ecsCoordinator->CreateEntity();
            
            // Load texture to get actual dimensions
            m_spriteSystem->LoadTexture(m_levels[i].paintingTexture, m_levels[i].paintingTexture + ".png");
            auto paintingDimensions = m_spriteSystem->GetTextureDimensions(m_levels[i].paintingTexture);
            float paintingTextureWidth = paintingDimensions.first > 0 ? paintingDimensions.first : 96.0f;  // Correct base size
            float paintingTextureHeight = paintingDimensions.second > 0 ? paintingDimensions.second : 96.0f; // Correct base size
            
            // Use SAME dynamic scaling logic as UpdateLevelVisibility to prevent decentering
            float maxWidth = m_screenWidth * 0.8f; // Use 80% of screen width
            float maxHeight = m_screenHeight * 0.4f; // Use 40% of screen height for paintings
            
            float scaleByWidth = maxWidth / paintingTextureWidth;
            float scaleByHeight = maxHeight / paintingTextureHeight;
            float dynamicScale = std::min(scaleByWidth, scaleByHeight); // Use smaller scale to fit both dimensions
            
            auto paintingScaledDimensions = GetScaledDimensions(paintingTextureWidth, paintingTextureHeight, dynamicScale);
            float paintingScaledWidth = paintingScaledDimensions.first;
            float paintingScaledHeight = paintingScaledDimensions.second;
            
            // Use SAME manual positioning logic as UpdateLevelVisibility to prevent decentering
            float manualTopLeftX = paintingCenterX - (paintingScaledWidth / 2.0f);
            float manualTopLeftY = paintingCenterY - (paintingScaledHeight / 2.0f);
            
            GN_LOG_INFO("🔧 CreateLevelPaintings: painting " + std::to_string(i) + " using MANUAL positioning like UpdateLevelVisibility");
            GN_LOG_INFO("🔧 centerTarget=(" + std::to_string(paintingCenterX) + "," + std::to_string(paintingCenterY) + "), scaledSize=(" + std::to_string(paintingScaledWidth) + "," + std::to_string(paintingScaledHeight) + "), dynamicScale=" + std::to_string(dynamicScale));
            GN_LOG_INFO("🔧 manualTopLeft=(" + std::to_string(manualTopLeftX) + "," + std::to_string(manualTopLeftY) + ")");
            
            Transform paintingTransform(Gnosis::GNVector2(manualTopLeftX, manualTopLeftY), 0.0f, Gnosis::GNVector2(dynamicScale, dynamicScale));
            Sprite paintingSprite(m_levels[i].paintingTexture, paintingTextureWidth, paintingTextureHeight);
            paintingSprite.layer = 3; // Above background, below UI
            paintingSprite.visible = false; // Initially hidden
            m_ecsCoordinator->AddComponent<Transform>(paintingEntity, paintingTransform);
            m_ecsCoordinator->AddComponent<Sprite>(paintingEntity, paintingSprite);
            m_levelPaintingEntities.push_back(paintingEntity);
            
            GN_LOG_INFO("✅ Created painting " + std::to_string(i) + " '" + m_levels[i].name + "': centerPos=(" + std::to_string(paintingCenterX) + "," + std::to_string(paintingCenterY) + "), topLeft=(" + std::to_string(manualTopLeftX) + "," + std::to_string(manualTopLeftY) + "), size=" + std::to_string(paintingScaledWidth) + "x" + std::to_string(paintingScaledHeight));
            
            // === Create frame entity for locked levels === //
            Gnosis::Entity frameEntity = m_ecsCoordinator->CreateEntity();
            
            // For perfect overlay, the locked frame should use EXACTLY the same dimensions and position as the painting
            // This ensures perfect synchronization between locked and unlocked paintings
            
            Transform frameTransform(Gnosis::GNVector2(manualTopLeftX, manualTopLeftY), 0.0f, Gnosis::GNVector2(dynamicScale, dynamicScale));
            std::string frameTexture = m_levels[i].isUnlocked ? "" : "LockedPainting";
            Sprite frameSprite(frameTexture, paintingTextureWidth, paintingTextureHeight); // Use SAME dimensions as painting
            frameSprite.layer = 4; // Above paintings
            frameSprite.visible = false;
            m_ecsCoordinator->AddComponent<Transform>(frameEntity, frameTransform);
            m_ecsCoordinator->AddComponent<Sprite>(frameEntity, frameSprite);
            m_levelFrameEntities.push_back(frameEntity);
            
            // === Create level name text entity === //
            Gnosis::Entity textEntity = m_ecsCoordinator->CreateEntity();
            
            // Position text at top of screen, centered horizontally on screen (not painting)
            float textY = m_screenHeight * 0.15f; // 15% down from top
            float textX = screenCenterX; // Use screen center for proper horizontal centering
            
            Transform textTransform(Gnosis::GNVector2(textX, textY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
            UIElement textElement(m_levels[i].name, "", "");
            textElement.fontSize = m_isMobile ? 64.0f : 32.0f;
            textElement.textColor = Gnosis::GNColor(255, 255, 255, 255);
            textElement.centerTextHorizontally = true;
            textElement.centerTextVertically = true;
            textElement.visible = false;
            m_ecsCoordinator->AddComponent<Transform>(textEntity, textTransform);
            m_ecsCoordinator->AddComponent<UIElement>(textEntity, textElement);
            m_levelTextEntities.push_back(textEntity);
        }
        
        GN_LOG_INFO("✅ Created %zu level paintings with simplified centered positioning", m_levels.size());
    }

    void MainMenuState::CreateBackButton() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        m_backButtonEntity = m_ecsCoordinator->CreateEntity();
        float buttonX = m_screenWidth / 2.0f;  // Center horizontally
        float buttonY = m_screenHeight * 0.9f;  // 90% from top (near bottom)
        float buttonScale = m_isMobile ? 10.0f : 5.0f;
        
        // Load texture to get actual dimensions
        m_spriteSystem->LoadTexture("FloppyButtonBlue", "FloppyButtonBlue.png");
        auto buttonDimensions = m_spriteSystem->GetTextureDimensions("FloppyButtonBlue");
        float buttonTextureWidth = buttonDimensions.first > 0 ? buttonDimensions.first : 90.0f;
        float buttonTextureHeight = buttonDimensions.second > 0 ? buttonDimensions.second : 16.0f;
        
        // Use positioning helper for back button
        auto backButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float backButtonWidth = backButtonScaledDimensions.first;
        float backButtonHeight = backButtonScaledDimensions.second;
        
        Gnosis::GNVector2 backButtonPosition = CenterObjectAtPosition(buttonX, buttonY, backButtonWidth, backButtonHeight);
        float backButtonTopLeftX = backButtonPosition.x;
        float backButtonTopLeftY = backButtonPosition.y;
        
        Transform backTransform(Gnosis::GNVector2(backButtonTopLeftX, backButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite backSprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight);
        backSprite.layer = 5; // Top layer
        backSprite.visible = false;
        UIElement backButton("BACK", "FloppyButtonBlue", "FloppyButtonBlueHover");
        backButton.fontSize = m_isMobile ? 80.0f : 40.0f;
        backButton.textColor = Gnosis::GNColor(255, 255, 255, 255);
        backButton.visible = false;
        
        m_ecsCoordinator->AddComponent<Transform>(m_backButtonEntity, backTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_backButtonEntity, backSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_backButtonEntity, backButton);
        
        GN_LOG_INFO("Created back button");
    }

    void MainMenuState::CreateLevelPlayButton() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        m_levelPlayButtonEntity = m_ecsCoordinator->CreateEntity();
        float buttonX = m_screenWidth / 2.0f;
        float buttonY = m_screenHeight * 0.8f;
        float buttonScale = m_isMobile ? 10.0f : 5.0f;
        
        // Load texture to get actual dimensions
        m_spriteSystem->LoadTexture("FloppyButtonBlue", "FloppyButtonBlue.png");
        auto buttonDimensions = m_spriteSystem->GetTextureDimensions("FloppyButtonBlue");
        float buttonTextureWidth = buttonDimensions.first > 0 ? buttonDimensions.first : 90.0f;
        float buttonTextureHeight = buttonDimensions.second > 0 ? buttonDimensions.second : 16.0f;
        
        // Use positioning helper for level play button
        auto playButtonScaledDimensions = GetScaledDimensions(buttonTextureWidth, buttonTextureHeight, buttonScale);
        float playButtonWidth = playButtonScaledDimensions.first;
        float playButtonHeight = playButtonScaledDimensions.second;
        
        Gnosis::GNVector2 playButtonPosition = CenterObjectAtPosition(buttonX, buttonY, playButtonWidth, playButtonHeight);
        float playButtonTopLeftX = playButtonPosition.x;
        float playButtonTopLeftY = playButtonPosition.y;
        
        Transform playTransform(Gnosis::GNVector2(playButtonTopLeftX, playButtonTopLeftY), 0.0f, Gnosis::GNVector2(buttonScale, buttonScale));
        Sprite playSprite("FloppyButtonBlue", buttonTextureWidth, buttonTextureHeight);
        playSprite.layer = 5; // Top layer
        playSprite.visible = false;
        UIElement playButton("PLAY LEVEL", "FloppyButtonBlue", "FloppyButtonBlueHover");
        playButton.fontSize = m_isMobile ? 68.0f : 34.0f; // Increased slightly for better readability
        playButton.textColor = Gnosis::GNColor(255, 255, 255, 255);
        playButton.visible = false;
        
        m_ecsCoordinator->AddComponent<Transform>(m_levelPlayButtonEntity, playTransform);
        m_ecsCoordinator->AddComponent<Sprite>(m_levelPlayButtonEntity, playSprite);
        m_ecsCoordinator->AddComponent<UIElement>(m_levelPlayButtonEntity, playButton);
        
        GN_LOG_INFO("Created level play button");
    }

    void MainMenuState::ShowLevelSelect() {
        GN_LOG_INFO("Showing level select menu");
        m_currentMode = MenuMode::LEVEL_SELECT;
        
        // Hide main menu elements
        if (m_logoEntity != 0) {
            Sprite* logoSprite = m_ecsCoordinator->GetComponent<Sprite>(m_logoEntity);
            if (logoSprite) logoSprite->visible = false;
        }
        if (m_fButtonEntity != 0) {
            Sprite* fSprite = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity);
            if (fSprite) fSprite->visible = false;
        }
        
        // Hide main menu buttons
        std::vector<Gnosis::Entity> mainButtons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_quitButtonEntity};
        for (Gnosis::Entity entity : mainButtons) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (sprite) sprite->visible = false;
                if (uiElement) uiElement->visible = false;
            }
        }
        
        // Show level select elements
        UpdateLevelVisibility();
        
        if (m_backButtonEntity != 0) {
            Sprite* backSprite = m_ecsCoordinator->GetComponent<Sprite>(m_backButtonEntity);
            UIElement* backElement = m_ecsCoordinator->GetComponent<UIElement>(m_backButtonEntity);
            if (backSprite) backSprite->visible = true;
            if (backElement) backElement->visible = true;
        }
        
        if (m_levelPlayButtonEntity != 0) {
            Sprite* playSprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPlayButtonEntity);
            UIElement* playElement = m_ecsCoordinator->GetComponent<UIElement>(m_levelPlayButtonEntity);
            if (playSprite) playSprite->visible = true;
            if (playElement) playElement->visible = true;
        }
        
        // Show arrow buttons
        if (m_leftArrowButtonEntity != 0) {
            Sprite* leftSprite = m_ecsCoordinator->GetComponent<Sprite>(m_leftArrowButtonEntity);
            UIElement* leftElement = m_ecsCoordinator->GetComponent<UIElement>(m_leftArrowButtonEntity);
            if (leftSprite) leftSprite->visible = true;
            if (leftElement) leftElement->visible = true;
        }
        
        if (m_rightArrowButtonEntity != 0) {
            Sprite* rightSprite = m_ecsCoordinator->GetComponent<Sprite>(m_rightArrowButtonEntity);
            UIElement* rightElement = m_ecsCoordinator->GetComponent<UIElement>(m_rightArrowButtonEntity);
            if (rightSprite) rightSprite->visible = true;
            if (rightElement) rightElement->visible = true;
        }
    }

    void MainMenuState::HideLevelSelect() {
        GN_LOG_INFO("Hiding level select menu");
        m_currentMode = MenuMode::MAIN_MENU;
        
        // Show main menu elements
        if (m_logoEntity != 0) {
            Sprite* logoSprite = m_ecsCoordinator->GetComponent<Sprite>(m_logoEntity);
            if (logoSprite) logoSprite->visible = true;
        }
        if (m_fButtonEntity != 0) {
            Sprite* fSprite = m_ecsCoordinator->GetComponent<Sprite>(m_fButtonEntity);
            if (fSprite) fSprite->visible = true;
        }
        
        // Show main menu buttons
        std::vector<Gnosis::Entity> mainButtons = {m_playButtonEntity, m_optionsButtonEntity, m_quickPlayButtonEntity, m_quitButtonEntity};
        for (Gnosis::Entity entity : mainButtons) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (sprite) sprite->visible = true;
                if (uiElement) uiElement->visible = true;
            }
        }
        
        // Hide level select elements
        for (Gnosis::Entity entity : m_levelPaintingEntities) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                if (sprite) sprite->visible = false;
            }
        }
        
        for (Gnosis::Entity entity : m_levelFrameEntities) {
            if (entity != 0) {
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
                if (sprite) sprite->visible = false;
            }
        }
        
        for (Gnosis::Entity entity : m_levelTextEntities) {
            if (entity != 0) {
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
                if (uiElement) uiElement->visible = false;
            }
        }
        
        if (m_backButtonEntity != 0) {
            Sprite* backSprite = m_ecsCoordinator->GetComponent<Sprite>(m_backButtonEntity);
            UIElement* backElement = m_ecsCoordinator->GetComponent<UIElement>(m_backButtonEntity);
            if (backSprite) backSprite->visible = false;
            if (backElement) backElement->visible = false;
        }
        
        if (m_levelPlayButtonEntity != 0) {
            Sprite* playSprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPlayButtonEntity);
            UIElement* playElement = m_ecsCoordinator->GetComponent<UIElement>(m_levelPlayButtonEntity);
            if (playSprite) playSprite->visible = false;
            if (playElement) playElement->visible = false;
        }
        
        // Hide arrow buttons
        if (m_leftArrowButtonEntity != 0) {
            Sprite* leftSprite = m_ecsCoordinator->GetComponent<Sprite>(m_leftArrowButtonEntity);
            UIElement* leftElement = m_ecsCoordinator->GetComponent<UIElement>(m_leftArrowButtonEntity);
            if (leftSprite) leftSprite->visible = false;
            if (leftElement) leftElement->visible = false;
        }
        
        if (m_rightArrowButtonEntity != 0) {
            Sprite* rightSprite = m_ecsCoordinator->GetComponent<Sprite>(m_rightArrowButtonEntity);
            UIElement* rightElement = m_ecsCoordinator->GetComponent<UIElement>(m_rightArrowButtonEntity);
            if (rightSprite) rightSprite->visible = false;
            if (rightElement) rightElement->visible = false;
        }
        
        // Hide locked indicator to prevent it showing on main menu
        if (m_lockedIndicatorEntity != 0) {
            UIElement* lockedElement = m_ecsCoordinator->GetComponent<UIElement>(m_lockedIndicatorEntity);
            if (lockedElement) lockedElement->visible = false;
        }
    }

    void MainMenuState::HandleLevelSelectInput() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        extern FloppyTurdGame* g_Game;
        if (!g_Game) {
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        
        // Handle swipe input
        HandleSwipeInput();
        
        // Handle button clicks
        bool inputPressed = delegates.input.isPrimaryInputJustPressed();
        if (inputPressed) {
            float touchX, touchY;
            if (delegates.input.getPrimaryInputPosition) {
                delegates.input.getPrimaryInputPosition(&touchX, &touchY);
                
                // Check back button
                if (m_backButtonEntity != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_backButtonEntity);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_backButtonEntity);
                    
                    if (transform && sprite) {
                        // Button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x;
                        float buttonRight = transform->position.x + buttonWidth;
                        float buttonTop = transform->position.y;
                        float buttonBottom = transform->position.y + buttonHeight;
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnBackButtonPressed();
                            return;
                        }
                    }
                }
                
                // Check play button
                if (m_levelPlayButtonEntity != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPlayButtonEntity);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPlayButtonEntity);
                    
                    if (transform && sprite) {
                        // Button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x;
                        float buttonRight = transform->position.x + buttonWidth;
                        float buttonTop = transform->position.y;
                        float buttonBottom = transform->position.y + buttonHeight;
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnLevelPlayButtonPressed();
                            return;
                        }
                    }
                }
                
                // Check left arrow button
                if (m_leftArrowButtonEntity != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_leftArrowButtonEntity);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_leftArrowButtonEntity);
                    
                    if (transform && sprite) {
                        // Button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x;
                        float buttonRight = transform->position.x + buttonWidth;
                        float buttonTop = transform->position.y;
                        float buttonBottom = transform->position.y + buttonHeight;
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnLeftArrowPressed();
                            return;
                        }
                    }
                }
                
                // Check right arrow button
                if (m_rightArrowButtonEntity != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_rightArrowButtonEntity);
                    Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_rightArrowButtonEntity);
                    
                    if (transform && sprite) {
                        // Button is now positioned at top-left, so collision detection uses top-left based bounds
                        float buttonWidth = sprite->width * transform->scale.x;
                        float buttonHeight = sprite->height * transform->scale.y;
                        float buttonLeft = transform->position.x;
                        float buttonRight = transform->position.x + buttonWidth;
                        float buttonTop = transform->position.y;
                        float buttonBottom = transform->position.y + buttonHeight;
                        
                        if (touchX >= buttonLeft && touchX <= buttonRight &&
                            touchY >= buttonTop && touchY <= buttonBottom) {
                            OnRightArrowPressed();
                            return;
                        }
                    }
                }
            }
        }
    }

    void MainMenuState::HandleSwipeInput() {
        extern FloppyTurdGame* g_Game;
        if (!g_Game) {
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        
        // Use the new gesture detection methods
        bool swipeLeft = delegates.input.isSwipeLeftDetected ? delegates.input.isSwipeLeftDetected() : false;
        bool swipeRight = delegates.input.isSwipeRightDetected ? delegates.input.isSwipeRightDetected() : false;
        
        GN_LOG_DEBUG("Gesture Input: Left=" + std::to_string(swipeLeft) + 
                    " Right=" + std::to_string(swipeRight));
        
        if (swipeLeft && m_currentLevelIndex < m_levels.size() - 1) {
            // Swipe left - go to next level
            m_currentLevelIndex++;
            GN_LOG_INFO("Gesture detected: Swipe left - moved to level " + std::to_string(m_currentLevelIndex + 1));
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
        } else if (swipeRight && m_currentLevelIndex > 0) {
            // Swipe right - go to previous level
            m_currentLevelIndex--;
            GN_LOG_INFO("Gesture detected: Swipe right - moved to level " + std::to_string(m_currentLevelIndex + 1));
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
        }
        
        // Reset gesture state after processing
        if (delegates.input.resetGestureState) {
            delegates.input.resetGestureState();
        }
    }

    void MainMenuState::StartSwipe(float startX, float startY) {
        m_swipeStartX = startX;
        m_swipeStartY = startY;
        m_isSwiping = true;
        m_swipeDirection = SwipeDirection::NONE;
        GN_LOG_INFO("Swipe started at (" + std::to_string(startX) + ", " + std::to_string(startY) + ")");
    }

    void MainMenuState::UpdateSwipe(float currentX, float currentY) {
        if (!m_isSwiping) return;
        
        m_swipeEndX = currentX;
        m_swipeEndY = currentY;
        
        // Determine swipe direction
        float deltaX = currentX - m_swipeStartX;
        float deltaY = currentY - m_swipeStartY;
        
        if (std::abs(deltaX) > std::abs(deltaY) && std::abs(deltaX) > 20.0f) {
            m_swipeDirection = deltaX > 0 ? SwipeDirection::RIGHT : SwipeDirection::LEFT;
        }
    }

    void MainMenuState::EndSwipe(float endX, float endY) {
        if (!m_isSwiping) return;
        
        m_swipeEndX = endX;
        m_swipeEndY = endY;
        m_isSwiping = false;
        
        ProcessSwipe();
        GN_LOG_INFO("Swipe ended at (" + std::to_string(endX) + ", " + std::to_string(endY) + ")");
    }

    void MainMenuState::ProcessSwipe() {
        float deltaX = m_swipeEndX - m_swipeStartX;
        
        GN_LOG_INFO("Processing swipe: deltaX=" + std::to_string(deltaX) + 
                   " threshold=" + std::to_string(m_swipeThreshold) + 
                   " direction=" + std::to_string(static_cast<int>(m_swipeDirection)));
        
        if (std::abs(deltaX) > m_swipeThreshold) {
            if (m_swipeDirection == SwipeDirection::LEFT && m_currentLevelIndex < m_levels.size() - 1) {
                // Swipe left - go to next level
                m_currentLevelIndex++;
                GN_LOG_INFO("Swiped left - moved to level " + std::to_string(m_currentLevelIndex + 1));
            } else if (m_swipeDirection == SwipeDirection::RIGHT && m_currentLevelIndex > 0) {
                // Swipe right - go to previous level
                m_currentLevelIndex--;
                GN_LOG_INFO("Swiped right - moved to level " + std::to_string(m_currentLevelIndex + 1));
            }
            
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
        } else {
            GN_LOG_INFO("Swipe distance too small, ignoring");
        }
    }

    void MainMenuState::AnimateSwipe(float deltaTime) {
        if (m_swipeAnimationTimer < m_swipeAnimationDuration) {
            m_swipeAnimationTimer += deltaTime;
            float progress = m_swipeAnimationTimer / m_swipeAnimationDuration;
            
            // Smooth easing
            float easedProgress = 1.0f - (1.0f - progress) * (1.0f - progress);
            
            // Update painting positions using SAME logic as UpdateLevelVisibility for consistency
            float centerX = m_screenWidth / 2.0f;
            float centerY = m_screenHeight / 2.0f;
            
            GN_LOG_DEBUG("🎬 AnimateSwipe: progress=" + std::to_string(progress) + ", centerPos=(" + std::to_string(centerX) + "," + std::to_string(centerY) + ")");
            
            for (size_t i = 0; i < m_levelPaintingEntities.size(); ++i) {
                if (m_levelPaintingEntities[i] != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                    if (transform && i < m_levels.size()) {
                        // Use SAME dynamic scaling logic as UpdateLevelVisibility to prevent decentering
                        auto paintingDimensions = m_spriteSystem->GetTextureDimensions(m_levels[i].paintingTexture);
                        float paintingTextureWidth = paintingDimensions.first > 0 ? paintingDimensions.first : 96.0f;  // Correct base size
                        float paintingTextureHeight = paintingDimensions.second > 0 ? paintingDimensions.second : 96.0f; // Correct base size
                        
                        // Use SAME dynamic scaling calculation as UpdateLevelVisibility
                        float maxWidth = m_screenWidth * 0.8f; // Use 80% of screen width
                        float maxHeight = m_screenHeight * 0.4f; // Use 40% of screen height for paintings
                        
                        float scaleByWidth = maxWidth / paintingTextureWidth;
                        float scaleByHeight = maxHeight / paintingTextureHeight;
                        float dynamicScale = std::min(scaleByWidth, scaleByHeight); // Use smaller scale to fit both dimensions
                        
                        auto scaledDimensions = GetScaledDimensions(paintingTextureWidth, paintingTextureHeight, dynamicScale);
                        float paintingWidth = scaledDimensions.first;
                        float paintingHeight = scaledDimensions.second;
                        
                        // For the current level (i == m_currentLevelIndex), position at screen center
                        // For other levels, they should be hidden anyway by UpdateLevelVisibility
                        if (i == m_currentLevelIndex) {
                            // Use SAME manual positioning logic as UpdateLevelVisibility to prevent decentering
                            float manualTopLeftX = centerX - (paintingWidth / 2.0f);
                            float manualTopLeftY = centerY - (paintingHeight / 2.0f);
                            
                            transform->position.x = manualTopLeftX;
                            transform->position.y = manualTopLeftY;
                            transform->scale.x = dynamicScale;
                            transform->scale.y = dynamicScale;
                            
                            GN_LOG_DEBUG("🎬 AnimateSwipe: painting " + std::to_string(i) + " positioned at (" + std::to_string(manualTopLeftX) + "," + std::to_string(manualTopLeftY) + ") with scale=" + std::to_string(dynamicScale));
                        }
                    }
                }
                
                if (m_levelFrameEntities[i] != 0) {
                    Transform* frameTransform = m_ecsCoordinator->GetComponent<Transform>(m_levelFrameEntities[i]);
                    if (frameTransform && m_levelPaintingEntities[i] != 0) {
                        // For perfect sync during animation, copy painting's position exactly
                        Transform* paintingTransform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                        if (paintingTransform) {
                            frameTransform->position.x = paintingTransform->position.x;
                            frameTransform->position.y = paintingTransform->position.y;
                            frameTransform->scale = paintingTransform->scale;
                        }
                    }
                }
                
                if (m_levelTextEntities[i] != 0) {
                    Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelTextEntities[i]);
                    if (transform && i == m_currentLevelIndex) {
                        // Text stays centered horizontally at screen center for current level
                        transform->position.x = centerX;
                        transform->position.y = m_screenHeight * 0.15f; // Keep at top 15% of screen
                    }
                }
            }
        }
    }

    void MainMenuState::UpdateLevelVisibility() {
        float centerX = m_screenWidth / 2.0f;
        float centerY = m_screenHeight / 2.0f;
        
        GN_LOG_INFO("🎨 UpdateLevelVisibility: SCREEN=" + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight) + 
                   ", SCREEN_CENTER=(" + std::to_string(centerX) + "," + std::to_string(centerY) + "), currentLevel=" + std::to_string(m_currentLevelIndex));
        
        for (size_t i = 0; i < m_levelPaintingEntities.size(); ++i) {
            // Only show the current painting - hide all others
            bool shouldBeVisible = (i == m_currentLevelIndex);
            
            GN_LOG_INFO("🖼️ Painting " + std::to_string(i) + " (" + m_levels[i].name + "): shouldBeVisible=" + std::string(shouldBeVisible ? "YES" : "NO"));
            
            // Update painting position, visibility and texture
            if (m_levelPaintingEntities[i] != 0) {
                Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelPaintingEntities[i]);
                if (transform && sprite) {
                    // Update visibility first
                    sprite->visible = shouldBeVisible;
                    
                    // Only position and update texture for the visible painting
                    if (shouldBeVisible && i < m_levels.size()) {
                        // Get actual painting texture dimensions first
                        auto paintingDimensions = m_spriteSystem->GetTextureDimensions(m_levels[i].paintingTexture);
                        float paintingTextureWidth = paintingDimensions.first > 0 ? paintingDimensions.first : 96.0f;  // Correct base size
                        float paintingTextureHeight = paintingDimensions.second > 0 ? paintingDimensions.second : 96.0f; // Correct base size
                        
                        GN_LOG_INFO("🔍 Texture dimensions for " + m_levels[i].paintingTexture + ": returned=(" + 
                                   std::to_string(paintingDimensions.first) + "," + std::to_string(paintingDimensions.second) + 
                                   "), using=(" + std::to_string(paintingTextureWidth) + "," + std::to_string(paintingTextureHeight) + ")");
                        
                        // Calculate scale that fits screen width with 10% padding on each side
                        float maxWidth = m_screenWidth * 0.8f; // Use 80% of screen width
                        float maxHeight = m_screenHeight * 0.4f; // Use 40% of screen height for paintings
                        
                        float scaleByWidth = maxWidth / paintingTextureWidth;
                        float scaleByHeight = maxHeight / paintingTextureHeight;
                        float dynamicScale = std::min(scaleByWidth, scaleByHeight); // Use smaller scale to fit both dimensions
                        
                        auto scaledDimensions = GetScaledDimensions(paintingTextureWidth, paintingTextureHeight, dynamicScale);
                        float paintingWidth = scaledDimensions.first;
                        float paintingHeight = scaledDimensions.second;
                        
                        GN_LOG_INFO("📐 Painting " + std::to_string(i) + " calculations: textureSize(" + 
                                   std::to_string(paintingTextureWidth) + "," + std::to_string(paintingTextureHeight) + 
                                   ") -> scaledSize(" + std::to_string(paintingWidth) + "," + std::to_string(paintingHeight) + 
                                   ") at centerTarget(" + std::to_string(centerX) + "," + std::to_string(centerY) + ")");
                        
                        // MANUAL POSITIONING - bypass CenterObjectAtPosition to test
                        GN_LOG_INFO("🧪 TESTING: Manual painting positioning without helper functions");
                        GN_LOG_INFO("🧪 Screen center: (" + std::to_string(centerX) + "," + std::to_string(centerY) + ")");
                        GN_LOG_INFO("🧪 Painting scaled size: (" + std::to_string(paintingWidth) + "," + std::to_string(paintingHeight) + ")");
                        
                        // Calculate top-left position manually to center the painting
                        float manualTopLeftX = centerX - (paintingWidth / 2.0f);
                        float manualTopLeftY = centerY - (paintingHeight / 2.0f);
                        
                        GN_LOG_INFO("🧪 Manual calculation: topLeft=(" + std::to_string(manualTopLeftX) + "," + std::to_string(manualTopLeftY) + ") should center painting at (" + std::to_string(centerX) + "," + std::to_string(centerY) + ")");
                        
                        // Set position directly without helper function
                        transform->position.x = manualTopLeftX;
                        transform->position.y = manualTopLeftY;
                        
                        // Also update the transform scale to use the dynamic scale
                        transform->scale.x = dynamicScale;
                        transform->scale.y = dynamicScale;
                        
                        GN_LOG_INFO("🧪 Final transform position set to: (" + std::to_string(transform->position.x) + "," + std::to_string(transform->position.y) + ") with scale: " + std::to_string(dynamicScale));
                        
                        // Update the texture to show the correct level painting
                        sprite->textureId = m_levels[i].paintingTexture;
                    }
                }
            }
            
            // Update frame position and visibility
            if (m_levelFrameEntities[i] != 0) {
                Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelFrameEntities[i]);
                Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(m_levelFrameEntities[i]);
                if (transform && sprite) {
                    // Update visibility first
                    sprite->visible = shouldBeVisible && !m_levels[i].isUnlocked;
                    
                    // Only position if visible
                    if (shouldBeVisible && m_levelPaintingEntities[i] != 0) {
                        Transform* paintingTransform = m_ecsCoordinator->GetComponent<Transform>(m_levelPaintingEntities[i]);
                        if (paintingTransform) {
                            // Use EXACT same position and scale as the underlying painting
                            transform->position.x = paintingTransform->position.x;
                            transform->position.y = paintingTransform->position.y;
                            transform->scale = paintingTransform->scale;
                        }
                    }
                }
            }
            
            // Update text position and visibility
            if (m_levelTextEntities[i] != 0) {
                Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_levelTextEntities[i]);
                UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_levelTextEntities[i]);
                if (transform && uiElement) {
                    // Update visibility first
                    uiElement->visible = shouldBeVisible;
                    
                    // Only position and update text for visible level
                    if (shouldBeVisible) {
                        // Center the text horizontally at the painting position
                        transform->position.x = centerX;
                        transform->position.y = m_screenHeight * 0.15f; // Keep at top 15% of screen
                        
                        // Update the text to show the correct level name
                        if (i < m_levels.size()) {
                            uiElement->buttonText = m_levels[i].name;
                        }
                    }
                }
            }
        }
        
        // Update locked indicator visibility and position
        if (m_lockedIndicatorEntity != 0) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(m_lockedIndicatorEntity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_lockedIndicatorEntity);
            if (transform && uiElement) {
                // Position above the current level's painting
                transform->position.x = centerX;
                float paintingScale = m_isMobile ? 6.0f : 2.0f;
                transform->position.y = centerY - (paintingScale * 100.0f);
                
                // Show indicator only if current level is locked
                uiElement->visible = !m_levels[m_currentLevelIndex].isUnlocked;
            }
        }
        
        GN_LOG_INFO("Updated level visibility for current level: " + std::to_string(m_currentLevelIndex + 1));
    }

    void MainMenuState::CenterCurrentLevel() {
        UpdateLevelVisibility();
    }

    void MainMenuState::OnBackButtonPressed() {
        GN_LOG_INFO("Back button pressed - returning to main menu");
        HideLevelSelect();
    }

    void MainMenuState::OnLevelPlayButtonPressed() {
        GN_LOG_INFO("Level play button pressed - starting level " + std::to_string(m_currentLevelIndex + 1));
        OnLevelSelected(m_currentLevelIndex);
    }

    void MainMenuState::OnLeftArrowPressed() {
        // Check debounce timer to prevent rapid clicking
        if (m_lastArrowPressTime < m_arrowDebounceDelay) {
            return; // Too soon since last press, ignore
        }
        
        if (m_currentLevelIndex > 0) {
            m_currentLevelIndex--;
            GN_LOG_INFO("Left arrow pressed - moved to level " + std::to_string(m_currentLevelIndex + 1));
            
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
            
            // Reset debounce timer
            m_lastArrowPressTime = 0.0f;
        }
    }

    void MainMenuState::OnRightArrowPressed() {
        // Check debounce timer to prevent rapid clicking
        if (m_lastArrowPressTime < m_arrowDebounceDelay) {
            return; // Too soon since last press, ignore
        }
        
        if (m_currentLevelIndex < m_levels.size() - 1) {
            m_currentLevelIndex++;
            GN_LOG_INFO("Right arrow pressed - moved to level " + std::to_string(m_currentLevelIndex + 1));
            
            // Use UpdateLevelVisibility directly instead of animation to prevent decentering
            UpdateLevelVisibility();
            
            // Reset debounce timer
            m_lastArrowPressTime = 0.0f;
        }
    }

    void MainMenuState::OnLevelSelected(int levelIndex) {
        if (levelIndex >= 0 && levelIndex < m_levels.size()) {
            GN_LOG_INFO("Level selected: " + m_levels[levelIndex].name + " (Level " + std::to_string(m_levels[levelIndex].levelNumber) + ")");
            
            // Check if level is unlocked
            if (!m_levels[levelIndex].isUnlocked) {
                GN_LOG_INFO("Level is locked - cannot start");
                return;
            }
            
            // Store the selected level NUMBER (1-6) for the transition, not the index (0-5)
            m_selectedLevelIndex = m_levels[levelIndex].levelNumber;
            m_finished = true;
        }
    }

    void MainMenuState::DrawLevelSelectDebugInfo() {
        extern FloppyTurdGame* g_Game;
        if (!g_Game || !m_ecsCoordinator) {
            return;
        }
        
        const PlatformDelegates& delegates = g_Game->GetPlatformDelegates();
        if (!delegates.renderer.drawRectangle) {
            return;
        }
        
        // Draw debug info for level select elements
        GN_LOG_INFO("Level Select Debug: Current level " + std::to_string(m_currentLevelIndex + 1) + 
                   " of " + std::to_string(m_levels.size()) + 
                   ", Mode: " + (m_currentMode == MenuMode::LEVEL_SELECT ? "LEVEL_SELECT" : "MAIN_MENU"));
    }
    
    // Platform-specific layout implementation
    void MainMenuState::SetupLayout() {
#ifdef PLATFORM_IOS
        SetupIOSLayout();
#else
        SetupDesktopLayout();
#endif
    }
    
    void MainMenuState::SetupIOSLayout() {
        GN_LOG_INFO("MainMenuState: Setting up iOS layout");
        
        // Calculate dynamic scaling for iOS based on screen size
        float scaleX = m_screenWidth / 393.0f;   // iPhone 16 logical width reference
        float scaleY = m_screenHeight / 852.0f;  // iPhone 16 logical height reference
        float dynamicScale = std::min(scaleX, scaleY);
        
        GN_LOG_INFO("MainMenuState: iOS layout - Screen: " + 
                   std::to_string((int)m_screenWidth) + "x" + 
                   std::to_string((int)m_screenHeight) + 
                   ", Scale: " + std::to_string(dynamicScale));
        
        // iOS-specific main menu layout adjustments can go here
        // e.g., adjusting button positions, logo placement, safe area handling, etc.
    }
    
    void MainMenuState::SetupDesktopLayout() {
        GN_LOG_INFO("MainMenuState: Setting up desktop layout");
        
        // Calculate dynamic scaling for desktop
        float scaleX = m_screenWidth / 800.0f;   // Desktop reference width
        float scaleY = m_screenHeight / 600.0f;  // Desktop reference height
        float dynamicScale = std::min(scaleX, scaleY);
        
        GN_LOG_INFO("MainMenuState: Desktop layout - Screen: " + 
                   std::to_string((int)m_screenWidth) + "x" + 
                   std::to_string((int)m_screenHeight) + 
                   ", Scale: " + std::to_string(dynamicScale));
        
        // Desktop-specific main menu layout adjustments can go here
    }

} // namespace GameCore