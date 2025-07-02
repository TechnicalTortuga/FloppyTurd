#include "Explosion.h"
#include "TextureCache.h"

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