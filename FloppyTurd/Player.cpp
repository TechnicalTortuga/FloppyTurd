#include "Player.h"
#include "SoundManager.h"

Player::Player(Game* g) : game(g) {
	InitSprites();
	SetHeartMode(WHOLE);
	SetInitialHearts(1);
}

Player::~Player() {
	for (auto& projectile : projectiles) {
		delete projectile;
	}
	delete idleSpriteTurdlet;
	delete jumpSpriteTurdlet;
	delete shootSpriteTurdlet;
	delete idleSpriteTeen;
	delete jumpSpriteTeen;
	delete shootSpriteTeen;
	delete idleSpriteBig;
	delete jumpSpriteBig;
	delete shootSpriteBig;
	delete hurtSpriteTurdlet;
	delete hurtSpriteTeen;
	delete hurtSpriteBig;
	SoundManager::GetInstance().UnloadSoundClip(hurtSound);
}

void Player::SetInitialHearts(int difficultyIndex) {
	switch (difficultyIndex) {
	case 0: hearts = 3; break;
	case 1: hearts = 2; break;
	case 2: hearts = 1; break;
	default: hearts = 2; break;
	}
	liveSlices = hearts * (int)heartMode;
	ghostSlices = 0;
	SetMaxHearts(9);
}

void Player::SetHeartMode(HeartMode mode) {
	if (mode == heartMode) return;
	int unscaledHeartsLeft = (liveSlices + ghostSlices) / (int)heartMode;
	heartMode = mode;
	liveSlices = unscaledHeartsLeft * (int)heartMode;
	ghostSlices = 0;
}

void Player::EnableHeartMagnet(bool enable) {
	heartMagnet = enable;
}

void Player::EnableCoinShield(bool enable) {
	coinShieldEnabled = enable;
	if (enable) coinShieldActive = true;
}

bool Player::ConsumeCoinShield() {
	if (coinShieldEnabled && coinShieldActive && sessionCoins > 0) {
		coinShieldActive = false;
		int droppedCoins = sessionCoins;
		sessionCoins = 0;
		if (game && game->playing) {
			game->playing->TOTALCOINS -= droppedCoins;
			game->playing->GetStats().totalCoins = game->playing->TOTALCOINS;
			game->playing->savePending = true;
		}
		return true;
	}
	return false;
}

void Player::ActivateBigTurdBuff(float duration) {
	bigTurdBuffActive = true;
	bigTurdBuffTimer = duration;
	bigTurdFromMaxHearts = true; // Assume max hearts activation unless specified otherwise
	ChangeForm(2);
	shootCooldown = 0.3f;
	JUMPVELOCITY = 750.0f; // 25% boost: 600 * 1.25 = 750
}

void Player::Draw() {
	currentSprite->Draw(pos.x, pos.y);
	for (auto& projectile : projectiles) {
		projectile->Draw();
	}
	if (currentSelectedHat) currentSelectedHat->Draw(formLevel, isShooting, hurtBuffer > 0.0f, pos);
}

