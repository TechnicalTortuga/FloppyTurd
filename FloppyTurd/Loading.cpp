#include "Loading.h"
#include "ResourceManager.h"
#include "UIManager.h"
#include <thread>
#include <iostream>
#include "AudioStateManager.h"
#include "FontCache.h"
#include <future>
#include "LogManager.h"

Loading::Loading(Game* game)
    : game(game)
    , rotationAngle(0.0f)
    , rotationTimer(0.0f)
    , loadingProgress(0.0f)
    , loadingStarted(false)
    , loadingComplete(false)
    , poophatLoaded(false)
    , m_fontLoadingFuture()
    , m_fontLoadingComplete(false)
{
    try {
        std::cout << "[DEBUG] Loading constructor STARTING" << std::endl;
        
        // Validate game pointer
        if (!game) {
            std::cerr << "[ERROR] Loading constructor: game pointer is null" << std::endl;
            throw std::invalid_argument("Game pointer is null");
        }
        
        // Don't load the poophat texture here - defer until Initialize() is called
        // This avoids race condition with ResourceManager initialization
        poophat = {};
        
        std::cout << "[DEBUG] Loading constructor COMPLETED successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in Loading constructor: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception in Loading constructor" << std::endl;
        throw;
    }
}

void Loading::Initialize() {
    if (loadingStarted) return;
    loadingStarted = true;
    
    TraceLog(LOG_INFO, "Loading::Initialize() STARTING");
    
    // Add iOS-specific logging
    GameLog::Log("[LOADING] Loading::Initialize() STARTING");
    
    // Add direct NSLog and printf calls for testing
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    #ifdef __OBJC__
        NSLog(@"[LOADING_NSLOG] Loading::Initialize() STARTING - direct NSLog call");
    #endif
#endif
    printf("[LOADING_PRINTF] Loading::Initialize() STARTING - direct printf call\n");
    
    // Validate game pointer
    if (!game) {
        TraceLog(LOG_ERROR, "Loading::Initialize(): game pointer is null");
        GameLog::Log("[LOADING] ERROR: game pointer is null");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        #ifdef __OBJC__
            NSLog(@"[LOADING_NSLOG] ERROR: game pointer is null");
        #endif
#endif
        printf("[LOADING_PRINTF] ERROR: game pointer is null\n");
        return;
    }
    
    // Load the poophat texture now that ResourceManager should be initialized
    if (!poophatLoaded) {
        try {
            TraceLog(LOG_INFO, "Loading::Initialize() - Attempting to load poop_hat texture");
            GameLog::Log("[LOADING] Attempting to load poop_hat texture");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            #ifdef __OBJC__
                NSLog(@"[LOADING_NSLOG] Attempting to load poop_hat texture");
            #endif
#endif
            printf("[LOADING_PRINTF] Attempting to load poop_hat texture\n");
            
            poophat = ResourceManager::GetInstance().GetTexture("poop_hat");
            
            // Debug logging to see what we got
            TraceLog(LOG_INFO, "Loading::Initialize() - poophat.id=%u, poophat.texture=%p, poophat.width=%d, poophat.height=%d", 
                    poophat.id, poophat.texture, poophat.width, poophat.height);
            GameLog::Log("[LOADING] poophat.id=%u, poophat.texture=%p, poophat.width=%d, poophat.height=%d", 
                        poophat.id, poophat.texture, poophat.width, poophat.height);
            
#if defined(__APPLE__) && TARGET_OS_IPHONE
            if (poophat.texture != nullptr) {
#else
            if (poophat.id != 0) {
#endif
                TraceLog(LOG_INFO, "Loading::Initialize() - Successfully loaded poop_hat texture");
                GameLog::Log("[LOADING] Successfully loaded poop_hat texture");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                #ifdef __OBJC__
                    NSLog(@"[LOADING_NSLOG] Successfully loaded poop_hat texture");
                #endif
#endif
                printf("[LOADING_PRINTF] Successfully loaded poop_hat texture\n");
                poophatLoaded = true;
            } else {
                TraceLog(LOG_WARNING, "Loading::Initialize() - Failed to load poop_hat texture");
                GameLog::Log("[LOADING] WARNING: Failed to load poop_hat texture");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
                #ifdef __OBJC__
                    NSLog(@"[LOADING_NSLOG] WARNING: Failed to load poop_hat texture");
                #endif
#endif
                printf("[LOADING_PRINTF] WARNING: Failed to load poop_hat texture\n");
            }
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "Loading::Initialize() - Exception loading poop_hat texture: %s", e.what());
            GameLog::Log("[LOADING] ERROR: Exception loading poop_hat texture: %s", e.what());
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
            #ifdef __OBJC__
                NSLog(@"[LOADING_NSLOG] ERROR: Exception loading poop_hat texture: %s", e.what());
            #endif
#endif
            printf("[LOADING_PRINTF] ERROR: Exception loading poop_hat texture: %s\n", e.what());
        }
    }
    
    // Start background font loading thread
    m_fontLoadingFuture = std::async(std::launch::async, [this]() {
        return LoadFontsInBackground();
    });
    
    TraceLog(LOG_INFO, "[LOADING] Background font loading started");
    
    TraceLog(LOG_INFO, "Loading::Initialize() COMPLETED");
    GameLog::Log("[LOADING] Loading::Initialize() COMPLETED");
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
    #ifdef __OBJC__
        NSLog(@"[LOADING_NSLOG] Loading::Initialize() COMPLETED");
    #endif
#endif
    printf("[LOADING_PRINTF] Loading::Initialize() COMPLETED\n");
    
}

