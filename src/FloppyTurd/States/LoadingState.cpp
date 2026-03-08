#include "LoadingState.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../../Engine/Core/GNLog.h"
#include "../../Engine/AssetPaths.h"
#include "../Components/GameComponents.h"
#include "../Game/PooperTrooperGame.h"
#include "../Systems/RenderSystem.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

namespace GameCore {

    // Static constants
    const float LoadingState::MIN_LOADING_DURATION = 1.5f;  // Minimum time to show loading screen (1.5 seconds)
    const float LoadingState::MAX_LOADING_DURATION = 3.0f;  // Maximum time before forcing transition (3 seconds)
    const float LoadingState::ROTATION_SPEED = 360.0f;  // degrees per second
    const float LoadingState::ORBIT_RADIUS = 50.0f;     // pixels from center
    
    // List of all game assets to preload
    namespace {
        // (Removed unused intermediate asset category vectors)

        // Fonts are loaded separately, no need to check data files
        
        // Combine all textures into one vector for loading (names + extensions only)
        const std::vector<std::string> GAME_TEXTURES = {
            "a",
            "achievementicon",
            "achievementiconunlocked",
            "ballcap",
            "ballcapbigturdjump",
            "ballcapbigturdshoot",
            "ballcapturdletjump",
            "ballcapturdletshoot",
            "Beret",
            "berethatbigturdjump",
            "berethatbigturdshoot",
            "berethatturdletjump",
            "berethatturdletshoot",
            "BigTurdHurt",
            "BigTurdIdle",
            "BigTurdJump",
            "BigTurdShoot",
            "BirdHurt",
            "BirdIdle",
            "BossLevelBackgroundMobile",
            "BossLevelPillarMobile",
            "blast_big",
            "blast_small",
            "BlueCoin",
            "BossBarFrame",
            "BossBarHealth",
            "BossBarHurt",
            "BossFloor",
            "BossWalls",
            "BottomPipeWide",
            "BottomPipeWideBlue",
            "BottomToilet",
            "BottomToiletGold-export",
            "BottomToiletGold",
            "BottomToiletSnow",
            "BrickWall",
            "CabinPainting",
            "Cacti",
            "CactiA",
            "CactiB",
            "CactiBush",
            "CactiC",
            "CactiD",
            "CactiE",
            "CastleLevelPainting",
            "CoinBag",
            "cowboyhat",
            "cowboyhatbigturd",
            "cowboyhatbigturdjump",
            "cowboyhatbigturdshoot",
            "cowboyhatteenage",
            "cowboyhatteenageshoot",
            "cowboyhatturdlet",
            "cowboyhatturdletjump",
            "cowboyhatturdletshoot",
            "crown",
            "crownhatbigturdjump",
            "crownhatbigturdshoot",
            "crownhatturdletjump",
            "crownhatturdletshoot",
            "DownArrow",
            "EmptyPainting",
            "F",
            "Floppy Poop Large",
            "Floppy Poop Mid",
            "Floppy Poop",
            "FloppyButtonBlue",
            "FloppyButtonBlueHover",
            "FloppyLogo",
            "PooperTrooperCreditsBackground",
            "PooperTrooperMorte",
            "flowerhat",
            "flowerhatbigturdjump",
            "flowerhatbigturdshoot",
            "flowerhatturdletjump",
            "flowerhatturdletshoot",
            "GameOverBackground",
            "GameOverScore",
            "GoldCoin",
            "HatFrame",
            "HatFrameDenied",
            "HatFrameHover",
            "HatFrameLocked",
            "HatFrameSelected",
            "Janitor",
            "JanitorSurprise",
            "JanitorSweep",
            "LeftArrow",
            "LeftArrowHover",
            "Level1BackLayerBackground",
            "Level1Clouds",
            "Level1FrontLayerBackground",
            "Level1MidLayerBackground",
            "Level3BackLayerBackground",
            "Level3FrontLayerBackground",
            "Level3MidLayerBackground",
            "LockedPainting",
            "MainMenu",
            "MainMenuMobile",
            "minusbutton",
            "minusbuttonclicked",
            "minusbuttonhover",
            "mutebutton",
            "mutebuttonclicked",
            "mutebuttonhover",
            "mutebuttonlocked",
            "mutebuttonlockedclicked",
            "mutebuttonlockedhover",
            "Outhouse",
            "OuthouseToilet",
            "PauseMenuBackground",
            "pinwheelbigturdjump",
            "pinwheelbigturdshoot",
            "PinwheelHat",
            "pinwheelturdletjump",
            "pinwheelturdletshoot",
            "plusbutton",
            "plusbuttonclicked",
            "plusbuttonhover",
            "PooHeart",
            "PooHeartBig",
            "PooHeartRainbow",
            "PooHeartRainbowBeam",
            "poophat",
            "poophatbigturdjump",
            "poophatbigturdshoot",
            "poophatturdletjump",
            "poophatturdletshoot",
            "RabbitKnightPainting",
            "ramsesbigturdjump",
            "ramsesbigturdshoot",
            "RamsesHat",
            "ramsesturdletjump",
            "ramsesturdletshoot",
            "RatBeachPainting",
            "RatCopterHurt",
            "RatCopterIdle",
            "Ratking",
            "RatkingAimBackArmOnly",
            "RatkingAimTorsoOnly",
            "RatkingAimTossArmOnly",
            "RatkingDeath",
            "RatkingHurt",
            "RatKingPainting",
            "RatkingStatic",
            "RatkingWalk",
            "RedCoin",
            "RightArrow",
            "RightArrowHover",
            "RiverWalkPainting",
            "samuraibigturdjump",
            "samuraibigturdshoot",
            "SamuraiHelmet",
            "samuraiturdletjump",
            "samuraiturdletshoot",
            "Score",
            "ScoreSmall",
            "sdf_atlas",
            "SewerLargeA",
            "SewerLargeB",
            "SewerLargeC",
            "SewerLargeD",
            "SewerLevelPainting",
            "shellhat",
            "shellhatbigturdjump",
            "shellhatbigturdshoot",
            "shellhatturdletjump",
            "shellhatturdletshoot",
            "SkillMenuBorder",
            "SkillPointInfoBackground",
            "Snowball",
            "Snowfall",
            "SnowLevelBackLayerBackground",
            "SnowLevelMidLayerBackground",
            "SnowLevelFrontLayerBackground",
            "SnowLevelFrontLayerTrees",
            "SnowLevelPainting",
            "SnowManChad",
            "SnowManChill",
            "SnowManGreen",
            "SnowManIdle",
            "SnowManThrow",
            "SpartanHelmet",
            "SpikeBall",
            "SpikeBallBase",
            "strawhat",
            "strawhatbigturdjump",
            "strawhatbigturdshoot",
            "strawhatturdletjump",
            "strawhatturdletshoot",
            "TeenageTurdHurt",
            "TeenageTurdIdle",
            "TeenageTurdJump",
            "TeenageTurdShoot",
            "ToiletPaperFlap",
            "ToiletPaperHit",
            "toiletpaperprojectile",
            "tophat",
            "tophatbigturdjump",
            "tophatbigturdshoot",
            "tophatturdletjump",
            "tophatturdletshoot",
            "TopPipeWide",
            "TopPipeWideBlue",
            "TopToilet",
            "TopToiletGold",
            "TopToiletSnow",
            "TorchPillar",
            "TryAgainBackground",
            "TurdHeart",
            "TurdHeart0HalfHollow",
            "TurdHeart0HalfHollow1Half",
            "TurdHeart0ThirdHollow",
            "TurdHeart0ThirdHollow1Third",
            "TurdHeart1Half",
            "TurdHeart1HalfHollow",
            "TurdHeart1Third",
            "TurdHeart1ThirdHollow",
            "TurdHeart1ThirdsHollow1Thirds",
            "TurdHeart2Thirds",
            "TurdHeart2ThirdsHollow",
            "TurdHeart2ThirdsHollow2Thirds",
            "TurdHeartHollow",
            "TurdHeartSmall",
            "TurdletHurt",
            "TurdletIdle",
            "TurdletJump",
            "TurdletShoot",
            "TurdPointButton",
            "TurdPointButtonAvailable",
            "TurdPointButtonClaimed",
            "TurdPointButtonFocused",
            "TurdPointMenu",
            "UpArrow",
            "ushanka",
            "ushankabigturdjump",
            "ushankabigturdshoot",
            "ushankaturdletjump",
            "ushankaturdletshoot",
            "volumemeterempty",
            "volumemeterfull",
            "xbuttonselected",
            "xbuttonunselected",
            "Whacky_Joe_msdf",
            "WhackyJoe_32",
            "castlebacklayerbackground",
            "curtains",
            "castlelevelfloortorch",
            "castlelevelchandelier"
        };
        