void Player::Update(float deltaTime) {
	switch (playerstate) {
	case IDLE:
	case JUMPING:
		if (currentSprite) {
			currentSprite->Update(deltaTime);
		}
		break;
	case SHOOTING:
		if (currentSprite) {
			currentSprite->Update(deltaTime);
			if (currentSprite->hasLoopedOnce()) {
				currentSprite->loopedOnce = false;
				playerstate = JUMPING;
				isShooting = false;
				ChangeForm(bigTurdBuffActive ? 2 : formLevel);
			}
		}
		break;
	case DEAD:
	case HURT:
		if (currentSprite) {
			currentSprite->Update(deltaTime);
		}
		break;
	}

	// DeltaTime-based physics (classic platformer style)
	// Fast fall logic: use higher gravity after apex
	if (velocity.y > 0) {
		if (!isFastFalling) isFastFalling = true;
	} else {
		if (isFastFalling) isFastFalling = false;
	}
	float gravityToApply = isFastFalling ? FAST_FALL_GRAVITY : GRAVITY;
	velocity.y += gravityToApply * deltaTime;
	if (velocity.y > MAX_FALL_SPEED) velocity.y = MAX_FALL_SPEED;
	if (velocity.y < MAX_JUMP_SPEED) velocity.y = MAX_JUMP_SPEED;
	pos.y += velocity.y * deltaTime;

	// Clamp at ceiling: if player hits the top, stop upward movement
	if (pos.y <= -size.y / 2 && velocity.y < 0) {
		pos.y = -size.y / 2;
		velocity.y = 0;
	}

	circleRadius = 11;
	circleCenter = { pos.x + 32, pos.y + 34 };

	if (circleCenter.y - circleRadius > 320) {
		if (isAlive) {
			PutTheHurtOn(33);
		}
		pos.y = 0;
		circleCenter.y = pos.y + 34;
	}
	if (circleCenter.y + circleRadius / 3 < 0) {
		velocity.y = 0;
	}

	if (currentSprite) {
		currentSprite->SetPosition(pos.x, pos.y);
	}

	for (size_t i = 0; i < projectiles.size(); ) {
		projectiles[i]->Update(deltaTime);
		if (projectiles[i]->IsOffScreen()) {
			delete projectiles[i];
			projectiles.erase(projectiles.begin() + i);
		}
		else {
			++i;
		}
	}

	if (shootTimer > 0.0f) {
		shootTimer -= deltaTime;
	}

	if (hurtBuffer > 0.0f) {
		hurtBuffer -= deltaTime;
		if (hurtBuffer <= 0.0f) {
			playerstate = JUMPING;
			ChangeForm(bigTurdBuffActive ? 2 : formLevel);
		}
	}

	if (bigTurdBuffActive) {
		bigTurdBuffTimer -= deltaTime;
		if (bigTurdBuffTimer <= 0.0f) {
			bigTurdBuffActive = false;
			shootCooldown = 0.5f;
			JUMPVELOCITY = 600.0f; // Back to normal jump height
			ChangeForm(bigTurdFromMaxHearts ? 0 : formLevel);
			bigTurdFromMaxHearts = false;
		}
	}

	if (currentSelectedHat) {
		currentSelectedHat->Update(deltaTime, formLevel, isShooting, hurtBuffer > 0.0f, pos);
		int currentFrame = currentSprite->GetFrameIndex();
		currentSelectedHat->GetSprite(formLevel, isShooting, hurtBuffer > 0.0f)->SetFrameIndex(currentFrame);
	}

	if (isInvisible) {
		invisibilityTimer -= deltaTime;
		if (invisibilityTimer <= 0.0f) {
			isInvisible = false;
		}
	}
}

float Player::GetCircleRadius() const {
	return 11.0f;
}

Vector2 Player::GetCircleCenter() const {
	return Vector2{ pos.x + 32, pos.y + 34 };
}

int Player::GetTurdPoints() const {
	return turdPoints;
}

void Player::SetHat(Hat* hat) {
	currentSelectedHat = hat;
}

bool Player::godMode = false;

void Player::PutTheHurtOn(int DAMAGE) {
	if (godMode) return;
	if (hurtBuffer > 0.0f || isInvisible) return;

	const int slicesToLose = 1;
	int newSlices = liveSlices - slicesToLose;

	if (newSlices <= 0 && coinShieldEnabled && coinShieldActive && ConsumeCoinShield()) {
		liveSlices = 1;
		hurtBuffer = 1.0f;
		playerstate = HURT;
		ChangeForm(bigTurdBuffActive ? 2 : formLevel);
		SoundManager::GetInstance().PlaySoundClip(hurtSound);
		return;
	}

	liveSlices = std::max(0, newSlices);

	if (liveSlices == 0) {
		isAlive = false;
		playerstate = DEAD;
		if (game && game->playing) {
			game->playing->GetStats().totalFlops++;
			game->playing->GetStats().Save();
		}
	}

	hurtBuffer = 1.0f;
	playerstate = HURT;
	ChangeForm(bigTurdBuffActive ? 2 : formLevel);
	SoundManager::GetInstance().PlaySoundClip(hurtSound);
}

