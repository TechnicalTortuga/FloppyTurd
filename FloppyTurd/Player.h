#pragma once
#include "Sprite.h"
#include "Resources.h"
#include "GameSettings.h"
#include "Projectile.h"
#include <iostream>
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
	void ActivateBigTurdBuff(float duration);
	bool IsBigTurdActive() const { return bigTurdBuffActive; }
	float GetBigTurdTimer() const { return bigTurdBuffTimer; }

	void Draw();
	void Update(float deltaTime);
	void PutTheHurtOn(int DAMAGE);
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

	int GetHealth();

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

	bool isShooting;
	bool shootingUnlocked{ false };
	void EnableShooting(bool e) { shootingUnlocked = e; }
	bool CanShoot() const { return shootingUnlocked; }

	void AddHeartSlice(int amount);
	void SetMaxHearts(int max);
	void ActivateInvisibility(float duration);
	bool isAlive{ true };
	bool isInvisible = false;

	void AddCoins(int amount);
	int GetSessionCoins() const { return sessionCoins; }
	void ResetSessionCoins();

	void SetGame(Game* g) { game = g; }
	bool coinMagnet{ false };
	Vector2 circleCenter{ 77.0f, 100.0f };

private:
	PLAYERSTATE playerstate{ JUMPING };
	int health{ 99 };
	int formLevel{ 1 };
	int turdPoints{ 0 };

	float circleRadius{ 16 };
	float playerScale{ 1.0f };
	float MAXVELOCITY{ 4.0f };
	float GRAVITY{ 0.4f };
	float JUMPVELOCITY{ 20.0f };
	Vector2 pos{ 77.0f, 50.0f };
	Vector2 size{ 64.0f, 64.0f };
	Vector2 velocity{ 0.0f, 0.0f };

	Sprite* currentSprite;

	Sprite* idleSpriteTurdlet;
	Sprite* jumpSpriteTurdlet;
	Sprite* shootSpriteTurdlet;

	Sprite* idleSpriteTeen;
	Sprite* jumpSpriteTeen;
	Sprite* shootSpriteTeen;

	Sprite* idleSpriteBig;
	Sprite* jumpSpriteBig;
	Sprite* shootSpriteBig;

	Sprite* hurtSpriteTurdlet;
	Sprite* hurtSpriteTeen;
	Sprite* hurtSpriteBig;

	float hurtBuffer{ 1.0f };

	std::vector<Projectile*> projectiles;
	float shootCooldown{ 0.5f };
	float shootTimer{ 0.0f };

	Hat* currentSelectedHat;

	Sound hurtSound;

	int hearts{ 2 };
	HeartMode heartMode{ WHOLE };
	int liveSlices{ hearts * (int)heartMode };
	int ghostSlices{ 0 };
	bool hollowTurds{ false };

	float invisibilityTimer = 0.0f;

	bool bigTurdBuffActive{ false };
	float bigTurdBuffTimer{ 0.0f };

	int sessionCoins{ 0 };

	Game* game;
};