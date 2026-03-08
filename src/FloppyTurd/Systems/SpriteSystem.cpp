#include "SpriteSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    SpriteSystem::SpriteSystem(Gnosis::ECS* ecsCoordinator, const PlatformDelegates& delegates)
        : m_ecsCoordinator(ecsCoordinator)
        , m_delegates(delegates)
    {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("SpriteSystem: ECS coordinator is null");
        }
        
        GN_LOG_INFO("SpriteSystem: Initialized with integrated rendering");
    }

    void SpriteSystem::Update(float deltaTime) {
        if (!m_ecsCoordinator) {
            return;
        }

        // Get all entities with Sprite components
        auto entities = m_ecsCoordinator->GetEntitiesWithComponents<Sprite>();
        
        for (Gnosis::Entity entity : entities) {
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
            if (sprite && sprite->isAnimated && sprite->playing) {
                UpdateSpriteAnimation(sprite, deltaTime);
            }
        }
    }

    void SpriteSystem::Render() {
        // Rendering is handled by RenderSystem
        GN_LOG_DEBUG("SpriteSystem::Render() disabled - RenderSystem handles rendering");
        return;
    }

    void SpriteSystem::PlayAnimation(Gnosis::Entity entity) {
        if (!m_ecsCoordinator) {
            return;
        }
        
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        if (sprite && sprite->isAnimated) {
            sprite->Play();
        }
    }

    void SpriteSystem::PauseAnimation(Gnosis::Entity entity) {
        if (!m_ecsCoordinator) {
            return;
        }
        
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        if (sprite && sprite->isAnimated) {
            sprite->Pause();
        }
    }

    void SpriteSystem::StopAnimation(Gnosis::Entity entity) {
        if (!m_ecsCoordinator) {
            return;
        }
        
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        if (sprite && sprite->isAnimated) {
            sprite->Stop();
        }
    }

    void SpriteSystem::SetAnimationFrame(Gnosis::Entity entity, int frame) {
        if (!m_ecsCoordinator) {
            return;
        }
        
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        if (sprite && sprite->isAnimated) {
            sprite->SetFrame(frame);
        }
    }

    

    void SpriteSystem::UpdateSpriteAnimation(Sprite* sprite, float deltaTime) {
        if (!sprite || !sprite->isAnimated || !sprite->playing) {
            return;
        }
        
        sprite->currentFrameTime += deltaTime;
        
        // Check if it's time to advance to the next frame
        if (sprite->currentFrameTime >= sprite->frameTime) {
            sprite->currentFrameTime = 0.0f;
            sprite->currentFrame++;
            
            // Handle frame wrapping
            if (sprite->currentFrame >= sprite->frameCount) {
                if (sprite->loop) {
                    sprite->currentFrame = 0;
                } else {
                    sprite->currentFrame = sprite->frameCount - 1;
                    sprite->playing = false; // Stop animation if not looping
                    sprite->hasCompleted = true; // Mark animation as completed
                }
            }
            
            // Performance: Disabled per-frame animation logging
            // GN_LOG_DEBUG("Animation frame advanced: currentFrame=" + std::to_string(sprite->currentFrame) + ", frameCount=" + std::to_string(sprite->frameCount) + ", playing=" + std::to_string(sprite->playing));
        }
    }

    void SpriteSystem::RenderSprite(Gnosis::Entity /*entity*/, const Transform& /*transform*/, const Sprite& /*sprite*/) {
        // No-op: rendering handled by RenderSystem
    }
    
    
    
    Gnosis::GNRectangle SpriteSystem::CalculateSourceRect(const Sprite& sprite) const {
        if (!sprite.isAnimated) {
            // Static sprite - use entire texture
            return Gnosis::GNRectangle{0.0f, 0.0f, static_cast<float>(sprite.frameWidth), static_cast<float>(sprite.frameHeight)};
        }
        
        // Animated sprite - calculate frame position
        // Ensure we don't go out of bounds
        int safeFrameCount = sprite.frameCount > 0 ? sprite.frameCount : 1;
        int currentFrame = sprite.currentFrame % safeFrameCount;
        
        // Calculate frame position in spritesheet (horizontal layout)
        int frameX = currentFrame * sprite.frameWidth;
        int frameY = 0; // Single row layout - could be extended for multi-row sheets
        
        GN_LOG_DEBUG("SpriteSystem: CalculateSourceRect - frame " + std::to_string(currentFrame) + 
                    " of " + std::to_string(safeFrameCount) + 
                    " at (" + std::to_string(frameX) + "," + std::to_string(frameY) + ") " +
                    "size " + std::to_string(sprite.frameWidth) + "x" + std::to_string(sprite.frameHeight));
        
        return Gnosis::GNRectangle{
            static_cast<float>(frameX),
            static_cast<float>(frameY),
            static_cast<float>(sprite.frameWidth),
            static_cast<float>(sprite.frameHeight)
        };
    }
    
    
    
    bool SpriteSystem::IsEntityVisible(Gnosis::Entity entity) const {
        if (!m_ecsCoordinator) {
            return false;
        }
        
        // Basic visibility check - could be extended with frustum culling
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        return sprite && sprite->visible;
    }

    

} // namespace GameCore
