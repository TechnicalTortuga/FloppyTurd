#include "CreditsState.h"
#include "../Components/GameComponents.h"
#include "../Systems/RenderSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <random>
#include <cmath>
#include <algorithm>

namespace GameCore {

CreditsState::CreditsState(Gnosis::ECS* ecsSystem, PlatformDelegates* platformDelegates)
    : m_ecsSystem(ecsSystem)
    , m_platformDelegates(platformDelegates)
    , m_finished(false)
    , m_shouldSkip(false)
    , m_musicStarted(false)
    , m_elapsedTime(0.0f)
    , m_totalMusicDuration(0.0f)
    , m_turdBounceTimer(0.0f)
    , m_textScrollOffset(0.0f)
    , m_whiteFadeAlpha(1.0f)  // Start fully white
    , m_fadingOut(false)  // Not fading out yet
    , m_screenWidth(0.0f)
    , m_screenHeight(0.0f)
    , m_backgroundEntity(0)
    , m_turdEntity(0)
    , m_skipButtonEntity(0)
    , m_whiteFadeEntity(0)
    , m_finalHoldActive(false)
    , m_finalHoldTimer(0.0f)
    , m_finalTitleEntity(0)
    , m_finalNameEntity(0)
    , m_enforceFinalExitOnly(ENFORCE_FINAL_SEQUENCE_EXIT_ONLY)
    , m_finalEntryIndex(0)
{
    GN_LOG_INFO("CreditsState created");
}

CreditsState::~CreditsState() {
    GN_LOG_INFO("CreditsState destroyed");
}

void CreditsState::Enter() {
    GN_LOG_INFO("CreditsState::Enter - getting screen dimensions from RenderSystem");
    
    // Get actual screen dimensions from RenderSystem (like GameplayState does)
    RenderSystem* renderSystem = nullptr;
    if (m_ecsSystem && m_ecsSystem->GetSystemManager()) {
        renderSystem = m_ecsSystem->GetSystemManager()->GetRenderSystem();
    }
    
    if (renderSystem) {
        const ScreenInfo& screenInfo = renderSystem->GetScreenInfo();
        m_screenWidth = screenInfo.pixelWidth;
        m_screenHeight = screenInfo.pixelHeight;
        GN_LOG_INFO("CreditsState: Screen dimensions from RenderSystem: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
    } else {
        GN_LOG_ERROR("CreditsState: Cannot get RenderSystem! Using fallback dimensions");
        m_screenWidth = 2556.0f;
        m_screenHeight = 1179.0f;
    }
    
    // Initialize credit entries with Y variance between 20%-80% of screen height
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_real_distribution<float> yPosDist(m_screenHeight * 0.2f, m_screenHeight * 0.8f);
    
    // Final label Y position: vertically centered on screen (fontSize ~48, offset by ~24 to center text)
    float finalLabelY = (m_screenHeight * 0.5f) - 24.0f;
    
    m_creditEntries = {
        {"Game Developer:", "Alexandru Istrate", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"Programmer:", "Alexandru Istrate", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"Music Director:", "Alexandru Istrate", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"Pixel Artist:", "Alexandru Istrate", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"Assist. Pixel Artist:", "William Henson", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"Fartist:", "Kevin Hooks", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"Tools Used:", "", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"", "Aseprite", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"", "Xcode & CMake", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"", "FL Studios", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"", "", yPosDist(engine), 0, 0, 0.0f, 0.0f},  // Empty line
        {"Special Thanks:", "Betty Istrate", yPosDist(engine), 0, 0, 0.0f, 0.0f},
        {"", "", yPosDist(engine), 0, 0, 0.0f, 0.0f},  // Empty line
        {"", "", yPosDist(engine), 0, 0, 0.0f, 0.0f},  // Extra gap so Betty scrolls fully off before final freeze
        {"Thank you for playing!", "", finalLabelY, 0, 0, 0.0f, 0.0f}  // Final label already vertically centered
    };
    
    // Track which entry is the final one
    m_finalEntryIndex = m_creditEntries.size() - 1;
    
    // Start text scroll from off-screen right
    m_textScrollOffset = m_screenWidth + 50.0f;
    
    // Create all entities
    CreateEntities();
    
    // Start credits music
    StartCreditsMusic();
    
    GN_LOG_INFO("CreditsState entered - landscape mode, screen: " + 
                std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
}

void CreditsState::Exit() {
    GN_LOG_INFO("CreditsState::Exit");
    
    // Stop music
    StopCreditsMusic();
    
    // Destroy all entities
    DestroyEntities();
}

void CreditsState::Pause() {
    GN_LOG_INFO("CreditsState::Pause");
    // Could pause music here if needed
}

void CreditsState::Resume() {
    GN_LOG_INFO("CreditsState::Resume");
    // Could resume music here if needed
}

void CreditsState::Update(float deltaTime) {
    m_elapsedTime += deltaTime;
    
    // Final hold progression and time-based fallback to fade out
    if (m_finalHoldActive) {
        m_finalHoldTimer += deltaTime;
        if (!m_fadingOut && m_finalHoldTimer >= FINAL_HOLD_DURATION) {
            m_fadingOut = true;
            GN_LOG_INFO("Credits: Final label hold complete, starting fade out");
        }
    }
    // Fallback: time-based fade out near end of duration
    // Enforce final-sequence-only exit: remove time-based auto-fade fallback
    // Fade-out will be triggered only by the final label center + hold sequence
    
    // Update white fade (fade in from white at start, fade out to white at end)
    UpdateWhiteFade(deltaTime);
    
    // Update turd bounce animation
    UpdateTurdBounce(deltaTime);
    
    // Update pipes scrolling
    UpdatePipes(deltaTime);
    
    // Update credit text scrolling
    UpdateCreditScroll(deltaTime);
    
    // Finish when skipped OR when fade out is complete
    if (m_shouldSkip || (m_fadingOut && m_whiteFadeAlpha >= 1.0f)) {
        GN_LOG_INFO("Credits finished - elapsed: " + std::to_string(m_elapsedTime) + 
                   ", duration: " + std::to_string(CREDITS_DURATION) + 
                   ", skipped: " + std::to_string(m_shouldSkip) +
                   ", fadeOut: " + std::to_string(m_fadingOut));
        m_finished = true;
    }
}

void CreditsState::Render() {
    // Rendering is handled by ECS systems
    // Entities are already set up with correct render layers and visibility
}

void CreditsState::HandleInput() {
    if (!m_platformDelegates) return;
    
    // Check for skip button press
    if (m_platformDelegates->input.isTouchDown && m_platformDelegates->input.isTouchDown()) {
        float touchX = 0.0f, touchY = 0.0f;
        
        if (m_platformDelegates->input.getTouchPosition) {
            m_platformDelegates->input.getTouchPosition(0, &touchX, &touchY);  // Touch index 0
            
            if (IsSkipButtonPressed(touchX, touchY)) {
                GN_LOG_INFO("Skip button pressed - ending credits early");
                m_shouldSkip = true;
            }
        }
    }
}

void CreditsState::CreateEntities() {
    CreateWhiteFadeOverlay();  // Create first so it renders on top initially
    CreateBackground();
    CreatePipes();
    CreateTurd();
    CreateCreditText();
    CreateSkipButton();
}

void CreditsState::CreateBackground() {
    m_backgroundEntity = m_ecsSystem->CreateEntity();
    
    // Background sprite - HEIGHT-BASED scaling to ensure full height is visible
    // Texture: 320x180, Screen: 2556x1179 (landscape)
    // Scale based on HEIGHT so top/bottom patterns are fully on-screen
    float textureWidth = 320.0f;
    float textureHeight = 180.0f;
    float heightBasedScale = m_screenHeight / textureHeight;  // Fit height exactly
    
    Sprite sprite;
    sprite.textureId = "PooperTrooperCreditsBackground";
    sprite.width = textureWidth;
    sprite.height = textureHeight;
    sprite.layer = 100;  // UI layer for screen space rendering
    sprite.visible = true;
    // Align frame to logical size to avoid implicit extra scaling in RenderSystem
    sprite.isAnimated = false;
    sprite.frameWidth = static_cast<int>(sprite.width);
    sprite.frameHeight = static_cast<int>(sprite.height);
    sprite.frameCount = 1;
    sprite.currentFrame = 0;
    sprite.sourceWidth = 0.0f;
    sprite.sourceHeight = 0.0f;
    m_ecsSystem->AddComponent(m_backgroundEntity, sprite);
    
    Transform transform;
    transform.position = Gnosis::GNVector2(0.0f, 0.0f);
    transform.scale = Gnosis::GNVector2(heightBasedScale, heightBasedScale);  // Height-based uniform scale
    transform.rotation = 0.0f;
    m_ecsSystem->AddComponent(m_backgroundEntity, transform);
    // Track background entities for proper cleanup
    m_backgroundEntities.push_back(m_backgroundEntity);

    // Tile a second background to the right if height-based scaled width doesn't cover the screen
    const float scaledBgWidth = textureWidth * heightBasedScale;
    if (scaledBgWidth < m_screenWidth) {
        // Create a second background entity without stretching
        Gnosis::Entity bgEntity2 = m_ecsSystem->CreateEntity();

        Sprite sprite2 = sprite; // same texture and frame setup
        m_ecsSystem->AddComponent(bgEntity2, sprite2);

        Transform transform2;
        transform2.position = Gnosis::GNVector2(scaledBgWidth, 0.0f); // place immediately to the right
        transform2.scale = Gnosis::GNVector2(heightBasedScale, heightBasedScale);
        transform2.rotation = 0.0f;
        m_ecsSystem->AddComponent(bgEntity2, transform2);

        m_backgroundEntities.push_back(bgEntity2);
    }
    
    GN_LOG_INFO("Created credits background: texture=" + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) + 
                ", heightBasedScale=" + std::to_string(heightBasedScale) +
                ", finalSize=" + std::to_string(textureWidth * heightBasedScale) + "x" + std::to_string(textureHeight * heightBasedScale) +
                ", screen=" + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight) +
                ", spriteFrame=" + std::to_string(sprite.frameWidth) + "x" + std::to_string(sprite.frameHeight) + 
                ", tiled=" + std::to_string((scaledBgWidth < m_screenWidth) ? 2 : 1) + 
                ", scaledBgWidth=" + std::to_string(scaledBgWidth) + ")");
}

void CreditsState::CreateTurd() {
    m_turdEntity = m_ecsSystem->CreateEntity();
    
    // Add sprite component (TurdletIdle - single frame)
    Sprite sprite;
    sprite.textureId = "TurdletIdle";
    sprite.width = 32.0f;   // Base turd sprite size
    sprite.height = 32.0f;
    sprite.layer = 110;     // UI layer (screen space)
    sprite.visible = true;
    // Align frame to logical size (prevents any width/height ratio distortion)
    sprite.isAnimated = false;
    sprite.frameWidth = static_cast<int>(sprite.width);
    sprite.frameHeight = static_cast<int>(sprite.height);
    sprite.frameCount = 1;
    sprite.currentFrame = 0;
    m_ecsSystem->AddComponent(m_turdEntity, sprite);
    GN_LOG_INFO("Created turd sprite w/h=" + std::to_string(sprite.width) + "x" + std::to_string(sprite.height) +
                " frame=" + std::to_string(sprite.frameWidth) + "x" + std::to_string(sprite.frameHeight));
    
    // Position on left side, vertically centered, scaled 8x
    // Position on left side, vertically centered, dynamic scale
    // Target ~22% of screen height for the 32px sprite (consistent with iPhone look)
    float targetHeight = m_screenHeight * 0.22f;
    float textureSize = 32.0f;
    float turdScale = targetHeight / textureSize;
    
    Transform transform;
    transform.position = Gnosis::GNVector2(m_screenWidth * 0.1f, m_screenHeight / 2.0f - (textureSize * turdScale * 0.5f));
    transform.scale = Gnosis::GNVector2(turdScale, turdScale);
    transform.rotation = 0.0f;
    m_ecsSystem->AddComponent(m_turdEntity, transform);
    
    GN_LOG_INFO("Created bouncing turd at position (150, " + std::to_string(transform.position.y) + ")");
}

void CreditsState::CreatePipes() {
    // Create 10 toilet PAIRS using percentage-based scaling
    // Target same ~22% relative scale as Turd (since they share the same pixel art grid usually)
    // Actually pipes are larger. Visual height 256px.
    // On iPhone (1179h), 256px is ~22%.
    // So let's use screen height to determine scale.
    float targetHeight = m_screenHeight * 0.22f; // 256px visual height
    float textureHeight = 256.0f; // This is the sprite height
    // Wait, sprite is 64x256. 
    // Existing code: pipeScale = 8.0f. 256 * 8 = 2048 ?? 
    // Ah, lines 303: width=64, height=256.
    // If scale was 8.0f, height would be 2048px? That's taller than the screen (1179).
    // The previous code said "visual 65x190 like ObstacleSystem".
    // Let's stick to the ObstacleSystem reference logic: Pipes are big.
    // Logic: Pipes usually span a large portion of vertical space.
    // Let's use a scale that makes them look "correct" - maybe the 8.0f was excessive or I misread the texture size.
    // If standard texture is 32px and scale is 8, that's 256px.
    // If texture is 256px? 
    // Let's assume we want to preserve the *relative* look.
    // Let's target the pipe *width* (64px texture) to be ~15% of screen height?
    // 64 * 8 = 512px width? On 1179h? That's huge.
    // Let's assume the previous `8.0f` was based on a smaller base unit or I should trust the `8.0f` logic but scale it relative to resolution.
    // If 8.0f was for 1179h (iPhone).
    // New Scale = 8.0f * (m_screenHeight / 1179.0f).
    
    float pipeScale = 8.0f * (m_screenHeight / 1179.0f);
    
    float visualToiletHeight = 256.0f * pipeScale;  // Height of the sprite * scale
    float pipeSpacing = m_screenWidth * 0.4f;  // 40% of screen width between pairs
    float startX = -m_screenWidth;  // Start completely off-screen to the left (full screen width)
    float fixedGapHeight = 400.0f;  // Reduced vertical gap between top and bottom toilets
    
    // Top toilet Y: 80% above screen like ObstacleSystem
    float topToiletY = -visualToiletHeight * 0.8f;
    
    GN_LOG_INFO("Creating pipes (PAIRS): topY=" + std::to_string(topToiletY) + ", gap=" + std::to_string(fixedGapHeight) + ", startX=" + std::to_string(startX) + ", spacing=" + std::to_string(pipeSpacing) + ", scale=" + std::to_string(pipeScale));
    
    for (int i = 0; i < 10; ++i) {
        // Create top pipe (visual 65x190)
        Gnosis::Entity topPipe = m_ecsSystem->CreateEntity();
        Sprite topSprite;
        topSprite.textureId = "TopToilet";
        topSprite.width = 64.0f;
        topSprite.height = 256.0f;
        topSprite.layer = 105;  // UI layer (screen space)
        topSprite.visible = true;
        topSprite.isAnimated = false;
        topSprite.frameWidth = 64;
        topSprite.frameHeight = 256;
        topSprite.frameCount = 1;
        topSprite.currentFrame = 0;
        m_ecsSystem->AddComponent(topPipe, topSprite);
        
        Transform topTransform;
        topTransform.position = Gnosis::GNVector2(startX, topToiletY);
        topTransform.scale = Gnosis::GNVector2(pipeScale, pipeScale);
        topTransform.rotation = 0.0f;
        m_ecsSystem->AddComponent(topPipe, topTransform);
        m_pipeEntities.push_back(topPipe);
        
        // Create bottom pipe from visual height + fixed gap
        float bottomToiletY = topToiletY + visualToiletHeight + fixedGapHeight;
        
        Gnosis::Entity bottomPipe = m_ecsSystem->CreateEntity();
        Sprite bottomSprite;
        bottomSprite.textureId = "BottomToilet";
        bottomSprite.width = 64.0f;
        bottomSprite.height = 256.0f;
        bottomSprite.layer = 105;  // UI layer (screen space)
        bottomSprite.visible = true;
        bottomSprite.isAnimated = false;
        bottomSprite.frameWidth = 64;
        bottomSprite.frameHeight = 256;
        bottomSprite.frameCount = 1;
        bottomSprite.currentFrame = 0;
        m_ecsSystem->AddComponent(bottomPipe, bottomSprite);
        
        Transform bottomTransform;
        bottomTransform.position = Gnosis::GNVector2(startX, bottomToiletY);
        bottomTransform.scale = Gnosis::GNVector2(pipeScale, pipeScale);
        bottomTransform.rotation = 0.0f;
        m_ecsSystem->AddComponent(bottomPipe, bottomTransform);
        m_pipeEntities.push_back(bottomPipe);
        
        startX += pipeSpacing;  // Use calculated spacing
    }
    
    GN_LOG_INFO("Created " + std::to_string(m_pipeEntities.size()) + " pipe entities (10 pairs, scale=" + std::to_string(pipeScale) + ", topY=" + std::to_string(topToiletY) + ", gap=" + std::to_string(fixedGapHeight) + ", spacing=" + std::to_string(pipeSpacing) + ", screen=" + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight) + ")");
}

