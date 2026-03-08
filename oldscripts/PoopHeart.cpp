#include "PoopHeart.h"
#include "Resources.h"
#include "AudioManager.h"

PoopHeart::PoopHeart(Vector2 pos, PoopHeartType type) : type(type) {
    position = pos;

    switch (type) {
    case PoopHeartType::SMALL:
        sprite = new Sprite(Resources::PooHeart, 1, 0.1f, 1.0f, position);
        break;
    case PoopHeartType::BIG:
        sprite = new Sprite(Resources::PooHeartBig, 1, 0.1f, 1.0f, position);
        break;
    case PoopHeartType::INVISIBLE:
        sprite = new Sprite(Resources::PooHeartInvisible, 11, 0.05f, 1.0f, position);
        break;
    }

    hitbox = { position.x, position.y, 32, 32 };
}

PoopHeart::~PoopHeart() {
    delete sprite;
}

void PoopHeart::Update(float deltaTime) {
    position.x -= panSpeed * deltaTime;

    if (sprite) {
        if (type == PoopHeartType::SMALL) {
            float bobOffset = 4.0f * sinf(GetTime() * 2.5f + position.x * 0.05f);
            sprite->SetPosition({ position.x, position.y + bobOffset });
            hitbox.x = position.x;
            hitbox.y = position.y + bobOffset;
        }
        else {
            sprite->SetPosition(position);
            hitbox.x = position.x;
            hitbox.y = position.y;
        }

        sprite->Update(deltaTime);
    }
}

void PoopHeart::Draw() const {
    if (sprite)
        sprite->Draw(position.x, sprite->GetPosition().y); // draw at oscillated Y
}

Rectangle PoopHeart::GetHitbox() const {
    return hitbox;
}

void PoopHeart::OnPickup() {
    collected = true;

    if (type == PoopHeartType::BIG) {
        AudioManager::GetInstance().PlaySoundEffect("GotHealthBig");
    }
    else {
        AudioManager::GetInstance().PlaySoundEffect("GotHealth");
    }

    // Game logic to heal & trigger speedup will be handled where collision is detected
}

int PoopHeart::GetHealAmount() const {
    switch (type) {
    case PoopHeartType::BIG:
        return 3;   // three slices from a big heart
    case PoopHeartType::SMALL:
        return 1;   // one slice from a small heart
    default:
        return 27;   //  Maxed Out
    }
}

bool PoopHeart::IsInvisible() const {
    return type == PoopHeartType::INVISIBLE;
}

PoopHeartType PoopHeart::GetType() const {
    return type;
}