bool Loading::LoadFontsInBackground() {
    TraceLog(LOG_INFO, "[LOADING] Background thread: Starting font loading");
    
    // Initialize font cache
    FontCache& cache = FontCache::GetInstance();
    if (!cache.Initialize()) {
        TraceLog(LOG_WARNING, "[LOADING] Background thread: Failed to initialize font cache");
    }
    
    // Load Whacky Joe font in background
    try {
        Font whackyJoeFont = ResourceManager::GetInstance().GetFont("whacky_joe_font");
        if (whackyJoeFont.baseSize > 0 && whackyJoeFont.glyphCount > 0) {
            TraceLog(LOG_INFO, "[LOADING] Background thread: Successfully loaded Whacky Joe font");
            return true;
        } else {
            TraceLog(LOG_WARNING, "[LOADING] Background thread: Whacky Joe font loading failed");
            return false;
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[LOADING] Background thread: Exception during font loading: %s", e.what());
        return false;
    }
}

// Remove all references to LoadResources and loadingThread
// Only use m_fontLoadingFuture for background font loading

void Loading::UpdateLoadingProgress(float progress) {
    // Ensure progress is between 0 and 1
    loadingProgress = std::min(1.0f, std::max(0.0f, progress));
    TraceLog(LOG_INFO, "Loading progress: %.0f%%", loadingProgress * 100.0f);
}

Loading::~Loading() {
    UnloadTexture(poophat);
}

void Loading::Update(float deltaTime) {
    if (!loadingStarted) {
        Initialize();
        return;
    }
    
    // Update loading animation (poophat spin) - counter-clockwise
    rotationAngle -= 180.0f * deltaTime; // 180 degrees per second counter-clockwise
    if (rotationAngle <= -360.0f) {
        rotationAngle += 360.0f;
    }
    
    // Check if background font loading is complete
    if (m_fontLoadingFuture.valid()) {
        auto status = m_fontLoadingFuture.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            bool fontLoaded = m_fontLoadingFuture.get();
            if (fontLoaded) {
                TraceLog(LOG_INFO, "[LOADING] Font loading completed successfully");
            } else {
                TraceLog(LOG_WARNING, "[LOADING] Font loading completed with warnings");
            }
            m_fontLoadingComplete = true;
        }
    }
    
    // Update loading progress - make it take longer so users can see the rotating poophat
    loadingProgress += deltaTime * 0.2f; // 5 seconds total (was 0.5f for 2 seconds)
    if (loadingProgress >= 1.0f) {
        loadingProgress = 1.0f;
        loadingComplete = true;
    }
    
    // Debug logging for loading progress
    TraceLog(LOG_INFO, "[LOADING DEBUG] Progress: %.2f, fontLoadingComplete=%s, loadingComplete=%s, rotationAngle=%.2f", 
             loadingProgress, 
             m_fontLoadingComplete ? "true" : "false", 
             loadingComplete ? "true" : "false",
             rotationAngle);
    
    // Check if loading is complete
    if (!loadingComplete && m_fontLoadingComplete && loadingProgress >= 1.0f) {
        loadingComplete = true;
        TraceLog(LOG_INFO, "Loading complete, transitioning to main menu");
        GameLog::Log("[LOADING] Loading complete, transitioning to main menu");
        
        // Set the game state to MAINMENU
        if (game) {
            game->SetGameState(MAINMENU);
            GameLog::Log("[LOADING] Game state set to MAINMENU");
        }
    }
    
    // Log progress every few seconds to track loading
    static float lastProgressLog = 0.0f;
    lastProgressLog += deltaTime;
    if (lastProgressLog >= 2.0f) { // Log every 2 seconds
        lastProgressLog = 0.0f;
        GameLog::Log("[LOADING] Progress: %.0f%%, fontLoadingComplete=%s, loadingComplete=%s", 
                    loadingProgress * 100.0f, 
                    m_fontLoadingComplete ? "true" : "false", 
                    loadingComplete ? "true" : "false");
    }

    // Update touch state so overlay can respond
    // if (touchControls) { // Removed touchControls instance
    //     touchControls->Update();
    //     touchControls->Draw();
    // }
}