void CreditsState::CreateCreditText() {
    // Create text entities for each credit entry with UIElement for outline support
    float xOffset = m_textScrollOffset;
    float charWidth = 48.0f * (m_screenWidth / 2556.0f);
    
    GN_LOG_INFO("=== CreateCreditText: Starting creation of " + std::to_string(m_creditEntries.size()) + " credit entries ===");
    
    for (size_t i = 0; i < m_creditEntries.size(); ++i) {
        auto& entry = m_creditEntries[i];
        
        GN_LOG_INFO("Entry[" + std::to_string(i) + "]: title='" + entry.title + "', name='" + entry.name + "', yOffset=" + std::to_string(entry.yOffset));
        
        // Create title text if present
        if (!entry.title.empty()) {
            Gnosis::Entity titleEntity = m_ecsSystem->CreateEntity();
            
            UIElement titleUI;
            titleUI.buttonText = entry.title;
            titleUI.fontSize = 60.0f;
            titleUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
            titleUI.textOutlineWidth = 3.0f;
            titleUI.visible = true;
            titleUI.textLayer = 45;
            titleUI.centerTextHorizontally = false;
            titleUI.centerTextVertically = false;
            m_ecsSystem->AddComponent(titleEntity, titleUI);
            
            Transform titleTransform;
            titleTransform.position = Gnosis::GNVector2(xOffset, entry.yOffset);
            titleTransform.scale = Gnosis::GNVector2(1.0f, 1.0f);
            titleTransform.rotation = 0.0f;
            m_ecsSystem->AddComponent(titleEntity, titleTransform);
            
            entry.titleEntity = titleEntity;
            entry.titleWidth = entry.title.length() * charWidth;
            m_creditTextEntities.push_back(titleEntity);
            
            // Track final title entity
            if (i == m_finalEntryIndex) {
                m_finalTitleEntity = titleEntity;
                GN_LOG_INFO("  -> FINAL TITLE ENTITY tracked: entity=" + std::to_string(titleEntity) + ", x=" + std::to_string(xOffset));
            }
            
            GN_LOG_INFO("  -> Created TITLE entity=" + std::to_string(titleEntity) + " at x=" + std::to_string(xOffset) + ", y=" + std::to_string(entry.yOffset) + ", width=" + std::to_string(entry.titleWidth));
            xOffset += entry.titleWidth + (30.0f * (m_screenWidth / 2556.0f));
        }
        
        // Create name text if present
        if (!entry.name.empty()) {
            Gnosis::Entity nameEntity = m_ecsSystem->CreateEntity();
            
            UIElement nameUI;
            nameUI.buttonText = entry.name;
            nameUI.fontSize = 60.0f;
            nameUI.textColor = Gnosis::GNColor(255, 255, 255, 255);
            nameUI.textOutlineWidth = 3.0f;
            nameUI.visible = true;
            nameUI.textLayer = 45;
            nameUI.centerTextHorizontally = false;
            nameUI.centerTextVertically = false;
            m_ecsSystem->AddComponent(nameEntity, nameUI);
            
            Transform nameTransform;
            nameTransform.position = Gnosis::GNVector2(xOffset, entry.yOffset + 60.0f);
            nameTransform.scale = Gnosis::GNVector2(1.0f, 1.0f);
            nameTransform.rotation = 0.0f;
            m_ecsSystem->AddComponent(nameEntity, nameTransform);
            
            entry.nameEntity = nameEntity;
            entry.nameWidth = entry.name.length() * charWidth;
            m_creditTextEntities.push_back(nameEntity);
            
            GN_LOG_INFO("  -> Created NAME entity=" + std::to_string(nameEntity) + " at x=" + std::to_string(xOffset) + ", y=" + std::to_string(entry.yOffset + 60.0f) + ", width=" + std::to_string(entry.nameWidth));
            xOffset += entry.nameWidth + (30.0f * (m_screenWidth / 2556.0f));
        }
        
        // Add spacing after each entry
        xOffset += 75.0f * (m_screenWidth / 2556.0f);
    }
    
    GN_LOG_INFO("=== CreateCreditText: Created " + std::to_string(m_creditTextEntities.size()) + " total text entities ===");
}

