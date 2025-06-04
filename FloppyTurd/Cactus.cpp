#include "Cactus.h"

Cactus::Cactus(Vector2 spawnPos, CactusVariant variant)
    : type(variant)
{
    using namespace Resources;

    const char* texturePath = nullptr;
    int frames = 1;
    float animSpeed = 0.1f;
    float scale = 1.0f;

    switch (type) {
    case CactusVariant::A:
        texturePath = CactiA;
        break;
    case CactusVariant::B:
        texturePath = CactiB;
        break;
    case CactusVariant::C:
        texturePath = CactiC;
        break;
    case CactusVariant::D:
        texturePath = CactiD;
        break;
    case CactusVariant::E:
        texturePath = CactiE;
        break;
    case CactusVariant::BUSH:
        texturePath = CactiBush;
        break;
    }

    sprite = new Sprite(texturePath, frames, animSpeed, scale, spawnPos);

    // Grounding: move sprite to sit flush with y = 180
    float groundedY = 180.0f - sprite->GetHeight();
    pos = { spawnPos.x, groundedY };
    sprite->SetPosition(pos);

    UpdateHitbox();
}

Cactus::~Cactus() {
    delete sprite;
}

void Cactus::Update(float deltaTime) {
    pos.x -= 80.0f * deltaTime;
    sprite->SetPosition(pos);
    sprite->Update(deltaTime);
    UpdateHitbox();
}

void Cactus::Draw() {
    sprite->Draw(pos.x, pos.y);
    // Debug:
    //DrawRectangleLines(static_cast<int>(hitbox.x), static_cast<int>(hitbox.y), static_cast<int>(hitbox.width), static_cast<int>(hitbox.height), GREEN);
}

void Cactus::UpdateHitbox() {
    float centerX = pos.x + sprite->GetWidth() / 2.0f;
    float bottomY = pos.y + sprite->GetHeight();

    switch (type) {
    case CactusVariant::A:
        hitbox = { centerX - 16.0f, bottomY - 45.0f, 32.0f, 45.0f };
        break;
    case CactusVariant::B:
        hitbox = { pos.x, pos.y, 32.0f, 32.0f };
        break;
    case CactusVariant::C:
        hitbox = { centerX - 26.0f, bottomY - 66.0f, 52.0f, 66.0f };
        break;
    case CactusVariant::D:
        hitbox = { centerX - 21.0f, bottomY - 64.0f, 42.0f, 64.0f };
        break;
    case CactusVariant::E:
        hitbox = { centerX - 26.0f, bottomY - 60.0f, 52.0f, 60.0f };
        break;
    case CactusVariant::BUSH:
        hitbox = { centerX - 7.0f, bottomY - 10.0f, 14.0f, 10.0f };
        break;
    }
}

std::vector<Rectangle> Cactus::GetHitboxes() {
    return { hitbox };
}

float Cactus::GetWidth() const {
    return static_cast<float>(sprite->GetWidth());
}