        // Combine all audio into one vector for loading (names + extensions only)
        const std::vector<std::string> GAME_SOUNDS = {
            "BeachLevel.mp3",
            "BossKill.mp3",
            "BossLevel.mp3",
            "BossThemeFast.mp3",
            "BossThemeLowHealth.mp3",
            "BossThemeSlow.mp3",
            "CastleFast.mp3",
            "CastleLevel.mp3",
            "CastleSlow.mp3",
            "DesertFast.mp3",
            "DesertLevel.mp3",
            "DesertSlow.mp3",
            "EndTheme.mp3",
            "PooperTrooperMenu.mp3",
            "PooperTrooperMenuAlt.mp3",
            "MenuFast.mp3",
            "ParkFast.mp3",
            "ParkLevel.mp3",
            "ParkSlow.mp3",
            "SewerFast.mp3",
            "SewerLevel.mp3",
            "SewerSlow.mp3",
            "SnowFast.mp3",
            "SnowLevel.mp3",
            "SnowSlow.mp3",
            "bubble.mp3",
            "confirm.mp3",
            "fart1.mp3",
            "fart10.mp3",
            "fart11.mp3",
            "fart2.mp3",
            "fart3.mp3",
            "fart4.mp3",
            "fart5.mp3",
            "fart6.mp3",
            "fart7.mp3",
            "fart8.mp3",
            "fart9.mp3",
            "gameover.mp3",
            "hurt.mp3",
            "pickup.mp3",
            "ratkill.mp3",
            "RatKingScreech.mp3",
            "tpkill.mp3",
            "BigHealthPickup.wav",
            "SmallHealthPickup.wav"
        };
    }