void CreditsState::CreateSkipButton() {
    m_skipButtonEntity = m_ecsSystem->CreateEntity();
    
    // Create skip button with blue outlined text in bottom right corner (landscape)
    // Dynamic font size
    float fontScaleFactor = m_screenWidth / 2556.0f;
    float fontSize = 48.0f * fontScaleFactor;
    
    UIElement skipUI;
    skipUI.buttonText = "SKIP";
    skipUI.fontSize = fontSize;  // Dynamic size
    skipUI.textColor = Gnosis::GNColor(0, 150, 255, 255);  // Blue
    skipUI.textOutlineWidth = 3.0f;  // Outlined
    skipUI.visible = true;
    skipUI.textLayer = 100;
    skipUI.centerTextHorizontally = false;
    skipUI.centerTextVertically = false;
    m_ecsSystem->AddComponent(m_skipButtonEntity, skipUI);
    
    // Position in bottom-right corner with proper margin
    // Text renders from top-left, so we need to position it accounting for text size
    float marginRight = 50.0f * fontScaleFactor;  // Dynamic margin
    float marginBottom = 50.0f * fontScaleFactor;
    float estimatedTextWidth = 250.0f * fontScaleFactor;  // Dynamic width estimate
    
    Transform skipTransform;
    skipTransform.position = Gnosis::GNVector2(
        m_screenWidth - estimatedTextWidth - marginRight,
        m_screenHeight - skipUI.fontSize - marginBottom
    );
    skipTransform.scale = Gnosis::GNVector2(1.0f, 1.0f);
    skipTransform.rotation = 0.0f;
    m_ecsSystem->AddComponent(m_skipButtonEntity, skipTransform);
    
    GN_LOG_INFO("Created skip button at (" + std::to_string(skipTransform.position.x) + 
                ", " + std::to_string(skipTransform.position.y) + "), screen=" + 
                std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
}

void CreditsState::CreateWhiteFadeOverlay() {
    m_whiteFadeEntity = m_ecsSystem->CreateEntity();
    
    // Create full-screen white rectangle overlay (exactly like boss level)
    UIShape whiteRect;
    whiteRect.type = UIShapeType::Rectangle;
    whiteRect.width = m_screenWidth;
    whiteRect.height = m_screenHeight;
    whiteRect.color = Gnosis::GNColor(255, 255, 255, 255);  // White, full alpha
    whiteRect.visible = true;
    whiteRect.layer = 250;  // Top-most layer
    m_ecsSystem->AddComponent(m_whiteFadeEntity, whiteRect);
    
    Transform fadeTransform;
    fadeTransform.position = Gnosis::GNVector2(0.0f, 0.0f);
    fadeTransform.scale = Gnosis::GNVector2(1.0f, 1.0f);
    fadeTransform.rotation = 0.0f;
    m_ecsSystem->AddComponent(m_whiteFadeEntity, fadeTransform);
    
    GN_LOG_INFO("Created white fade overlay: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight) + " at (0,0)");
}

void CreditsState::DestroyEntities() {
    // Destroy background
    if (m_backgroundEntity != 0) {
        m_ecsSystem->DestroyEntity(m_backgroundEntity);
        m_backgroundEntity = 0;
    }
    // Destroy any additional background tiles
    for (auto e : m_backgroundEntities) {
        m_ecsSystem->DestroyEntity(e);
    }
    m_backgroundEntities.clear();
    
    // Destroy turd
    if (m_turdEntity != 0) {
        m_ecsSystem->DestroyEntity(m_turdEntity);
        m_turdEntity = 0;
    }
    
    // Destroy skip button
    if (m_skipButtonEntity != 0) {
        m_ecsSystem->DestroyEntity(m_skipButtonEntity);
        m_skipButtonEntity = 0;
    }
    
    // Destroy white fade overlay
    if (m_whiteFadeEntity != 0) {
        m_ecsSystem->DestroyEntity(m_whiteFadeEntity);
        m_whiteFadeEntity = 0;
    }
    
    // Destroy pipes
    for (auto pipeEntity : m_pipeEntities) {
        m_ecsSystem->DestroyEntity(pipeEntity);
    }
    m_pipeEntities.clear();
    
    // Destroy credit text
    for (auto textEntity : m_creditTextEntities) {
        m_ecsSystem->DestroyEntity(textEntity);
    }
    m_creditTextEntities.clear();
    
    GN_LOG_INFO("Destroyed all credits entities");
}

void CreditsState::UpdateTurdBounce(float deltaTime) {
    if (m_turdEntity == 0) return;
    
    m_turdBounceTimer += deltaTime * TURD_BOUNCE_SPEED;
    
    Transform* transform = m_ecsSystem->GetComponent<Transform>(m_turdEntity);
    if (transform) {
        // Sinusoidal bounce (adjusted for 8x scaled sprite)
        float baseY = m_screenHeight / 2.0f - 128.0f;  // Centered accounting for 8x scale
        transform->position.y = baseY + std::sin(m_turdBounceTimer) * TURD_BOUNCE_AMPLITUDE;
    }
}

void CreditsState::UpdatePipes(float deltaTime) {
    // Move pipes left
    float pipeSpacing = m_screenWidth * 0.4f;  // Same spacing as creation (40% of screen width)
    float rightmostX = -9999.0f;
    
    for (auto pipeEntity : m_pipeEntities) {
        Transform* transform = m_ecsSystem->GetComponent<Transform>(pipeEntity);
        if (transform) {
            transform->position.x -= PIPE_SPEED * deltaTime;
            
            // Track rightmost pipe
            if (transform->position.x > rightmostX) {
                rightmostX = transform->position.x;
            }
        }
    }
    
    // Wrap pipes that go COMPLETELY off-screen left before wrapping
    for (auto pipeEntity : m_pipeEntities) {
        Transform* transform = m_ecsSystem->GetComponent<Transform>(pipeEntity);
        if (transform) {
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(pipeEntity);
            // Use sprite frame width × actual transform scale to compute on-screen width
            float pipeWidth = (sprite ? static_cast<float>(sprite->frameWidth) : 65.0f) * transform->scale.x;
            
            // If pipe is completely off-screen left, wrap to right of rightmost pipe
            if (transform->position.x + pipeWidth < -pipeWidth) {
                transform->position.x = rightmostX + pipeSpacing;
            }
        }
    }
}

void CreditsState::UpdateCreditScroll(float deltaTime) {
    // Update scroll offset (only when not in final hold)
    if (!m_finalHoldActive) {
        m_textScrollOffset -= (SCROLL_SPEED * 0.7875f) * deltaTime;
    }
    
    // Update all credit text positions using stored entity references
    float xOffset = m_textScrollOffset;
    float charWidth = 48.0f * (m_screenWidth / 2556.0f);
    
    for (size_t i = 0; i < m_creditEntries.size(); ++i) {
        auto& entry = m_creditEntries[i];
        
        // Only freeze the FINAL label horizontally when hold is active
        bool isFinalEntry = (i == m_finalEntryIndex);
        bool shouldUpdateX = !m_finalHoldActive || !isFinalEntry;
        
        // Update title position if present
        if (entry.titleEntity != 0) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(entry.titleEntity);
            if (transform && shouldUpdateX) {
                transform->position.x = xOffset;
            }

            // Check if this is the final label and if it's centered
            if (!m_finalHoldActive && i == m_finalEntryIndex) {
                float centerX = xOffset + entry.titleWidth * 0.5f;
                float screenCenter = m_screenWidth * 0.5f;
                float distance = std::abs(centerX - screenCenter);
                
                if (distance <= FINAL_CENTER_TOLERANCE) {
                    GN_LOG_INFO("FINAL LABEL CENTERED! centerX=" + std::to_string(centerX) + ", screenCenter=" + std::to_string(screenCenter) + ", distance=" + std::to_string(distance));
                    BeginFinalSequence();
                }
            }

            xOffset += entry.titleWidth + (30.0f * (m_screenWidth / 2556.0f));
        }
        
        // Update name position if present
        if (entry.nameEntity != 0) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(entry.nameEntity);
            if (transform && shouldUpdateX) {
                transform->position.x = xOffset;
            }
            xOffset += entry.nameWidth + (30.0f * (m_screenWidth / 2556.0f));
        }
        
        xOffset += 75.0f * (m_screenWidth / 2556.0f);
    }
}