bool Player::SpendCoinsForShoot() {
	const int shootCost = 1;
	if (sessionCoins >= shootCost) {
		sessionCoins -= shootCost;
		if (game && game->playing) {
			game->playing->TOTALCOINS -= shootCost;
		}
		return true;
	}
	return false;
}

void Player::InitSprites() {
	using namespace Resources;
	float playerScale = 1.0f;
	int idleFrames = 1;
	int jumpFrames = 6;
	int shootFrames = 5;

	idleSpriteTurdlet = new Sprite("turd/TurdletIdle.png", idleFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);
	jumpSpriteTurdlet = new Sprite("turd/TurdletJump.png", jumpFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);
	shootSpriteTurdlet = new Sprite("turd/TurdletShoot.png", shootFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);

	idleSpriteTeen = new Sprite("turd/TeenageTurdIdle.png", idleFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);
	jumpSpriteTeen = new Sprite("turd/TeenageTurdJump.png", jumpFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);
	shootSpriteTeen = new Sprite("turd/TeenageTurdShoot.png", shootFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);

	idleSpriteBig = new Sprite("turd/BigTurdIdle.png", idleFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);
	jumpSpriteBig = new Sprite("turd/BigTurdJump.png", jumpFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);
	shootSpriteBig = new Sprite("turd/BigTurdShoot.png", shootFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);

	hurtSpriteTurdlet = new Sprite("turd/TurdletHurt.png", jumpFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);
	hurtSpriteTeen = new Sprite("turd/TeenageTurdHurt.png", jumpFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);
	hurtSpriteBig = new Sprite("turd/BigTurdHurt.png", jumpFrames, 0.1f, playerScale, pos, AtlasCategory::PLAYER_SPRITES);

	formLevel = 0;
	ChangeForm(formLevel);

	hurtSound = LoadSound("sounds/hurt.mp3");
	hurtSound.length = 0; // Length will be set by platform implementation
}

void Player::ChangeForm(int newForm) {
	if (newForm < 0 || newForm > 2) {
		return;
	}

	formLevel = newForm;

	switch (formLevel) {
	case 0:
		if (playerstate == IDLE) {
			currentSprite = idleSpriteTurdlet;
		} else if (playerstate == JUMPING) {
			currentSprite = jumpSpriteTurdlet;
		} else if (playerstate == SHOOTING) {
			currentSprite = shootSpriteTurdlet;
		} else if (playerstate == HURT) {
			currentSprite = hurtSpriteTurdlet;
		}
		break;
	case 1:
		if (playerstate == IDLE) {
			currentSprite = idleSpriteTeen;
		} else if (playerstate == JUMPING) {
			currentSprite = jumpSpriteTeen;
		} else if (playerstate == SHOOTING) {
			currentSprite = shootSpriteTeen;
		} else if (playerstate == HURT) {
			currentSprite = hurtSpriteTeen;
		}
		break;
	case 2:
		if (playerstate == IDLE) {
			currentSprite = idleSpriteBig;
		} else if (playerstate == JUMPING) {
			currentSprite = jumpSpriteBig;
		} else if (playerstate == SHOOTING) {
			currentSprite = shootSpriteBig;
		} else if (playerstate == HURT) {
			currentSprite = hurtSpriteBig;
		}
		break;
	}
	
	if (currentSprite) {
		currentSprite->ResetAnimation();
	}

	if (currentSelectedHat) {
		currentSelectedHat->GetSprite(formLevel, isShooting, hurtBuffer > 0.0f)->ResetAnimation();
	}
}