    LoadingState::LoadingState(Gnosis::ECS* ecsCoordinator)
        : m_ecsCoordinator(ecsCoordinator)
        , m_poopHatEntity(nullptr) {
    }

    void LoadingState::Enter() {
        GN_LOG_INFO("Entering Loading State");
        m_finished = false;
        m_assetsLoaded = false;
        m_loadingTimer = 0.0f;
        m_rotationAngle = 0.0f;
        
        // Create the rotating poop hat loading icon
        CreateLoadingEntities();
        
        // Start preloading all assets
        PreloadAssets();
    }

    void LoadingState::Exit() {
        GN_LOG_INFO("Exiting Loading State");
        // Clean up debug loading entities - we don't want them in main menu anymore
        DestroyLoadingEntities();
        GN_LOG_INFO("Cleaned up loading entities for cleaner main menu");
    }

    void LoadingState::Pause() {
        // Loading state doesn't need to handle pause
    }

    void LoadingState::Resume() {
        // Loading state doesn't need to handle resume
    }

    void LoadingState::Update(float deltaTime) {
        // Update loading timer
        m_loadingTimer += deltaTime;
        
        // Update rotation angle for the poop hat
        m_rotationAngle += ROTATION_SPEED * deltaTime;
        if (m_rotationAngle >= 360.0f) {
            m_rotationAngle -= 360.0f;
        }
        
        // Update poop hat position (circular orbit around center)
        UpdatePoopHatPosition();
        
        // Check if loading is complete
        // We need both the minimum loading time to have passed AND all assets to be loaded
        bool minLoadingTimeElapsed = (m_loadingTimer >= MIN_LOADING_DURATION);
        
        // After minimum loading time, check if all assets are loaded
        if (minLoadingTimeElapsed && !m_assetsLoaded) {
            m_assetsLoaded = true; // Assume true, will be set to false if any asset is missing
            
            extern GameCore::PooperTrooperGame* g_Game;
            if (g_Game) {
                const auto& delegates = g_Game->GetPlatformDelegates();
                // Access RenderSystem via ECS -> SystemManager
                GameCore::RenderSystem* renderSystem = nullptr;
                if (m_ecsCoordinator && m_ecsCoordinator->GetSystemManager()) {
                    renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem();
                }

                // Check textures via RenderSystem
                if (renderSystem) {
                    for (const auto& texture : GAME_TEXTURES) {
                        if (!renderSystem->IsTextureLoaded(texture)) {
                            m_assetsLoaded = false;
                            GN_LOG_DEBUG("Waiting for texture to load: " + texture);
                            break;
                        }
                    }
                } else if (delegates.asset.isAssetLoaded) {
                    // Fallback: platform check (should not be used in normal flow)
                    for (const auto& texture : GAME_TEXTURES) {
                        if (!delegates.asset.isAssetLoaded(texture.c_str())) {
                            m_assetsLoaded = false;
                            GN_LOG_DEBUG("Waiting for texture to load (fallback): " + texture);
                            break;
                        }
                    }
                }

                // Check sounds if textures are loaded
                if (m_assetsLoaded && delegates.asset.isAssetLoaded) {
                    for (const auto& sound : GAME_SOUNDS) {
                        if (!delegates.asset.isAssetLoaded(sound.c_str())) {
                            m_assetsLoaded = false;
                            GN_LOG_DEBUG("Waiting for sound to load: " + sound);
                            break;
                        }
                    }
                }

                // Fonts and data files are loaded separately, no need to check them here
            }
        }
        
        // Finish when minimum time elapsed AND assets loaded, OR when maximum time reached
        bool maxLoadingTimeElapsed = (m_loadingTimer >= MAX_LOADING_DURATION);
        if (maxLoadingTimeElapsed && !m_assetsLoaded) {
            GN_LOG_WARN("Maximum loading time reached, forcing transition despite asset loading failures");
        }
        
        bool wasFinished = m_finished;
        m_finished = (minLoadingTimeElapsed && m_assetsLoaded) || maxLoadingTimeElapsed;
        
        // Log detailed loading state for debugging
        GN_LOG_DEBUG("[LoadingState] Loading progress: timer=" + std::to_string(m_loadingTimer) + 
                    ", minElapsed=" + std::to_string(minLoadingTimeElapsed) + 
                    ", maxElapsed=" + std::to_string(maxLoadingTimeElapsed) + 
                    ", assetsLoaded=" + std::to_string(m_assetsLoaded) + 
                    ", finished=" + std::to_string(m_finished));
                    
        // Log when state changes from not finished to finished
        if (!wasFinished && m_finished) {
            GN_LOG_INFO("[LoadingState] State marked as FINISHED - ready for transition");
        }
        
        // When m_finished is true, the GameStateManager will detect this via IsFinished()
        // and call HandleStateTransition() which will create and push the MainMenuState
        
        // Log loading progress
        if (m_finished) {
            GN_LOG_INFO("Loading complete in " + std::to_string(m_loadingTimer) + " seconds");
        }
        
        // Update ECS systems
        if (m_ecsCoordinator) {
            m_ecsCoordinator->Update(deltaTime);
        }
    }

