#pragma once
#include "Sprite.h"
#include "ResourceCompat.h"
#include "GameSettings.h"
#include "Projectile.h"
#include <vector>
#include "Hat.h"
#include "Game.h"

class Game;

class Player {
public:
	Player(Game* game = nullptr);
	~Player();

	enum HeartMode { WHOLE = 1, HALVES = 2, THIRDS = 3 };

	void SetHeartMode(HeartMode mode);
	HeartMode GetHeartMode() const { return heartMode; }
	int GetTotalHearts() const { return hearts; }
	int GetSlicesPerHeart() const { return (int)heartMode; }
	int GetSlicesLeft() const { return liveSlices; }
	int GetGhostSlicesLeft() const { return ghostSlices; }
	void EnableHollowTurds(bool enabled) { hollowTurds = enabled; }
	void SetInitialHearts(int difficultyIndex);
	void EnableCoinMagnet(bool enable) { coinMagnet = enable; }
	void EnableHeartMagnet(bool enable);
	void EnableCoinShield(bool enable);
	bool HasCoinShield() const { return coinShieldEnabled; }
	bool ConsumeCoinShield();
	void ActivateBigTurdBuff(float duration);
	bool IsBigTurdActive() const { return bigTurdBuffActive; }
	float GetBigTurdTimer() const { return bigTurdBuffTimer; }

	void Draw();
	void Update(float deltaTime);
	void PutTheHurtOn(int damage);
	bool SpendCoinsForShoot();
	float GetCircleRadius() const;
	Vector2 GetCircleCenter() const;
	int GetTurdPoints() const;
	void SetHat(Hat* hat);
	void InitSprites();
	void Jump();
	void Shoot();
	void Revive();
	void SetHealth(int hp);
	void ResetPosition();

	int GetHealth() const { return health; }
	Vector2 GetPosition() const { return pos; }
	void ChangeForm(int newForm);

	const std::vector<Projectile*>& GetProjectiles() const;
	std::vector<Projectile*>& GetProjectilesNonConst();

	enum PLAYERSTATE {
		IDLE,
		JUMPING,
		SHOOTING,
		HURT,
		DEAD,
	};

	bool isShooting{ false };
	bool shootingUnlocked{ false };
	void EnableShooting(bool e) { shootingUnlocked = e; }
	bool CanShoot() const { return shootingUnlocked; }

	void AddHeartSlice(int amount);
	void SetMaxHearts(int max);
	void ActivateInvisibility(float duration);
	bool isAlive{ true };
	bool isInvisible{ false };

	void AddCoins(int amount);
	int GetSessionCoins() const { return sessionCoins; }
	void ResetSessionCoins();

	void SetGame(Game* g) { game = g; }
	bool coinMagnet{ false };
	bool heartMagnet{ false };
	Vector2 circleCenter{ 77.0f, 100.0f };

	float GetJumpVelocity() const { return JUMPVELOCITY; }
	void SetJumpVelocity(float v) { JUMPVELOCITY = v; }
	float GetGravity() const { return GRAVITY; }
	void SetGravity(float v) { GRAVITY = v; }
	float GetFastFallGravity() const { return FAST_FALL_GRAVITY; }
	void SetFastFallGravity(float v) { FAST_FALL_GRAVITY = v; }
	float GetMaxJumpSpeed() const { return MAX_JUMP_SPEED; }
	void SetMaxJumpSpeed(float v) { MAX_JUMP_SPEED = v; }
	float GetMaxFallSpeed() const { return MAX_FALL_SPEED; }
	void SetMaxFallSpeed(float v) { MAX_FALL_SPEED = v; }

	static bool godMode;

private:
	PLAYERSTATE playerstate{ JUMPING };
	int health{ 100 };
	int formLevel{ 0 };
	int turdPoints{ 0 };

	float circleRadius{ 11.0f };
	float playerScale{ 1.0f };
	float MAX_FALL_SPEED{ 400.0f }; // Terminal downward velocity (pixels/sec)
	float MAX_JUMP_SPEED{ -180.0f }; // Terminal upward velocity (pixels/sec, negative)
	float GRAVITY{ 800.0f };       // Normal gravity (pixels/sec^2)
	float FAST_FALL_GRAVITY{ 1200.0f }; // Gravity after apex (pixels/sec^2)
	float JUMPVELOCITY{ 800.0f }; // Jump impulse (pixels/sec)
	Vector2 pos{ 77.0f, 50.0f };
	Vector2 size{ 64.0f, 64.0f };
	Vector2 velocity{ 0.0f, 0.0f };

	Sprite* currentSprite{ nullptr };

	Sprite* idleSpriteTurdlet{ nullptr };
	Sprite* jumpSpriteTurdlet{ nullptr };
	Sprite* shootSpriteTurdlet{ nullptr };

	Sprite* idleSpriteTeen{ nullptr };
	Sprite* jumpSpriteTeen{ nullptr };
	Sprite* shootSpriteTeen{ nullptr };

	Sprite* idleSpriteBig{ nullptr };
	Sprite* jumpSpriteBig{ nullptr };
	Sprite* shootSpriteBig{ nullptr };

	Sprite* hurtSpriteTurdlet{ nullptr };
	Sprite* hurtSpriteTeen{ nullptr };
	Sprite* hurtSpriteBig{ nullptr };

	float hurtBuffer{ 1.0f };

	std::vector<Projectile*> projectiles;
	float shootCooldown{ 0.5f };
	float shootTimer{ 0.0f };

	Hat* currentSelectedHat{ nullptr };

	Sound hurtSound;

	int hearts{ 2 };
	HeartMode heartMode{ WHOLE };
	int liveSlices{ hearts * (int)heartMode };
	int ghostSlices{ 0 };
	bool hollowTurds{ false };

	float invisibilityTimer{ 0.0f };

	bool bigTurdBuffActive{ false };
	float bigTurdBuffTimer{ 0.0f };
	bool bigTurdFromMaxHearts{ false }; // Tracks if Big Turd Buff was activated by max hearts

	bool coinShieldEnabled{ false };
	bool coinShieldActive{ false };

	int sessionCoins{ 0 };

	Game* game{ nullptr };

	bool isFastFalling{ false }; // True if player is in fast fall mode
};