void Loading::Draw() {
    UIManager& ui = UIManager::GetInstance();
    Rectangle safeAreaPx = ui.GetSafeArea(true);
    float screenWidthPx = safeAreaPx.x + safeAreaPx.width;
    float screenHeightPx = safeAreaPx.y + safeAreaPx.height;
    ClearBackground(BLACK);

    // --- Centered, large rotating poophat ---
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (this->poophat.texture != nullptr) {
#else
    if (this->poophat.id != 0) {
#endif
        Vector2 center = ui.GetPosition(UIAnchor::CENTER, {0, 0}, true);
        float poophatSize = fminf(safeAreaPx.width, safeAreaPx.height) * 0.18f;
        poophatSize = fmaxf(poophatSize, 96.0f);
        // Use CPU vertex transformation for poophat rotation
        Vector2 poophatPosition = {
            center.x - poophatSize / 2.0f,
            center.y - poophatSize / 2.0f
        };
        float poophatScale = poophatSize / (float)this->poophat.width;
        float rotationRadians = this->rotationAngle * DEG2RAD;
        DrawTextureEx(this->poophat, poophatPosition, rotationRadians, poophatScale, WHITE);
    }

    // --- Progress bar at bottom center ---
    float barWidth = safeAreaPx.width * 0.6f;
    float barHeight = fmaxf(12.0f, safeAreaPx.height * 0.025f);
    float barX = safeAreaPx.x + (safeAreaPx.width - barWidth) / 2.0f;
    float barY = safeAreaPx.y + safeAreaPx.height * 0.88f;
    Rectangle barRect = { barX, barY, barWidth, barHeight };
    Rectangle barFillRect = { barX, barY, barWidth * loadingProgress, barHeight };
    DrawRectangleRounded(barRect, 0.4f, 12, ColorAlpha(WHITE, 0.18f));
    DrawRectangleRounded(barFillRect, 0.4f, 12, WHITE);

    // --- Loading text removed - just show the progress bar ---

    // --- Progress percent below bar ---
    // char progressText[32];
    // snprintf(progressText, sizeof(progressText), "%.0f%%", loadingProgress * 100.0f);
    // int percentWidth = MeasureText(progressText, fontSize);
    // float percentY = barY + barHeight + fontSize * 0.5f;
    // DrawText(progressText, (screenWidthPx - percentWidth) / 2, percentY, fontSize, WHITE);

    // DEBUG: Draw touch state overlay for debugging
#ifdef PLATFORM_MOBILE
    TouchControls* tc = game->GetTouchControls();
    if (tc) {
        bool touchActive = tc->IsPrimaryInputDown();
        bool touchPressed = tc->IsPrimaryInputPressed();
        Vector2 touchPos = tc->GetPrimaryInputPosition();
        
        // Log touch state for debugging (only when state changes)
        static bool lastTouchActive = false;
        static bool lastTouchPressed = false;
        if (touchActive != lastTouchActive || touchPressed != lastTouchPressed) {
            LogManager::GetInstance().Log("Touch state changed - Active: " + std::string(touchActive ? "true" : "false") + 
                                        ", Pressed: " + std::string(touchPressed ? "true" : "false") + 
                                        ", Pos: (" + std::to_string((int)touchPos.x) + "," + std::to_string((int)touchPos.y) + ")", "TOUCH");
            lastTouchActive = touchActive;
            lastTouchPressed = touchPressed;
        }
        
        // Draw fullscreen white overlay if touch is active
        if (touchActive) {
            PlatformLayer::GetInstance().DrawRectangle(0, 0, screenWidthPx, screenHeightPx, ColorToUInt(ColorAlpha(WHITE, 0.18f)));
        }
        // Draw green overlay if pressed (for extra feedback)
        if (touchPressed) {
            PlatformLayer::GetInstance().DrawRectangle(0, 0, screenWidthPx, screenHeightPx, ColorToUInt(ColorAlpha(GREEN, 0.18f)));
        }
        // Draw debug rectangle at touch position
        if (touchActive) {
            PlatformLayer::GetInstance().DrawRectangle(touchPos.x - 25, touchPos.y - 25, 50, 50, ColorToUInt(ColorAlpha(GREEN, 0.8f)));
        }
    } else {
        LogManager::GetInstance().Log("TouchControls is null in Loading::Draw()", "ERROR");
    }
#endif
    
    // Draw the touch overlay so user sees feedback
    // if (touchControls) touchControls->Draw(); // Removed touchControls instance
}