    void LoadingState::Render() {
        // No debug rendering - we want a clean loading screen
        if (m_poopHatEntity && m_ecsCoordinator) {
            Gnosis::Entity poopHatEntityId = static_cast<Gnosis::Entity>(reinterpret_cast<uintptr_t>(m_poopHatEntity));
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(poopHatEntityId);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(poopHatEntityId);
            if (transform && sprite) {
                // Rotation is updated in UpdatePoopHatPosition(); no changes here
            } else {
                GN_LOG_WARN("[LoadingState] Transform or Sprite component missing for poophat entity");
            }
        } else {
            GN_LOG_WARN("[LoadingState] Poophat entity or ECS coordinator is null");
        }
        
        // Render ECS entities as usual
        if (m_ecsCoordinator) {
            GN_LOG_DEBUG("[LoadingState] Calling ECS Render()");
            m_ecsCoordinator->Render();
        } else {
            GN_LOG_WARN("[LoadingState] ECS coordinator is null during render");
        }
    }

    void LoadingState::PreloadAssets() {
        extern GameCore::PooperTrooperGame* g_Game;
        if (!g_Game) {
            GN_LOG_ERROR("Cannot preload assets: Game instance is null");
            return;
        }
        
        const auto& delegates = g_Game->GetPlatformDelegates();
        if (!delegates.asset.loadTexture || !delegates.asset.loadAudio) {
            GN_LOG_ERROR("Cannot preload assets: Missing required asset loading delegates");
            return;
        }
        
        GN_LOG_INFO(
            "Starting preloading of all game assets (" +
            std::to_string(GAME_TEXTURES.size()) + " textures, " +
            std::to_string(GAME_SOUNDS.size()) + " sounds)"
        );
        
        // ⏱️ PERFORMANCE PROFILING: Measure texture preload time
        auto texturePreloadStart = std::chrono::high_resolution_clock::now();
        
        // Preload all textures via RenderSystem
        GameCore::RenderSystem* renderSystem = nullptr;
        if (m_ecsCoordinator && m_ecsCoordinator->GetSystemManager()) {
            renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem();
        }
        if (renderSystem) {
            GN_LOG_INFO("[LoadingState] ⏱️ [PROFILE] Texture preload START - queuing " + std::to_string(GAME_TEXTURES.size()) + " textures");
            renderSystem->PreloadTextures(GAME_TEXTURES);
            
            auto texturePreloadEnd = std::chrono::high_resolution_clock::now();
            auto texturePreloadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(texturePreloadEnd - texturePreloadStart).count();
            GN_LOG_INFO("[LoadingState] ⏱️ [PROFILE] Texture preload QUEUED in " + std::to_string(texturePreloadDuration) + "ms (actual GPU loading happens async)");
        } else {
            // Fallback to delegates (should not happen in unified architecture)
            for (const auto& texture : GAME_TEXTURES) {
                GN_LOG_DEBUG("Preloading texture (fallback): " + texture);
                delegates.asset.loadTexture(texture.c_str(),
                    [](GameCore::TextureData* tex, const char* error, void* /*userData*/) {
                        if (!tex) {
                            GN_LOG_ERROR("Failed to preload texture: " + std::string(error ? error : "Unknown error"));
                        }
                    },
                    nullptr
                );
            }
        }
        
        // Preload all audio
        for (const auto& sound : GAME_SOUNDS) {
            GN_LOG_DEBUG("Preloading audio: " + sound);
            delegates.asset.loadAudio(sound.c_str(),
                [](void* data, size_t /*size*/, const char* error, void* /*userData*/) {
                    if (!data) {
                        GN_LOG_ERROR("Failed to preload audio: " + std::string(error ? error : "Unknown error"));
                    }
                },
                nullptr
            );
        }

        // Fonts and data files are loaded separately by the system
        // No need to preload them here
        
        GN_LOG_INFO("Initiated preloading of all game assets");
    }

