#include "SpriteSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    SpriteSystem::SpriteSystem(Gnosis::ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates)
        : m_ecsCoordinator(ecsCoordinator)
        , m_delegates(delegates)
        , m_textureBasePath("Assets/Graphics/")
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
        if (!m_ecsCoordinator) {
            return;
        }

        // Get all entities with both Transform and Sprite components
        auto entities = m_ecsCoordinator->GetEntitiesWithComponents<Transform, Sprite>();
        
        // Collect visible sprites with their layer info
        std::vector<std::pair<Gnosis::Entity, int>> visibleSprites;
        
        for (Gnosis::Entity entity : entities) {
            if (!IsEntityVisible(entity)) {
                continue;
            }
            
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
            if (sprite && sprite->visible) {
                visibleSprites.emplace_back(entity, sprite->layer);
            }
        }
        
        // Sort by layer (lower layers render first)
        std::sort(visibleSprites.begin(), visibleSprites.end(), 
                  [](const std::pair<Gnosis::Entity, int>& a, const std::pair<Gnosis::Entity, int>& b) {
                      return a.second < b.second;
                  });
        
        // Render all visible sprites in layer order
        for (const auto& entityLayer : visibleSprites) {
            Gnosis::Entity entity = entityLayer.first;
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(entity);
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
            
            if (transform && sprite) {
                RenderSprite(entity, *transform, *sprite);
            }
        }
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
                }
            }
        }
    }

    void SpriteSystem::RenderSprite(Gnosis::Entity entity, const Transform& transform, const Sprite& sprite) {
        // Get or load the texture
        uint32_t textureHandle = GetOrLoadTexture(sprite.textureId, entity);
        if (textureHandle == 0) {
            return; // Skip if texture couldn't be loaded or is still loading
        }
        
        // Calculate source rectangle for animated sprites
        Gnosis::GNRectangle sourceRect = CalculateSourceRect(sprite);
        
        // Use platform delegate to render the sprite
        if (m_delegates.renderer.drawSpriteScaled) {
            // Calculate scale from sprite dimensions
            float scaleX = sprite.width / sprite.frameWidth;
            float scaleY = sprite.height / sprite.frameHeight;
            
            m_delegates.renderer.drawSpriteScaled(
                textureHandle,
                transform.position.x,
                transform.position.y,
                scaleX * transform.scale.x,
                scaleY * transform.scale.y,
                transform.rotation
            );
        }
    }
    
    uint32_t SpriteSystem::GetOrLoadTexture(const std::string& textureId, Gnosis::Entity entity) {
        // Check if already loaded
        auto it = m_textureCache.find(textureId);
        if (it != m_textureCache.end()) {
            return it->second;
        }
        
        // Check if already loading
        if (m_pendingTextures.find(textureId) != m_pendingTextures.end()) {
            return 0; // Still loading
        }
        
        std::string fullPath = GetFullTexturePath(textureId);
        
        // Check if file exists using platform delegate
        if (m_delegates.asset.fileExists && !m_delegates.asset.fileExists(fullPath.c_str())) {
            GN_LOG_ERROR("SpriteSystem: Texture file not found: %s", fullPath.c_str());
            return 0;
        }
        
        const char* assetPath = m_delegates.asset.getAssetPath ? 
                               m_delegates.asset.getAssetPath(fullPath.c_str()) : 
                               fullPath.c_str();
        
        // Skip asset reuse check - delegate doesn't exist yet
        // Will be implemented when asset caching system is added
        
        // Use modern AssetDelegate interface for async texture loading
        if (m_delegates.asset.loadTexture) {
            m_pendingTextures[textureId] = true;
            
            TextureLoadContext* context = new TextureLoadContext(textureId, assetPath, this, entity);
            
            m_delegates.asset.loadTexture(assetPath, 
                [](GameCore::TextureData* textureData, const char* error, void* userData) {
                    SpriteSystem::HandleTextureLoaded(textureData, error, userData);
                }, context);
                
            return 0; // Will be available async
        }
        
        GN_LOG_ERROR("SpriteSystem: No texture loading interface available for '%s'", textureId.c_str());
        return 0;
    }
    
    Gnosis::GNRectangle SpriteSystem::CalculateSourceRect(const Sprite& sprite) const {
        if (!sprite.isAnimated) {
            // Static sprite - use entire texture
            return Gnosis::GNRectangle{0.0f, 0.0f, static_cast<float>(sprite.frameWidth), static_cast<float>(sprite.frameHeight)};
        }
        
        // Animated sprite - calculate frame position
        // Assuming horizontal sprite sheet layout
        int frameX = (sprite.currentFrame % (sprite.frameCount > 0 ? sprite.frameCount : 1)) * sprite.frameWidth;
        int frameY = 0; // Could be extended for multi-row sprite sheets
        
        return Gnosis::GNRectangle{
            static_cast<float>(frameX),
            static_cast<float>(frameY),
            static_cast<float>(sprite.frameWidth),
            static_cast<float>(sprite.frameHeight)
        };
    }
    
    std::string SpriteSystem::GetFullTexturePath(const std::string& textureId) const {
        return m_textureBasePath + textureId;
    }
    
    void SpriteSystem::HandleTextureLoaded(GameCore::TextureData* textureData, const char* error, void* userData) {
        TextureLoadContext* context = static_cast<TextureLoadContext*>(userData);
        SpriteSystem* system = context->system;
        
        if (textureData && textureData->platformTexture) {
            // Store texture handle as uint32_t (platform-specific conversion)
        system->m_textureCache[context->textureId] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(textureData->platformTexture));
            GN_LOG_INFO("SpriteSystem: Successfully loaded texture '%s' (%dx%d, platform: %p)", 
                       context->textureId.c_str(), textureData->width, textureData->height, 
                       textureData->platformTexture);
        } else {
            GN_LOG_ERROR("SpriteSystem: Failed to load texture '%s': %s", 
                       context->textureId.c_str(), error ? error : "Unknown error");
        }
        
        system->m_pendingTextures.erase(context->textureId);
        delete context;
    }
    
    void SpriteSystem::LoadTexture(const std::string& textureId, const std::string& filePath) {
        GN_LOG_INFO("SpriteSystem: Load texture '%s' from '%s'", textureId.c_str(), filePath.c_str());
        
        // Check if already loaded
        if (m_textureCache.find(textureId) != m_textureCache.end()) {
            GN_LOG_INFO("SpriteSystem: Texture '%s' already loaded", textureId.c_str());
            return;
        }
        
        // Verify file exists
        if (m_delegates.asset.fileExists && !m_delegates.asset.fileExists(filePath.c_str())) {
            GN_LOG_ERROR("SpriteSystem: Texture file not found: %s", filePath.c_str());
            return;
        }
    }
    // Enhanced AssetDelegate support will be added when needed
    
    bool SpriteSystem::IsEntityVisible(Gnosis::Entity entity) const {
        if (!m_ecsCoordinator) {
            return false;
        }
        
        // Basic visibility check - could be extended with frustum culling
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        return sprite && sprite->visible;
    }

} // namespace GameCore
