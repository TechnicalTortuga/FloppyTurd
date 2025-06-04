#include "Player.h"
#include "SoundManager.h"

Player::Player() {
    InitSprites();
    SetHeartMode(WHOLE);
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

void Player::SetHeartMode(HeartMode mode)
{
    if (mode == heartMode) return;
    int unscaledHeartsLeft = (liveSlices + ghostSlices) / heartMode;
    heartMode = mode;
    liveSlices = unscaledHeartsLeft * (int)heartMode;
    ghostSlices = 0;
}

void Player::Draw() {
    currentSprite->Draw(pos.x, pos.y);
    for (auto& projectile : projectiles) {
        projectile->Draw();
    }
    currentSelectedHat->Draw(formLevel, isShooting, hurtBuffer > 0.0f, pos);
}

void Player::Update(float deltaTime) {
    switch (playerstate) {
    case IDLE:
    case JUMPING:
        currentSprite->Update(deltaTime);
        break;
    case SHOOTING:
        currentSprite->Update(deltaTime);
        if (currentSprite->hasLoopedOnce()) {
            currentSprite->loopedOnce = false;
            playerstate = JUMPING;
            isShooting = false;
            ChangeForm(formLevel);
        }
        break;
    case DEAD:
    case HURT:
        currentSprite->Update(deltaTime);
        break;
    }

    velocity.y += GRAVITY;
    if (velocity.y > MAXVELOCITY)  velocity.y = MAXVELOCITY;
    if (velocity.y < -MAXVELOCITY) velocity.y = -MAXVELOCITY;
    pos.y += velocity.y;

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

    currentSprite->SetPosition(pos.x, pos.y);

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
            ChangeForm(formLevel);
        }
    }

    currentSelectedHat->Update(deltaTime, formLevel, isShooting, hurtBuffer > 0.0f, pos);
    int currentFrame = currentSprite->GetFrameIndex();
    currentSelectedHat->GetSprite(formLevel, isShooting, hurtBuffer > 0.0f)->SetFrameIndex(currentFrame);

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
    return Vector2{
        pos.x + 32,
        pos.y + 34
    };
}

int Player::GetTurdPoints() const {
    return turdPoints;
}

void Player::SetHat(Hat* hat)
{
    currentSelectedHat = hat;
}

void Player::PutTheHurtOn(int DAMAGE)
{
    if (hurtBuffer > 0.0f || isInvisible) return; // No damage during invincibility

    const int slicesToLose = 1;
    liveSlices = std::max(0, liveSlices - slicesToLose);

    if (liveSlices == 0)
    {
        isAlive = false;
        playerstate = DEAD;
    }

    hurtBuffer = 1.0f;
    playerstate = HURT;
    ChangeForm(formLevel);
    SoundManager::GetInstance().PlaySoundClip(hurtSound);
}

bool Player::SpendSliceForShoot()
{
    if (hollowTurds)
    {
        if (ghostSlices > 0) { --ghostSlices; return true; }
        if (liveSlices > 0) { --liveSlices; return true; }
        return false;
    }

    if (liveSlices > 0) { --liveSlices; return true; }
    return false;
}

void Player::InitSprites() {
    using namespace Resources;
    float playerScale = 1.0f;
    int idleFrames = 1;
    int jumpFrames = 6;
    int shootFrames = 8;

    idleSpriteTurdlet = new Sprite(TurdletIdle, idleFrames, 0.1f, playerScale, pos);
    jumpSpriteTurdlet = new Sprite(TurdletJump, jumpFrames, 0.1f, playerScale, pos);
    shootSpriteTurdlet = new Sprite(TurdletShoot, 5, 0.1f, playerScale, pos);

    idleSpriteTeen = new Sprite(TeenageTurdIdle, idleFrames, 0.1f, playerScale, pos);
    jumpSpriteTeen = new Sprite(TeenageTurdJump, jumpFrames, 0.1f, playerScale, pos);
    shootSpriteTeen = new Sprite(TeenageTurdShoot, shootFrames, 0.1f, playerScale, pos);

    idleSpriteBig = new Sprite(BigTurdIdle, idleFrames, 0.1f, playerScale, pos);
    jumpSpriteBig = new Sprite(BigTurdJump, jumpFrames, 0.1f, playerScale, pos);
    shootSpriteBig = new Sprite(BigTurdShoot, shootFrames, 0.1f, playerScale, pos);

    hurtSpriteTurdlet = new Sprite(TurdletHurt, jumpFrames, 0.1f, playerScale, pos);
    hurtSpriteTeen = new Sprite(TeenageTurdHurt, jumpFrames, 0.1f, playerScale, pos);
    hurtSpriteBig = new Sprite(BigTurdHurt, jumpFrames, 0.1f, playerScale, pos);

    formLevel = 0;
    ChangeForm(formLevel);

    hurtSound = LoadSound(Hurt);
}

