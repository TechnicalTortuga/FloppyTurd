#include "Coin.h"
#include "Resources.h"
#include <raymath.h>

static AudioClip* s_GotCoin = nullptr;

Coin::Coin(Vector2 pos, CoinType type) : type(type) {
    position = pos;
    Init();
}

Coin::~Coin() {
    delete sprite;
}

void Coin::Init() {
    using namespace Resources;

    switch (type) {
    case CoinType::GOLDCOIN:
        sprite = new Sprite(GoldCoin, 10, 0.2f, 1.0f, position);
        value = 1;
        break;
    case CoinType::BLUECOIN:
        sprite = new Sprite(BlueCoin, 10, 0.2f, 1.0f, position);
        value = 2;
        break;
    case CoinType::REDCOIN:
        sprite = new Sprite(RedCoin, 10, 0.2f, 1.0f, position);
        value = 5;
        break;
    }

    // Generic 16x16 hitbox, adjust as needed
    hitbox = { position.x, position.y, 16, 16 };
}

void Coin::SetPanSpeed(float speed) {
    panSpeed = speed;
}

void Coin::Update(float deltaTime) {
    position.x -= panSpeed * deltaTime;
    if (sprite) sprite->SetPosition(position);  // update sprite position
    sprite->Update(deltaTime);

    // Update hitbox to match new position
    hitbox.x = position.x;
    hitbox.y = position.y;
}

void Coin::Draw() const {
    if (sprite)
        sprite->Draw(position.x, position.y);
}

Rectangle Coin::GetHitbox() const {
    return hitbox;
}

void Coin::OnPickup() {
    collected = true;
    AudioManager::GetInstance().PlaySoundEffect("GotCoin");
}