void CreditsState::UpdateWhiteFade(float deltaTime) {
    if (m_whiteFadeEntity == 0) return;
    
    UIShape* fadeShape = m_ecsSystem->GetComponent<UIShape>(m_whiteFadeEntity);
    if (!fadeShape) return;
    
    if (m_fadingOut) {
        // Fade TO white at end (increase alpha)
        m_whiteFadeAlpha += deltaTime / FADE_OUT_DURATION;
        m_whiteFadeAlpha = std::min(1.0f, m_whiteFadeAlpha);
        fadeShape->visible = true;  // Make sure it's visible during fade out
    } else if (m_whiteFadeAlpha > 0.0f) {
        // Fade FROM white at start (decrease alpha)
        m_whiteFadeAlpha -= deltaTime / FADE_IN_DURATION;
        m_whiteFadeAlpha = std::max(0.0f, m_whiteFadeAlpha);
        
        // Hide completely when fully transparent
        if (m_whiteFadeAlpha <= 0.0f) {
            fadeShape->visible = false;
        }
    }
    
    // Update alpha component
    fadeShape->color.a = static_cast<uint8_t>(m_whiteFadeAlpha * 255.0f);
}

void CreditsState::CheckMusicCompletion() {
    // Music duration is obtained when music starts
    // For now, we just check elapsed time against duration
    // If duration is 0, we'll wait for it to be set by the audio system
}

