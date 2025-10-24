#include "CreditsState.h"
#include "../Components/GameComponents.h"
#include "../../Engine/Core/GNLog.h"
#include <random>
#include <cmath>

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
    , m_screenWidth(0.0f)
    , m_screenHeight(0.0f)
    , m_backgroundEntity(0)
    , m_turdEntity(0)
    , m_skipButtonEntity(0)
    , m_whiteFadeEntity(0)
{
    GN_LOG_INFO("CreditsState created");
}

CreditsState::~CreditsState() {
    GN_LOG_INFO("CreditsState destroyed");
}

void CreditsState::Enter() {
    GN_LOG_INFO("CreditsState::Enter - getting screen dimensions from delegates");
    
    // Get actual screen dimensions from platform delegates (landscape mode)
    if (m_platformDelegates && m_platformDelegates->renderer.getScreenInfo) {
        ScreenInfo screenInfo;
        m_platformDelegates->renderer.getScreenInfo(&screenInfo);
        m_screenWidth = screenInfo.pixelWidth;
        m_screenHeight = screenInfo.pixelHeight;
        GN_LOG_INFO("CreditsState: Screen dimensions from delegates: " + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
    } else {
        GN_LOG_ERROR("CreditsState: Cannot get screen info from delegates!");
        m_screenWidth = 2556.0f;
        m_screenHeight = 1179.0f;
    }
    
    // Initialize credit entries with more random Y variance
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_real_distribution<float> offsetDist(-60.0f, 60.0f);  // More variance
    
    m_creditEntries = {
        {"Game Developer:", "Alexandru Istrate", offsetDist(engine)},
        {"Programmer:", "Alexandru Istrate", offsetDist(engine)},
        {"Music Director:", "Alexandru Istrate", offsetDist(engine)},
        {"Pixel Artist:", "Alexandru Istrate", offsetDist(engine)},
        {"Assist. Pixel Artist:", "William Henson", offsetDist(engine)},
        {"Fartist:", "Kevin Hooks", offsetDist(engine)},
        {"Tools Used:", "", offsetDist(engine)},
        {"", "Aseprite", offsetDist(engine)},
        {"", "Xcode & CMake", offsetDist(engine)},
        {"", "FL Studios", offsetDist(engine)},
        {"", "", offsetDist(engine)},  // Empty line
        {"Special Thanks to:", "", offsetDist(engine)},
        {"", "Betty Istrate", offsetDist(engine)},
        {"", "", offsetDist(engine)},  // Empty line
        {"Thank you for playing!", "", offsetDist(engine)}
    };
    
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
    
    // Update white fade (fade out from white at start)
    UpdateWhiteFade(deltaTime);
    
    // Update turd bounce animation
    UpdateTurdBounce(deltaTime);
    
    // Update pipes scrolling
    UpdatePipes(deltaTime);
    
    // Update credit text scrolling
    UpdateCreditScroll(deltaTime);
    
    // Check if music is complete
    CheckMusicCompletion();
    
    // If skipped or music complete, finish state
    if (m_shouldSkip || (m_totalMusicDuration > 0.0f && m_elapsedTime >= m_totalMusicDuration)) {
        GN_LOG_INFO("Credits finished - elapsed: " + std::to_string(m_elapsedTime) + 
                   ", duration: " + std::to_string(m_totalMusicDuration) + 
                   ", skipped: " + std::to_string(m_shouldSkip));
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
    
    // Credits background texture dimensions (portrait texture used in landscape)
    float textureWidth = 384.0f;
    float textureHeight = 512.0f;
    
    // Use UNIFORM SCALING like boss level - scale to fit screen height, then it will cover width too
    float heightScale = m_screenHeight / textureHeight;
    float finalScale = heightScale * 1.0f;  // scaleMultiplier = 1.0
    
    // Add sprite component for credits background
    Sprite sprite;
    sprite.textureId = "FloppyTurdCreditsBackground";
    sprite.width = textureWidth;
    sprite.height = textureHeight;
    sprite.layer = 0;  // Background layer
    sprite.visible = true;
    m_ecsSystem->AddComponent(m_backgroundEntity, sprite);
    
    // Position at top-left (0, 0) with UNIFORM scale
    Transform transform;
    transform.position = Gnosis::GNVector2(0.0f, 0.0f);
    transform.scale = Gnosis::GNVector2(finalScale, finalScale);  // UNIFORM scaling
    transform.rotation = 0.0f;
    m_ecsSystem->AddComponent(m_backgroundEntity, transform);
    
    float scaledWidth = textureWidth * finalScale;
    float scaledHeight = textureHeight * finalScale;
    
    GN_LOG_INFO("Created credits background: texture=" + std::to_string(textureWidth) + "x" + std::to_string(textureHeight) + 
                ", heightScale=" + std::to_string(heightScale) +
                ", finalScale=" + std::to_string(finalScale) + 
                ", scaledSize=" + std::to_string(scaledWidth) + "x" + std::to_string(scaledHeight) +
                ", screen=" + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
}

void CreditsState::CreateTurd() {
    m_turdEntity = m_ecsSystem->CreateEntity();
    
    // Add sprite component (TurdletIdle - single frame)
    Sprite sprite;
    sprite.textureId = "TurdletIdle";
    sprite.width = 32.0f;   // Base turd sprite size
    sprite.height = 32.0f;
    sprite.layer = 50;      // Above pipes
    sprite.visible = true;
    m_ecsSystem->AddComponent(m_turdEntity, sprite);
    
    // Position on left side, vertically centered, scaled 8x
    Transform transform;
    transform.position = Gnosis::GNVector2(150.0f, m_screenHeight / 2.0f - 128.0f);  // Adjusted for 8x scale
    transform.scale = Gnosis::GNVector2(8.0f, 8.0f);  // Scale 8x like other sprites
    transform.rotation = 0.0f;
    m_ecsSystem->AddComponent(m_turdEntity, transform);
    
    GN_LOG_INFO("Created bouncing turd at position (150, " + std::to_string(transform.position.y) + ")");
}

void CreditsState::CreatePipes() {
    // Create 6 toilet pairs with proper gap and spacing (landscape mode)
    float centerY = m_screenHeight / 2.0f;
    float pipeScale = 6.0f;  // Scale 6x
    float scaledPipeHeight = 190.0f * pipeScale;  // 1140px
    float gapSize = m_screenHeight * 0.35f;  // 35% of screen height as gap
    float startX = m_screenWidth * 0.2f;  // Start at 20% of screen width
    
    GN_LOG_INFO("Creating pipes: centerY=" + std::to_string(centerY) + ", gapSize=" + std::to_string(gapSize) + ", startX=" + std::to_string(startX));
    
    for (int i = 0; i < 6; ++i) {
        // Create top pipe
        Gnosis::Entity topPipe = m_ecsSystem->CreateEntity();
        Sprite topSprite;
        topSprite.textureId = "TopToilet";
        topSprite.width = 65.0f;
        topSprite.height = 190.0f;
        topSprite.layer = 40;
        topSprite.visible = true;
        m_ecsSystem->AddComponent(topPipe, topSprite);
        
        Transform topTransform;
        float topY = centerY - (gapSize / 2.0f) - scaledPipeHeight;
        topTransform.position = Gnosis::GNVector2(startX, topY);
        topTransform.scale = Gnosis::GNVector2(pipeScale, pipeScale);
        topTransform.rotation = 0.0f;
        m_ecsSystem->AddComponent(topPipe, topTransform);
        
        m_pipeEntities.push_back(topPipe);
        
        // Create bottom pipe
        Gnosis::Entity bottomPipe = m_ecsSystem->CreateEntity();
        Sprite bottomSprite;
        bottomSprite.textureId = "BottomToilet";
        bottomSprite.width = 65.0f;
        bottomSprite.height = 190.0f;
        bottomSprite.layer = 40;
        bottomSprite.visible = true;
        m_ecsSystem->AddComponent(bottomPipe, bottomSprite);
        
        Transform bottomTransform;
        float bottomY = centerY + (gapSize / 2.0f);
        bottomTransform.position = Gnosis::GNVector2(startX, bottomY);
        bottomTransform.scale = Gnosis::GNVector2(pipeScale, pipeScale);
        bottomTransform.rotation = 0.0f;
        m_ecsSystem->AddComponent(bottomPipe, bottomTransform);
        
        m_pipeEntities.push_back(bottomPipe);
        
        startX += PIPE_SPACING * pipeScale;  // Spacing proportional to scale
    }
    
    GN_LOG_INFO("Created " + std::to_string(m_pipeEntities.size()) + " pipe entities (6 pairs, scale=" + std::to_string(pipeScale) + ", screen=" + std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight) + ")");
}

void CreditsState::CreateCreditText() {
    // Create text entities for each credit entry with UIElement for outline support
    float xOffset = m_textScrollOffset;
    
    for (size_t i = 0; i < m_creditEntries.size(); ++i) {
        const auto& entry = m_creditEntries[i];
        
        // Create title text if present
        if (!entry.title.empty()) {
            Gnosis::Entity titleEntity = m_ecsSystem->CreateEntity();
            
            UIElement titleUI;
            titleUI.buttonText = entry.title;
            titleUI.fontSize = 48.0f;  // Bigger text
            titleUI.textColor = Gnosis::GNColor(255, 255, 255, 255);  // White
            titleUI.textOutlineWidth = 3.0f;  // Outlined text
            titleUI.visible = true;
            titleUI.textLayer = 45;
            titleUI.centerTextHorizontally = false;
            titleUI.centerTextVertically = false;
            m_ecsSystem->AddComponent(titleEntity, titleUI);
            
            Transform titleTransform;
            titleTransform.position = Gnosis::GNVector2(xOffset, (m_screenHeight / 2.0f) + entry.yOffset);
            titleTransform.scale = Gnosis::GNVector2(1.0f, 1.0f);
            titleTransform.rotation = 0.0f;
            m_ecsSystem->AddComponent(titleEntity, titleTransform);
            
            m_creditTextEntities.push_back(titleEntity);
            
            // Approximate text width (bigger font, more spacing) - scale with screen
            float charWidth = 48.0f * (m_screenWidth / 2556.0f);  // Scale with screen width
            xOffset += entry.title.length() * charWidth + (60.0f * (m_screenWidth / 2556.0f));
        }
        
        // Create name text if present
        if (!entry.name.empty()) {
            Gnosis::Entity nameEntity = m_ecsSystem->CreateEntity();
            
            UIElement nameUI;
            nameUI.buttonText = entry.name;
            nameUI.fontSize = 48.0f;  // Bigger text
            nameUI.textColor = Gnosis::GNColor(255, 255, 255, 255);  // White
            nameUI.textOutlineWidth = 3.0f;  // Outlined text
            nameUI.visible = true;
            nameUI.textLayer = 45;
            nameUI.centerTextHorizontally = false;
            nameUI.centerTextVertically = false;
            m_ecsSystem->AddComponent(nameEntity, nameUI);
            
            Transform nameTransform;
            nameTransform.position = Gnosis::GNVector2(xOffset, (m_screenHeight / 2.0f) + entry.yOffset + 60.0f);
            nameTransform.scale = Gnosis::GNVector2(1.0f, 1.0f);
            nameTransform.rotation = 0.0f;
            m_ecsSystem->AddComponent(nameEntity, nameTransform);
            
            m_creditTextEntities.push_back(nameEntity);
            
            float charWidth = 48.0f * (m_screenWidth / 2556.0f);
            xOffset += entry.name.length() * charWidth + (60.0f * (m_screenWidth / 2556.0f));
        }
        
        // Add more spacing after each entry - scale with screen
        xOffset += 150.0f * (m_screenWidth / 2556.0f);
    }
    
    GN_LOG_INFO("Created " + std::to_string(m_creditTextEntities.size()) + " credit text entities");
}

void CreditsState::CreateSkipButton() {
    m_skipButtonEntity = m_ecsSystem->CreateEntity();
    
    // Create skip button with blue outlined text in bottom right corner (landscape)
    UIElement skipUI;
    skipUI.buttonText = "SKIP";
    skipUI.fontSize = 32.0f;  // Bigger text
    skipUI.textColor = Gnosis::GNColor(0, 150, 255, 255);  // Blue
    skipUI.textOutlineWidth = 3.0f;  // Outlined
    skipUI.visible = true;
    skipUI.textLayer = 100;
    skipUI.centerTextHorizontally = false;
    skipUI.centerTextVertically = false;
    m_ecsSystem->AddComponent(m_skipButtonEntity, skipUI);
    
    // Position 5% from right edge and 5% from bottom (landscape) - properly anchored
    Transform skipTransform;
    skipTransform.position = Gnosis::GNVector2(m_screenWidth - (m_screenWidth * 0.05f), m_screenHeight - (m_screenHeight * 0.05f));
    skipTransform.scale = Gnosis::GNVector2(1.0f, 1.0f);
    skipTransform.rotation = 0.0f;
    m_ecsSystem->AddComponent(m_skipButtonEntity, skipTransform);
    
    GN_LOG_INFO("Created skip button at (" + std::to_string(skipTransform.position.x) + 
                ", " + std::to_string(skipTransform.position.y) + ")");
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
    
    // Wrap pipes that go off-screen left
    for (auto pipeEntity : m_pipeEntities) {
        Transform* transform = m_ecsSystem->GetComponent<Transform>(pipeEntity);
        if (transform) {
            Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(pipeEntity);
            float pipeWidth = sprite ? sprite->width : 64.0f;
            
            // If pipe is completely off-screen left, wrap to right
            if (transform->position.x + pipeWidth < 0) {
                transform->position.x = rightmostX + PIPE_SPACING;
            }
        }
    }
}

void CreditsState::UpdateCreditScroll(float deltaTime) {
    m_textScrollOffset -= SCROLL_SPEED * deltaTime;
    
    // Update all credit text positions
    float xOffset = m_textScrollOffset;
    
    for (size_t i = 0; i < m_creditEntries.size(); ++i) {
        const auto& entry = m_creditEntries[i];
        
        // Update title position if present
        if (!entry.title.empty() && i * 2 < m_creditTextEntities.size()) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(m_creditTextEntities[i * 2]);
            if (transform) {
                transform->position.x = xOffset;
            }
            float charWidth = 48.0f * (m_screenWidth / 2556.0f);
            xOffset += entry.title.length() * charWidth + (60.0f * (m_screenWidth / 2556.0f));
        }
        
        // Update name position if present
        if (!entry.name.empty() && i * 2 + 1 < m_creditTextEntities.size()) {
            Transform* transform = m_ecsSystem->GetComponent<Transform>(m_creditTextEntities[i * 2 + 1]);
            if (transform) {
                transform->position.x = xOffset;
            }
            float charWidth = 48.0f * (m_screenWidth / 2556.0f);
            xOffset += entry.name.length() * charWidth + (60.0f * (m_screenWidth / 2556.0f));
        }
        
        xOffset += 150.0f * (m_screenWidth / 2556.0f);
    }
    
    // Wrap text if it scrolls completely off-screen left
    float totalTextWidth = xOffset - m_textScrollOffset;
    if (m_textScrollOffset < -totalTextWidth) {
        m_textScrollOffset = m_screenWidth + 50.0f;
    }
}

void CreditsState::UpdateWhiteFade(float deltaTime) {
    if (m_whiteFadeEntity == 0) return;
    
    // Fade out from white over FADE_IN_DURATION seconds
    if (m_whiteFadeAlpha > 0.0f) {
        m_whiteFadeAlpha -= deltaTime / FADE_IN_DURATION;
        m_whiteFadeAlpha = std::max(0.0f, m_whiteFadeAlpha);
        
        UIShape* fadeShape = m_ecsSystem->GetComponent<UIShape>(m_whiteFadeEntity);
        if (fadeShape) {
            fadeShape->color.a = static_cast<uint8_t>(m_whiteFadeAlpha * 255.0f);
            
            // Hide completely when fully transparent
            if (m_whiteFadeAlpha <= 0.0f) {
                fadeShape->visible = false;
            }
        }
    }
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
    
    // Approximate button bounds (SKIP text at bottom right, check area around it)
    float buttonWidth = 120.0f;
    float buttonHeight = 60.0f;
    float buttonLeft = transform->position.x - buttonWidth;
    float buttonRight = transform->position.x;
    float buttonTop = transform->position.y - buttonHeight;
    float buttonBottom = transform->position.y;
    
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

} // namespace GameCore