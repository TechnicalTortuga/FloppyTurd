#include "ExplosionSystem.h"
#include "../../Engine/Core/GNLog.h"
#include "../Components/GameComponents.h"
#include <algorithm>

namespace GameCore {

ExplosionSystem::ExplosionSystem(Gnosis::ECS* ecsSystem)
    : m_ecsSystem(ecsSystem)
{
    GN_LOG_INFO("ExplosionSystem initialized");
}

ExplosionSystem::~ExplosionSystem() {
    ClearAll();
    GN_LOG_INFO("ExplosionSystem destroyed");
}

void ExplosionSystem::Update(float deltaTime) {
    if (!m_ecsSystem) return;

    // Update and remove completed explosions
    for (auto it = m_activeExplosions.begin(); it != m_activeExplosions.end(); ) {
        Entity explosion = *it;
        
        Sprite* sprite = m_ecsSystem->GetComponent<Sprite>(explosion);
        if (!sprite) {
            it = m_activeExplosions.erase(it);
            continue;
        }

        // Check if animation has completed (one-shot animations)
        if (sprite->hasCompleted && !sprite->loop) {
            // Destroy the explosion entity
            m_ecsSystem->DestroyEntity(explosion);
            it = m_activeExplosions.erase(it);
            GN_LOG_INFO("Explosion " + std::to_string(explosion) + " completed and removed");
        } else {
            ++it;
        }
    }
}

Entity ExplosionSystem::SpawnSmallExplosion(const Gnosis::GNVector2& position, float scale, float animationSpeed) {
    return CreateExplosion("blast_small", 7, position, scale, animationSpeed);
}

Entity ExplosionSystem::SpawnBigExplosion(const Gnosis::GNVector2& position, float scale, float animationSpeed) {
    return CreateExplosion("blast_big", 8, position, scale, animationSpeed);
}

Entity ExplosionSystem::CreateExplosion(const std::string& textureId, int frameCount, 
                                       const Gnosis::GNVector2& position, float scale, float animationSpeed) {
    if (!m_ecsSystem) return 0;

    Entity explosion = m_ecsSystem->CreateEntity();
    if (explosion == 0) {
        GN_LOG_ERROR("Failed to create explosion entity");
        return 0;
    }

    // Create transform at specified position
    Transform transform(position, 0.0f, Gnosis::GNVector2(scale, scale));
    m_ecsSystem->AddComponent<Transform>(explosion, transform);

    // Create animated sprite
    // IMPORTANT: Use raw frame dimensions - Transform.scale will handle scaling!
    // The renderer applies: renderSize = sprite.width * transform.scale
    // So we must NOT pre-scale the sprite dimensions
    const int FRAME_WIDTH = 32;
    const int FRAME_HEIGHT = 32;
    
    Sprite sprite(textureId, FRAME_WIDTH, FRAME_HEIGHT);
    sprite.isAnimated = true;
    sprite.frameWidth = FRAME_WIDTH;
    sprite.frameHeight = FRAME_HEIGHT;
    sprite.frameCount = frameCount;
    sprite.currentFrame = 0;
    sprite.frameTime = 0.3f / animationSpeed;  // Base 0.3s per frame (matches old implementation), adjusted by speed
    sprite.currentFrameTime = 0.0f;
    sprite.playing = true;
    sprite.loop = false;  // One-shot animation
    sprite.hasCompleted = false;
    sprite.visible = true;
    sprite.layer = 200;  // High layer to render on top of everything
    sprite.color = Gnosis::GNColor(255, 255, 255, 255);
    m_ecsSystem->AddComponent<Sprite>(explosion, sprite);

    // CRITICAL: Add ScrollSpeed(0.0f) to keep explosions in screen-space
    // Without this, explosions scroll with the camera and appear off-screen
    ScrollSpeed scrollSpeed(0.0f);
    m_ecsSystem->AddComponent<ScrollSpeed>(explosion, scrollSpeed);

    // Add to active explosions list
    m_activeExplosions.push_back(explosion);

    GN_LOG_INFO("Spawned " + textureId + " explosion at (" + 
               std::to_string(position.x) + ", " + std::to_string(position.y) + 
               ") with scale " + std::to_string(scale) + 
               " and speed " + std::to_string(animationSpeed));

    return explosion;
}

bool ExplosionSystem::HasActiveExplosions() const {
    return !m_activeExplosions.empty();
}

void ExplosionSystem::ClearAll() {
    if (!m_ecsSystem) return;

    for (Entity explosion : m_activeExplosions) {
        m_ecsSystem->DestroyEntity(explosion);
    }
    
    m_activeExplosions.clear();
    GN_LOG_INFO("All explosions cleared");
}

} // namespace GameCore