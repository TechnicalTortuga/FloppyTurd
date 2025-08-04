#include "SpriteSystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    SpriteSystem::SpriteSystem(Gnosis::ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates)
        : m_ecsCoordinator(ecsCoordinator)
        , m_delegates(delegates)
        , m_textureBasePath("")
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
        
        GN_LOG_DEBUG("SpriteSystem: Found " + std::to_string(entities.size()) + " entities with Transform and Sprite components");
        
        // Collect visible sprites with their layer info
        std::vector<std::pair<Gnosis::Entity, int>> visibleSprites;
        
        for (Gnosis::Entity entity : entities) {
            if (!IsEntityVisible(entity)) {
                continue;
            }
            
            Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
            if (sprite && sprite->visible) {
                visibleSprites.emplace_back(entity, sprite->layer);
                GN_LOG_DEBUG("SpriteSystem: Found visible sprite entity " + std::to_string(entity) + " with texture '" + sprite->textureId + "'");
            }
        }
        
        GN_LOG_DEBUG("SpriteSystem: Rendering " + std::to_string(visibleSprites.size()) + " visible sprites");
        
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
                GN_LOG_DEBUG("SpriteSystem: Rendering sprite entity " + std::to_string(entity) + " at position (" + 
                           std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + ")");
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

    void SpriteSystem::SetTextureBasePath(const std::string& basePath) {
        m_textureBasePath = basePath;
        GN_LOG_INFO("SpriteSystem: Set texture base path to '" + basePath + "'");
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
            
            GN_LOG_DEBUG("Animation frame advanced: currentFrame=" + std::to_string(sprite->currentFrame) + ", frameCount=" + std::to_string(sprite->frameCount) + ", playing=" + std::to_string(sprite->playing));
        }
    }

    void SpriteSystem::RenderSprite(Gnosis::Entity entity, const Transform& transform, const Sprite& sprite) {
        // Get or load the texture
        uint32_t textureHandle = GetOrLoadTexture(sprite.textureId, entity);
        if (textureHandle == 0) {
            GN_LOG_DEBUG("SpriteSystem: Texture not ready for entity " + std::to_string(entity) + ", texture '" + sprite.textureId + "'");
            return; // Skip if texture couldn't be loaded or is still loading
        }
        
        GN_LOG_DEBUG("SpriteSystem: Rendering sprite entity " + std::to_string(entity) + " with texture handle " + std::to_string(textureHandle));
        
        // Use platform delegate to render the sprite
        if (sprite.isAnimated && m_delegates.renderer.drawSpriteScaledWithSource) {
            // For animated sprites, use source rectangle to show current frame
            Gnosis::GNRectangle sourceRect = CalculateSourceRect(sprite);
            
            // Calculate scale factor relative to frame size (Metal renderer multiplies this by sourceWidth/sourceHeight)
            float scaleX = sprite.width / sprite.frameWidth;
            float scaleY = sprite.height / sprite.frameHeight;
            
            m_delegates.renderer.drawSpriteScaledWithSource(
                textureHandle,
                transform.position.x,
                transform.position.y,
                scaleX * transform.scale.x,
                scaleY * transform.scale.y,
                transform.rotation,
                sourceRect.x,
                sourceRect.y,
                sourceRect.width,
                sourceRect.height
            );
            
            GN_LOG_INFO("🎯 ANIMATED SPRITE RENDER: Entity " + std::to_string(entity) + 
                        " | Position: (" + std::to_string(transform.position.x) + ", " + std::to_string(transform.position.y) + ")" +
                        " | Scale: (" + std::to_string(scaleX * transform.scale.x) + ", " + std::to_string(scaleY * transform.scale.y) + ")" +
                        " | Rotation: " + std::to_string(transform.rotation) +
                        " | SourceRect: (" + std::to_string(sourceRect.x) + ", " + std::to_string(sourceRect.y) + ", " + 
                        std::to_string(sourceRect.width) + ", " + std::to_string(sourceRect.height) + ")" +
                        " | CurrentFrame: " + std::to_string(sprite.currentFrame) + "/" + std::to_string(sprite.frameCount) +
                        " | Texture: " + sprite.textureId);
        } else if (m_delegates.renderer.drawSpriteScaled) {
            // For non-animated sprites, use the old method (draw entire texture)
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
            
            GN_LOG_INFO("🎯 STATIC SPRITE RENDER: Entity " + std::to_string(entity) + 
                        " | Position: (" + std::to_string(transform.position.x) + ", " + std::to_string(transform.position.y) + ")" +
                        " | Scale: (" + std::to_string(scaleX * transform.scale.x) + ", " + std::to_string(scaleY * transform.scale.y) + ")" +
                        " | Rotation: " + std::to_string(transform.rotation) +
                        " | Texture: " + sprite.textureId);
        } else {
            GN_LOG_ERROR("SpriteSystem: No sprite rendering delegate available for entity " + std::to_string(entity));
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
            GN_LOG_ERROR("SpriteSystem: Texture file not found: " + fullPath);
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
        
        GN_LOG_ERROR("SpriteSystem: No texture loading interface available for '" + textureId + "'");
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
        // For iOS asset catalog, just return the texture ID directly
        // The asset catalog structure already organizes assets by folder
        return textureId;
    }
    
    void SpriteSystem::HandleTextureLoaded(GameCore::TextureData* textureData, const char* error, void* userData) {
        TextureLoadContext* context = static_cast<TextureLoadContext*>(userData);
        SpriteSystem* system = context->system;
        
        if (textureData && textureData->platformTexture) {
            // Store texture handle as uint32_t (platform-specific conversion)
        system->m_textureCache[context->textureId] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(textureData->platformTexture));
            GN_LOG_INFO("SpriteSystem: Successfully loaded texture '" + context->textureId + "' (" + std::to_string(textureData->width) + "x" + std::to_string(textureData->height) + ", platform: " + std::to_string(reinterpret_cast<uintptr_t>(textureData->platformTexture)) + ")");
        } else {
            GN_LOG_ERROR("SpriteSystem: Failed to load texture '" + context->textureId + "': " + (error ? error : "Unknown error"));
        }
        
        system->m_pendingTextures.erase(context->textureId);
        delete context;
    }
    
    void SpriteSystem::LoadTexture(const std::string& textureId, const std::string& filePath) {
        GN_LOG_INFO("SpriteSystem: Load texture '" + textureId + "' from '" + filePath + "'");
        
        // Check if already loaded
        if (m_textureCache.find(textureId) != m_textureCache.end()) {
            GN_LOG_INFO("SpriteSystem: Texture '" + textureId + "' already loaded");
            return;
        }
        
        // Verify file exists
        if (m_delegates.asset.fileExists && !m_delegates.asset.fileExists(filePath.c_str())) {
            GN_LOG_ERROR("SpriteSystem: Texture file not found: " + filePath);
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