void Player::ChangeForm(int newForm) {
    if (newForm < 0 || newForm > 2)
        return;

    formLevel = newForm;

    switch (formLevel) {
    case 0:
        if (playerstate == IDLE)
            currentSprite = idleSpriteTurdlet;
        else if (playerstate == JUMPING)
            currentSprite = jumpSpriteTurdlet;
        else if (playerstate == SHOOTING)
            currentSprite = shootSpriteTurdlet;
        else if (playerstate == HURT)
            currentSprite = hurtSpriteTurdlet;
        break;
    case 1:
        if (playerstate == IDLE)
            currentSprite = idleSpriteTeen;
        else if (playerstate == JUMPING)
            currentSprite = jumpSpriteTeen;
        else if (playerstate == SHOOTING)
            currentSprite = shootSpriteTeen;
        else if (playerstate == HURT)
            currentSprite = hurtSpriteTeen;
        break;
    case 2:
        if (playerstate == IDLE)
            currentSprite = idleSpriteBig;
        else if (playerstate == JUMPING)
            currentSprite = jumpSpriteBig;
        else if (playerstate == SHOOTING)
            currentSprite = shootSpriteBig;
        else if (playerstate == HURT)
            currentSprite = hurtSpriteBig;
        break;
    }
    currentSprite->ResetAnimation();

    if (currentSelectedHat != nullptr)
        currentSelectedHat->GetSprite(formLevel, isShooting, hurtBuffer > 0.0f)->ResetAnimation();
}

void Player::Jump() {
    if (isAlive)
        velocity.y -= JUMPVELOCITY;
}

void Player::Shoot()
{
    if (!shootingUnlocked) return;
    using namespace Resources;
    if (!isAlive || shootTimer > 0.0f || hurtBuffer > 0.0f) return;
    if (!SpendSliceForShoot()) return;

    isShooting = true;
    playerstate = SHOOTING;
    ChangeForm(formLevel);

    float projectileScale = 1.0f;
    float projectileSpeed = 300.0f;
    const char* spritePath = PoopSmall;
    if (formLevel == 1) spritePath = PoopMid;
    else if (formLevel == 2) spritePath = PoopLarge;

    Vector2 spawnPos = { pos.x + size.x / 2.f, pos.y + size.y / 2.f - 8 };
    Projectile* p = new Projectile(spawnPos, { 1,0 }, projectileSpeed,
        projectileScale, spritePath);
    projectiles.push_back(p);
    shootTimer = shootCooldown;
}

void Player::Revive() {
    playerstate = JUMPING;
    ChangeForm(formLevel);
    health = 100;
    isAlive = true;
    isShooting = false;

    liveSlices = hearts * (int)heartMode;
    ghostSlices = 0;

    pos = { 77.0f, 100.0f };
    velocity = { 0.0f, 0.0f };
    circleCenter = { pos.x + 32, pos.y + 34 };

    currentSprite->ResetAnimation();
}

void Player::SetHealth(int hp) {
    health = hp;
}

int Player::GetHealth() {
    return health;
}

const std::vector<Projectile*>& Player::GetProjectiles() const {
    return projectiles;
}

std::vector<Projectile*>& Player::GetProjectilesNonConst()
{
    return projectiles;
}

void Player::AddHeartSlice(int amount) {
    int maxSlices = hearts * (int)heartMode;
    liveSlices = std::min(liveSlices + amount, maxSlices);
}

void Player::SetMaxHearts(int max) {
    hearts = std::min(max, 9);
    liveSlices = std::min(liveSlices, hearts * (int)heartMode);
}

void Player::ActivateInvisibility(float duration) {
    isInvisible = true;
    invisibilityTimer = duration;
}