void Player::Jump() {
	if (isAlive) {
		velocity.y -= JUMPVELOCITY;
		isFastFalling = false; // Reset fast fall on jump
		if (game && game->playing) {
			game->playing->GetStats().totalJumps++;
			game->playing->savePending = true;
		}
	}
}

void Player::Shoot() {
	if (!shootingUnlocked) return;
	if (!isAlive || shootTimer > 0.0f || hurtBuffer > 0.0f) return;
	if (!SpendCoinsForShoot()) return;

	isShooting = true;
	playerstate = SHOOTING;
	ChangeForm(bigTurdBuffActive ? 2 : formLevel);

	float projectileScale = bigTurdBuffActive ? 1.2f : 1.0f;
	float projectileSpeed = bigTurdBuffActive ? 400.0f : 300.0f;
	const char* spritePath = "turd/Floppy Poop.png";
	if (formLevel == 1) spritePath = "turd/Floppy Poop Mid.png";
	else if (formLevel == 2 || bigTurdBuffActive) spritePath = "turd/Floppy Poop Large.png";

	Vector2 spawnPos = { pos.x + size.x / 2.f, pos.y + size.y / 2.f - 8 };
	Projectile* p = new Projectile(spawnPos, { 1, 0 }, projectileSpeed, projectileScale, spritePath);
	projectiles.push_back(p);
	shootTimer = shootCooldown;
}

void Player::Revive() {
	playerstate = JUMPING;
	ChangeForm(formLevel);
	health = 100;
	isAlive = true;
	isShooting = false;
	if (game && game->mainMenu) {
		SetInitialHearts(game->mainMenu->GetDifficultyIndex());
	}
	else {
		SetInitialHearts(1);
	}
	pos = { 77.0f, 100.0f };
	velocity = { 0.0f, 0.0f };
	circleCenter = { pos.x + 32, pos.y + 34 };
	currentSprite->ResetAnimation();
	ResetSessionCoins();
	bigTurdBuffActive = false;
	shootCooldown = 0.5f;
	JUMPVELOCITY = 800.0f;
	GRAVITY = 800.0f;
	FAST_FALL_GRAVITY = 1200.0f;
	MAX_FALL_SPEED = 400.0f;
	MAX_JUMP_SPEED = -180.0f;
	coinShieldActive = coinShieldEnabled;
	isFastFalling = false; // Reset fast fall on revive
}

void Player::SetHealth(int hp) {
	health = hp;
}

void Player::ResetPosition() {
	pos = { 50.0f, 90.0f };
}

const std::vector<Projectile*>& Player::GetProjectiles() const {
	return projectiles;
}

std::vector<Projectile*>& Player::GetProjectilesNonConst() {
	return projectiles;
}

void Player::AddHeartSlice(int amount) {
	int maxSlices = hearts * (int)heartMode;
	int previousSlices = liveSlices;
	liveSlices = std::min(liveSlices + amount, maxSlices);
	if (liveSlices == maxSlices && previousSlices < maxSlices && !bigTurdBuffActive) {
		ActivateBigTurdBuff(15.0f);
	}
	if (game && game->playing) {
		game->playing->savePending = true;
	}
}

void Player::SetMaxHearts(int max) {
	hearts = std::min(max, 9);
	liveSlices = std::min(liveSlices, hearts * (int)heartMode);
	if (liveSlices == hearts * (int)heartMode && !bigTurdBuffActive) {
		ActivateBigTurdBuff(15.0f);
	}
}

void Player::ActivateInvisibility(float duration) {
	isInvisible = true;
	invisibilityTimer = duration;
}

void Player::AddCoins(int amount) {
	if (!game || !game->playing) return;
	sessionCoins += amount;
	game->playing->GetStats().totalCoins += amount;
	game->playing->TOTALCOINS += amount;
	game->playing->GetStats().Save();
}

void Player::ResetSessionCoins() {
	sessionCoins = 0;
}