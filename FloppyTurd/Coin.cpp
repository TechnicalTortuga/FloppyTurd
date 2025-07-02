#include "Coin.h"
#include "ResourceCompat.h"
#include <raymath.h>

static AudioClip* s_GotCoin = nullptr;

Coin::Coin(Vector2 pos, CoinType type, Vector2* playerPos) : type(type), playerPos(playerPos) {
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

	hitbox = { position.x, position.y, 16, 16 };
}

void Coin::SetPanSpeed(float speed) {
	panSpeed = speed;
}

void Coin::Update(float deltaTime) {
	// Default panning
	position.x -= panSpeed * deltaTime;

	// Coin magnet effect: move toward player if within 48px and magnet is active
	if (playerPos) {
		float distance = Vector2Distance(position, *playerPos);
		if (distance <= 48.0f) {
			Vector2 direction = Vector2Subtract(*playerPos, position);
			direction = Vector2Normalize(direction);
			position = Vector2Add(position, Vector2Scale(direction, 100.0f * deltaTime)); // Move at 100px/s
		}
	}

	if (sprite) sprite->SetPosition(position);
	sprite->Update(deltaTime);

	// Update hitbox
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