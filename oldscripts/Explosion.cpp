#include "Explosion.h"
#include "TextureCache.h"

Explosion::Explosion(const char* texturePath, Vector2 position, float scale)
    : position(position), scale(scale)
{
    // Initialize sprite with 8 frames, 32x32, 0.3s frame time
    sprite = std::make_shared<Sprite>(texturePath, 8, 0.3f, scale, position);
    SetTextureFilter(sprite->GetTexture(), TEXTURE_FILTER_POINT);
    TraceLog(LOG_INFO, "[Explosion] Created explosion with texture %s at (%.1f, %.1f)", texturePath, position.x, position.y);
}

Explosion::~Explosion()
{
    TraceLog(LOG_INFO, "[Explosion] Destroyed explosion");
}

void Explosion::Update(float deltaTime)
{
    if (sprite && !sprite->hasLoopedOnce())
    {
        sprite->Update(deltaTime);
    }
}

void Explosion::Draw() const
{
    if (sprite && !sprite->hasLoopedOnce())
    {
        sprite->Draw(position.x, position.y);
    }
}