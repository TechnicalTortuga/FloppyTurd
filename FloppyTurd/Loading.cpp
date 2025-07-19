#include "Loading.h"
#include "PlatformAPI.h"
#include "ResourceManager.h"
#include "UIManager.h"
#include <thread>
#include <future>
#include "AudioStateManager.h"

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
        TraceLog(LOG_INFO, "[LOADING] Loading constructor STARTING");
        
        // Validate game pointer
        if (!game) {
            TraceLog(LOG_ERROR, "[LOADING] Loading constructor: game pointer is null");
            throw std::invalid_argument("Game pointer is null");
        }
        
        // Don't load the poophat texture here - defer until Initialize() is called
        // This avoids race condition with ResourceManager initialization
        poophat = Texture2D{0};
        
        TraceLog(LOG_INFO, "[LOADING] Loading constructor COMPLETED successfully");
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[LOADING] Exception in Loading constructor: %s", e.what());
        throw;
    } catch (...) {
        TraceLog(LOG_ERROR, "[LOADING] Unknown exception in Loading constructor");
        throw;
    }
}

void Loading::Initialize() {
    if (loadingStarted) return;
    loadingStarted = true;
    
    TraceLog(LOG_INFO, "[LOADING] Loading::Initialize() STARTING");
    
    // Validate game pointer
    if (!game) {
        TraceLog(LOG_ERROR, "[LOADING] ERROR: game pointer is null");
        return;
    }
    
    // Load the poophat texture now that ResourceManager should be initialized
    if (!poophatLoaded) {
        try {
            TraceLog(LOG_INFO, "[LOADING] Attempting to load poop_hat texture");
            
            poophat = ResourceManager::GetInstance().GetTexture("poop_hat");
            
            // Check if texture loaded successfully (platform-agnostic)
            if (poophat.id != 0) {
                TraceLog(LOG_INFO, "[LOADING] Successfully loaded poop_hat texture (id=%u, size=%dx%d)", 
                        poophat.id, poophat.width, poophat.height);
                poophatLoaded = true;
            } else {
                TraceLog(LOG_WARNING, "[LOADING] Failed to load poop_hat texture");
            }
        } catch (const std::exception& e) {
            TraceLog(LOG_ERROR, "[LOADING] Exception loading poop_hat texture: %s", e.what());
        }
    }
    
    // Start background font loading thread
    m_fontLoadingFuture = std::async(std::launch::async, [this]() {
        return LoadFontsInBackground();
    });
    
    TraceLog(LOG_INFO, "[LOADING] Background font loading started");
    TraceLog(LOG_INFO, "[LOADING] Loading::Initialize() COMPLETED");
}

bool Loading::LoadFontsInBackground() {
    TraceLog(LOG_INFO, "[LOADING] Background thread: Starting font loading");
    
    // Load Whacky Joe font using platform-specific implementation
    // On iOS: Uses Swift SDF font system through C++ interop bridge
    // On Desktop: Uses raylib LoadFont directly
    try {
        Font whackyJoeFont = ResourceManager::GetInstance().GetFont("whacky_joe_font");
        if (whackyJoeFont.baseSize > 0) {
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
    TraceLog(LOG_INFO, "[LOADING] Loading progress: %.0f%%", loadingProgress * 100.0f);
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
        TraceLog(LOG_INFO, "[LOADING] Loading complete, transitioning to main menu");
        
        // Set the game state to MAINMENU
        if (game) {
            game->SetGameState(MAINMENU);
            TraceLog(LOG_INFO, "[LOADING] Game state set to MAINMENU");
        }
    }
    
    // Log progress every few seconds to track loading
    static float lastProgressLog = 0.0f;
    lastProgressLog += deltaTime;
    if (lastProgressLog >= 2.0f) { // Log every 2 seconds
        lastProgressLog = 0.0f;
        TraceLog(LOG_INFO, "[LOADING] Progress: %.0f%%, fontLoadingComplete=%s, loadingComplete=%s", 
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
    if (this->poophat.id != 0) {
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
}