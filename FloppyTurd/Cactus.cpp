#include "Cactus.h"
#include "ResourceCompat.h"

Cactus::Cactus(Vector2 spawnPos, CactusVariant variant)
    : type(variant)
{
    using namespace Resources;

    const char* texturePath = nullptr;
    int frames = 1;
    float animSpeed = 0.10f;
    float scale = 1.0f;
    moveSpeed = 80.0f;
    collisionEnabled = true;

    switch (type)
    {
    case CactusVariant::A: texturePath = CactiA; break;
    case CactusVariant::B: texturePath = CactiB; break;
    case CactusVariant::C: texturePath = CactiC; break;
    case CactusVariant::D: texturePath = CactiD; break;
    case CactusVariant::E: texturePath = CactiE; break;
    case CactusVariant::BUSH: texturePath = CactiBush; break;
    case CactusVariant::DANCING_SMALL:
        texturePath = DancingCactiSmall;
        frames = 8;
        animSpeed = 0.08f;
        moveSpeed = 100.0f;
        collisionEnabled = false;
        break;
    case CactusVariant::DANCING_BIG:
        texturePath = DancingCacti;
        frames = 8;
        animSpeed = 0.08f;
        moveSpeed = 100.0f;
        collisionEnabled = false;
        break;
    case CactusVariant::DANCING_COWBOY:
        texturePath = DancingCactiCowboy;
        frames = 8;
        animSpeed = 0.08f;
        moveSpeed = 100.0f;
        collisionEnabled = false;
        break;
    }

    sprite = new Sprite(texturePath, frames, animSpeed, scale, spawnPos, AtlasCategory::ENVIRONMENT);

    float groundedY = 180.0f - sprite->GetHeight();
    sprite->SetPosition({ spawnPos.x, groundedY });
    this->pos = sprite->GetPosition();

    if (collisionEnabled)
        UpdateHitbox();
}

Cactus::~Cactus() { delete sprite; }

void Cactus::Update(float dt)
{
    pos.x -= moveSpeed * dt;
    sprite->SetPosition(pos);
    sprite->Update(dt);

    if (collisionEnabled)
        UpdateHitbox();
}

void Cactus::Draw() { sprite->Draw(pos.x, pos.y); }

void Cactus::UpdateHitbox()
{
    if (!collisionEnabled) return;

    float centerX = pos.x + sprite->GetWidth() * 0.5f;
    float bottomY = pos.y + sprite->GetHeight();

    switch (type)
    {
    case CactusVariant::A: hitbox = { centerX - 16, bottomY - 45, 32, 45 }; break;
    case CactusVariant::B: hitbox = { pos.x, pos.y, 32, 32 }; break;
    case CactusVariant::C: hitbox = { centerX - 26, bottomY - 66, 52, 66 }; break;
    case CactusVariant::D: hitbox = { centerX - 21, bottomY - 64, 42, 64 }; break;
    case CactusVariant::E: hitbox = { centerX - 26, bottomY - 60, 52, 60 }; break;
    case CactusVariant::BUSH: hitbox = { centerX - 7, bottomY - 10, 14, 10 }; break;
    default: break;
    }
}

std::vector<Rectangle> Cactus::GetHitboxes()
{
    return collisionEnabled ? std::vector<Rectangle>{hitbox} : std::vector<Rectangle>();
}

float Cactus::GetWidth() const { return static_cast<float>(sprite->GetWidth()); }

void Cactus::SetCollisionEnabled(bool enabled)
{
    collisionEnabled = enabled;
    if (!enabled) hitbox = { 0,0,0,0 };
}

void Cactus::SetPanSpeed(float speed)
{
    moveSpeed = speed;
}