    void LoadingState::HandleInput() {
        // Loading state doesn't handle input
        // Could add skip functionality later if desired
    }

    void LoadingState::CreateLoadingEntities() {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("ECS coordinator is null in LoadingState");
            return;
        }
        
        GN_LOG_INFO("Creating loading screen entities");
        
        // Create rotating poop hat entity
        Gnosis::Entity poopHatEntityId = m_ecsCoordinator->CreateEntity();
        
        GN_LOG_INFO("Created entity with ID: " + std::to_string(poopHatEntityId));
        
        GN_LOG_INFO("[LoadingState] m_ecsCoordinator ptr: " + std::to_string(reinterpret_cast<uintptr_t>(m_ecsCoordinator)));
        GN_LOG_INFO("[LoadingState] IsEntityValid(" + std::to_string(poopHatEntityId) + "): " + (m_ecsCoordinator && m_ecsCoordinator->IsEntityValid(poopHatEntityId) ? "true" : "false"));
        // Add Transform component (position will be updated in UpdatePoopHatPosition)
        GN_LOG_INFO("[LoadingState] About to add Transform component to entity " + std::to_string(poopHatEntityId));
        // Initialize position at actual screen center (pixel coordinates)
        float initCenterX = 1179.0f * 0.5f;
        float initCenterY = 2556.0f * 0.5f;
        if (m_ecsCoordinator && m_ecsCoordinator->GetSystemManager() && m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem();
            ScreenInfo si = rs->GetScreenInfo();
            initCenterX = static_cast<float>(si.pixelWidth) * 0.5f;
            initCenterY = static_cast<float>(si.pixelHeight) * 0.5f;
        }
        Transform transform;
        transform.position = Gnosis::GNVector2(initCenterX, initCenterY);
        transform.scale = Gnosis::GNVector2(1.0f, 1.0f);
        m_ecsCoordinator->AddComponent<Transform>(poopHatEntityId, transform);
        GN_LOG_INFO("[LoadingState] Added Transform component to entity " + std::to_string(poopHatEntityId));
        