bool CreditsState::IsSkipButtonPressed(float touchX, float touchY) {
    if (m_skipButtonEntity == 0) return false;
    
    Transform* transform = m_ecsSystem->GetComponent<Transform>(m_skipButtonEntity);
    if (!transform) return false;
    
    // Button bounds - significantly larger hit area to cover all of the text and surrounding area
    // Text is "SKIP" at fontSize 48.0f with outline width 3.0f
    float fontSize = 48.0f;  // Match the actual fontSize
    float buttonWidth = 400.0f;  // Very large hit area to ensure full text coverage (increased from 300.0f)
    float buttonHeight = fontSize + 60.0f;  // Extra padding above and below (increased from +40.0f)
    float buttonLeft = transform->position.x - 20.0f;  // Extra padding on left
    float buttonRight = transform->position.x + buttonWidth;
    float buttonTop = transform->position.y - 10.0f;  // Extra padding on top
    float buttonBottom = transform->position.y + buttonHeight;
    
    return (touchX >= buttonLeft && touchX <= buttonRight &&
            touchY >= buttonTop && touchY <= buttonBottom);
}

void CreditsState::StartCreditsMusic() {
    if (!m_platformDelegates || !m_platformDelegates->audio.playMusic) {
        GN_LOG_WARN("Cannot start credits music - no audio delegates");
        return;
    }
    
    // Play EndTheme.mp3
    m_platformDelegates->audio.playMusic("EndTheme.mp3", 1.0f, true);
    m_musicStarted = true;
    
    // Get music duration
    m_totalMusicDuration = GetMusicDuration();
    
    GN_LOG_INFO("Started credits music (EndTheme.mp3) - duration: " + std::to_string(m_totalMusicDuration) + "s");
}