        GN_LOG_INFO("Added Transform component to entity " + std::to_string(poopHatEntityId) + " at position (" + 
                   std::to_string(transform.position.x) + ", " + std::to_string(transform.position.y) + ")");
        
        // Add Sprite component using just the asset name for iOS asset catalog
        GN_LOG_INFO("[LoadingState] About to add Sprite component to entity " + std::to_string(poopHatEntityId));
        // Try using a different texture to test if the issue is with the poophat texture specifically
        Sprite sprite("poophat", 256.0f, 256.0f); // Much bigger size!
        sprite.frameWidth = 16;  // Set correct frame dimensions for scaling
        sprite.frameHeight = 16; // Set correct frame dimensions for scaling
        sprite.layer = 10; // High layer for UI elements
        sprite.visible = true; // Ensure sprite is visible
        m_ecsCoordinator->AddComponent<Sprite>(poopHatEntityId, sprite);
        GN_LOG_INFO("[LoadingState] Added Sprite component to entity " + std::to_string(poopHatEntityId));
        
        // Add RotationRenderer component so poophat uses centered rendering for proper rotation
        RotationRenderer rotationRenderer(true);
        m_ecsCoordinator->AddComponent<RotationRenderer>(poopHatEntityId, rotationRenderer);
        GN_LOG_INFO("[LoadingState] Added RotationRenderer component to entity " + std::to_string(poopHatEntityId));
        
        GN_LOG_INFO("Added Sprite component to entity " + std::to_string(poopHatEntityId) + " with texture '" + 
                   sprite.textureId + "' size (" + std::to_string(sprite.width) + "x" + std::to_string(sprite.height) + 
                   ") frame (" + std::to_string(sprite.frameWidth) + "x" + std::to_string(sprite.frameHeight) + 
                   ") layer " + std::to_string(sprite.layer) + " visible " + (sprite.visible ? "true" : "false"));
        
        // Store entity ID (convert to void* for compatibility)
        m_poopHatEntity = reinterpret_cast<void*>(static_cast<uintptr_t>(poopHatEntityId));
        
        GN_LOG_INFO("Created poop hat entity with ID: " + std::to_string(poopHatEntityId));
    }

    void LoadingState::DestroyLoadingEntities() {
        GN_LOG_INFO("Destroying loading screen entities");
        
        // Destroy poop hat entity
        if (m_poopHatEntity && m_ecsCoordinator) {
            Gnosis::Entity poopHatEntityId = static_cast<Gnosis::Entity>(reinterpret_cast<uintptr_t>(m_poopHatEntity));
            
            if (m_ecsCoordinator->IsEntityValid(poopHatEntityId)) {
                m_ecsCoordinator->DestroyEntity(poopHatEntityId);
                GN_LOG_INFO("Destroyed poop hat entity with ID: " + std::to_string(poopHatEntityId));
            }
            
            m_poopHatEntity = nullptr;
        }
    }

    void LoadingState::UpdatePoopHatPosition() {
        if (!m_poopHatEntity || !m_ecsCoordinator) {
            return;
        }
        
        // Keep rotation angle in degrees for Metal renderer (it converts to radians internally)
        float rotationDegrees = m_rotationAngle;
        
        // Get actual screen center coordinates from RenderSystem ScreenInfo (pixels)
        float centerX = 400.0f; // Default fallback
        float centerY = 300.0f; // Default fallback
        if (m_ecsCoordinator && m_ecsCoordinator->GetSystemManager() && m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
            auto* renderSystem = m_ecsCoordinator->GetSystemManager()->GetRenderSystem();
            ScreenInfo screenInfo = renderSystem->GetScreenInfo();
            centerX = static_cast<float>(screenInfo.pixelWidth) * 0.5f;
            centerY = static_cast<float>(screenInfo.pixelHeight) * 0.5f;
        }
        
        // Update the entity's Transform component with correct center position and rotation
        Gnosis::Entity poopHatEntityId = static_cast<Gnosis::Entity>(reinterpret_cast<uintptr_t>(m_poopHatEntity));
        
        if (m_ecsCoordinator->IsEntityValid(poopHatEntityId)) {
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(poopHatEntityId);
            if (transform) {
                // Update both position (to correct center) and rotation each frame
                transform->position.x = centerX;
                transform->position.y = centerY;
                transform->rotation = rotationDegrees; // Spin the sprite on its own axis (Metal renderer expects degrees)
            }
        }
        
        // Log rotation for debugging (remove in final version)
        if (static_cast<int>(m_rotationAngle) % 90 == 0) {
            GN_LOG_DEBUG("Poop hat spinning at center: (" + std::to_string(centerX) + ", " + std::to_string(centerY) + 
                        ") rotation angle " + std::to_string(m_rotationAngle) + "°");
        }
    }

    float LoadingState::GetLoadingProgress() const {
        // Progress is either time-based or asset-loading based, whichever is slower
        float timeProgress = std::min(m_loadingTimer / MIN_LOADING_DURATION, 1.0f);
        
        // If we're still in the minimum loading time, just return time-based progress
        if (m_loadingTimer < MIN_LOADING_DURATION) {
            return timeProgress * 0.9f; // Cap at 90% until minimum time elapses
        }
        
        // After minimum time, check asset loading progress
        if (m_assetsLoaded) {
            return 1.0f; // Fully loaded
        }
        
        // If still loading assets, stay at 90-99%
        return 0.9f + (timeProgress * 0.1f);
    }

} // namespace GameCore