void CreditsState::StopCreditsMusic() {
    if (!m_platformDelegates || !m_platformDelegates->audio.stopMusic) {
        return;
    }
    
    m_platformDelegates->audio.stopMusic();
    m_musicStarted = false;
    
    GN_LOG_INFO("Stopped credits music");
}

float CreditsState::GetMusicDuration() {
    // For now, return a default duration since we don't have getMusicDuration delegate
    // The credits will auto-advance based on this timer or when skipped
    GN_LOG_INFO("Using default music duration of 120 seconds for credits");
    return 120.0f;  // 2 minutes default
}

void CreditsState::CacheScreenDimensions() {
    // This is now called from Enter() directly, but kept for safety
    if (m_screenWidth <= 0.0f || m_screenHeight <= 0.0f) {
        if (m_platformDelegates && m_platformDelegates->renderer.getScreenInfo) {
            ScreenInfo screenInfo;
            m_platformDelegates->renderer.getScreenInfo(&screenInfo);
            m_screenWidth = screenInfo.pixelWidth;
            m_screenHeight = screenInfo.pixelHeight;
            GN_LOG_INFO("CacheScreenDimensions: Got screen info: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
        } else {
            m_screenWidth = 2556.0f;
            m_screenHeight = 1179.0f;
            GN_LOG_WARN("Could not get screen info, using default landscape: " + 
                       std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
        }
    }
}

void CreditsState::BeginFinalSequence() {
    if (m_finalHoldActive) return;
    m_finalHoldActive = true;
    m_finalHoldTimer = 0.0f;

    // Final label is already vertically centered from creation; no need to shift Y
    GN_LOG_INFO("Credits: Final label centered - beginning 5s hold before fade (2s fade out)");
}

// Optional stub (header may declare it); centering handled inline during scroll update
bool CreditsState::IsFinalLabelCentered(float /*tolerancePx*/) const {
    return false;
}

} // namespace GameCore