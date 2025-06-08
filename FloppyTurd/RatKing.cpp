#include "RatKing.h"
#include "Resources.h"
#include <raymath.h>
#include "AudioClip.h"
#include "LevelManager.h"
#include "RatCopter.h"

float SmoothAngleLerp(float current, float target, float amount)
{
    float diff = fmodf((target - current + 540.0f), 360.0f) - 180.0f;
    return current + diff * amount;
}

RatKing::RatKing(Vector2 startPosition)
    : walkRangeMin(100.0f), walkRangeMax(220.0f), walkSpeed(20.0f),
    hurtTimer(0.0f), hasFlashedHurt(false), hasSpawnedMinions(false)
{
    position = startPosition;
    health = 200;
    scale = 1.0f;
    isActive = true;
    currentState = IDLE;

    using namespace Resources;
    idleSprite = new Sprite(RatKingIdle, 1, 1.0f, scale, position);
    walkSprite = new Sprite(RatKingWalk, 8, 0.15f, scale, position);
    hurtSprite = new Sprite(RatKingHurt, 6, 0.2f, scale, position);
    deathSprite = new Sprite(RatKingDeath, 6, 0.3f, scale, position);
    torsoSprite = new Sprite(RatKingAimTorso, 7, 0.2f, scale, position);
    frontArmSprite = new Sprite(RatKingAimFrontArm, 7, 0.2f, scale, position);
    backArmSprite = new Sprite(RatKingAimBackArm, 7, 0.2f, scale, position);

    SetTextureFilter(idleSprite->GetTexture(), TEXTURE_FILTER_POINT);
    SetTextureFilter(walkSprite->GetTexture(), TEXTURE_FILTER_POINT);
    SetTextureFilter(hurtSprite->GetTexture(), TEXTURE_FILTER_POINT);
    SetTextureFilter(deathSprite->GetTexture(), TEXTURE_FILTER_POINT);
    SetTextureFilter(torsoSprite->GetTexture(), TEXTURE_FILTER_POINT);
    SetTextureFilter(frontArmSprite->GetTexture(), TEXTURE_FILTER_POINT);
    SetTextureFilter(backArmSprite->GetTexture(), TEXTURE_FILTER_POINT);

    shoulder = { position.x + 60, position.y + 38 };
    currentSprite = idleSprite;
}

RatKing::~RatKing()
{
    delete idleSprite;
    delete walkSprite;
    delete hurtSprite;
    delete deathSprite;
    delete torsoSprite;
    delete frontArmSprite;
    delete backArmSprite;

    for (auto& tp : projectiles) delete tp;
}

void RatKing::Draw()
{
    if (currentState == PREPARING_ATTACK || currentState == ATTACKING)
        DrawAiming();
    else
        DrawNormal();

    for (auto& tp : projectiles) tp->Draw();
}

void RatKing::DrawNormal()
{
    if (currentSprite) currentSprite->Draw(position.x, position.y);
}

void RatKing::DrawAiming()
{
    currentArmAngleDeg = SmoothAngleLerp(currentArmAngleDeg, RAD2DEG * aimingAngle, 0.25f);

    int frameWidth = 128;
    int frameHeight = 128;

    int frontFrame = frontArmSprite->GetFrameIndex();
    int backFrame = backArmSprite->GetFrameIndex();

    Rectangle dest = { shoulder.x, shoulder.y, frameWidth * scale, frameHeight * scale };
    Vector2 origin = { 64.0f, 64.0f };
    float visualOffset = -180.0f;

    Rectangle backRec = { (float)(backFrame * frameWidth), 0, (float)frameWidth, (float)frameHeight };
    DrawTexturePro(backArmSprite->GetTexture(), backRec, dest, origin, currentArmAngleDeg + visualOffset, WHITE);

    torsoSprite->Draw(position.x, position.y);

    Rectangle frontRec = { (float)(frontFrame * frameWidth), 0, (float)frameWidth, (float)frameHeight };
    DrawTexturePro(frontArmSprite->GetTexture(), frontRec, dest, origin, currentArmAngleDeg + visualOffset, WHITE);

    if (currentState == PREPARING_ATTACK)
        DrawLockOnIndicator();
}

void RatKing::DrawLockOnIndicator()
{
    int dots = 40;
    float spacing = 8.0f;
    float progress = Clamp(lockOnTimer / lockOnDuration, 0.0f, 1.0f);

    for (int i = 0; i < dots; ++i) {
        float fill = (float)i / (float)dots;
        if (fill > progress) break;

        Color color = ColorLerp(YELLOW, RED, fill);
        Vector2 dot = {
            shoulder.x + cos(aimingAngle) * i * spacing,
            shoulder.y + sin(aimingAngle) * i * spacing
        };
        DrawCircleV(dot, 2, color);
    }
}

void RatKing::Update(float deltaTime)
{
    switch (currentState) {
    case IDLE: HandleIdle(deltaTime); break;
    case WALKING: HandleWalking(deltaTime); break;
    case PREPARING_ATTACK: HandlePreparingAttack(deltaTime); break;
    case ATTACKING: HandleAttacking(deltaTime); break;
    case HURT: HandleHurt(deltaTime); break;
    case DEATH: HandleDeath(deltaTime); break;
    }

    float hpPercent = (float)health / 200.0f;

    while (health <= nextMinionHealthThreshold && nextMinionHealthThreshold > 0) {
        if (health >= 50)
            SpawnMinionWave(4);
        else
            SpawnMinionWave(6);
        nextMinionHealthThreshold -= 30;
    }

    for (size_t i = 0; i < projectiles.size(); ) {
        projectiles[i]->Update(deltaTime);
        if (projectiles[i]->IsOffScreen()) {
            delete projectiles[i];
            projectiles.erase(projectiles.begin() + i);
        }
        else ++i;
    }

    if (currentSprite && currentState != PREPARING_ATTACK)
        currentSprite->Update(deltaTime);

    if (currentState == HURT && hurtBuffer > 0)
        --hurtBuffer;

    if (currentState == PREPARING_ATTACK)
        lockOnTimer += deltaTime;
}

void RatKing::SpawnMinionWave(int count) {
    for (int i = 0; i < count; ++i) {
        float y = (float)GetRandomValue(10, 90);  // Top half (roughly)
        Vector2 spawnPos = { 320.0f + i * 32.0f, y };
        auto rat = std::make_shared<RatCopter>(spawnPos, 80.0f);
        rat->SetTarget(LevelManager::GetInstance()->GetPlayerPosition());
        LevelManager::GetInstance()->GetEnemies().push_back(rat);
    }
}

void RatKing::ChangeState(State newState)
{
    currentState = newState;
    hasFiredProjectile = false;

    switch (newState)
    {
    case IDLE: currentSprite = idleSprite; break;
    case WALKING: currentSprite = walkSprite; break;
    case PREPARING_ATTACK:
        currentSprite = torsoSprite;
        lockOnTimer = 0.0f;
        hasLockedOn = false;
        aimingAngle = PI;
        currentArmAngleDeg = 180.0f;
        break;
    case ATTACKING:
        currentSprite = torsoSprite;
        attackTimer = 0.0f;
        break;
    case HURT:
        currentSprite = hurtSprite;
        hasFlashedHurt = false;
        break;
    case DEATH:
        currentSprite = deathSprite;
        break;
    }

    if (currentSprite)
        currentSprite->ResetAnimation();
}

void RatKing::HandleIdle(float dt)
{
    static float timer = 0.0f;
    timer += dt;
    if (timer > 2.0f) {
        timer = 0.0f;
        ChangeState(GetRandomValue(0, 1) == 0 ? WALKING : PREPARING_ATTACK);
    }
}

void RatKing::HandleWalking(float dt)
{
    static float dir = 1.0f;
    position.x += dir * walkSpeed * dt;

    if (position.x < walkRangeMin) dir = 1.0f;
    else if (position.x > walkRangeMax) dir = -1.0f;

    static float walkTime = 0.0f;
    walkTime += dt;
    if (walkTime > 3.0f) {
        walkTime = 0.0f;
        ChangeState(IDLE);
    }
}

void RatKing::HandlePreparingAttack(float dt)
{
    shoulder = { position.x + 64, position.y + 64 };

    float healthPercent = (float)health / 200.0f;

    // Dynamic aim delay scaling
    if (healthPercent <= 0.40f) lockOnDuration = 0.5f;
    else if (healthPercent <= 0.5f) lockOnDuration = 1.0f;
    else lockOnDuration = 2.0f;

    // Trigger low health music
    if (!hasTriggeredLowHealthMusic && healthPercent <= 0.40f)
    {
        hasTriggeredLowHealthMusic = true;
        lowHealthTriggeredAt = GetTime();
        TraceLog(LOG_INFO, "RatKing LOW HEALTH triggered at %.5f", lowHealthTriggeredAt);
    }

    Vector2 toPlayer = Vector2Subtract(playerPosition, shoulder);
    float targetAngle = atan2(toPlayer.y, toPlayer.x);
    float resultDeg = SmoothAngleLerp(RAD2DEG * aimingAngle, RAD2DEG * targetAngle, dt * 4.0f);
    resultDeg = Clamp(resultDeg, 120.0f, 240.0f);
    aimingAngle = DEG2RAD * resultDeg;

    lockOnTimer += dt;
    if (lockOnTimer >= lockOnDuration) {
        hasLockedOn = true;
        aimingAngle = atan2(toPlayer.y, toPlayer.x);
        ChangeState(ATTACKING);
    }
}

void RatKing::HandleAttacking(float dt)
{
    attackTimer += dt;
    torsoSprite->Update(dt);
    frontArmSprite->Update(dt);
    backArmSprite->Update(dt);

    if (!hasFiredProjectile && torsoSprite->HasPlayedFrames(6)) {
        hasFiredProjectile = true;
        SpawnProjectile();

        // Dual throw below 10% HP
        if (health <= 20 && GetRandomValue(0, 1) == 1) {
            float offset = DEG2RAD * GetRandomValue(-10, 10);
            SpawnProjectile(aimingAngle + offset);
        }
    }

    if (torsoSprite->hasLoopedOnce()) {
        hasFiredProjectile = false;
        torsoSprite->loopedOnce = false;
        frontArmSprite->ResetAnimation();
        backArmSprite->ResetAnimation();
        ChangeState(IDLE);
        attackTimer = 0.0f;
    }
}

void RatKing::SpawnProjectile(float angleOverride)
{
    float angle = (angleOverride != 0.0f) ? angleOverride : aimingAngle;
    Vector2 dir = { cosf(angle), sinf(angle) };
    Vector2 handLoc = { shoulder.x - 40, shoulder.y };
    Vector2 spawn = Vector2Add(handLoc, Vector2Scale(dir, 20.0f));

    auto* tp = new ToiletPaperProjectile(spawn, dir, 100.0f, 1.0f);
    // Ensure the projectile's collision state matches the boss's
    tp->SetCollisionEnabled(collisionEnabled);
    projectiles.push_back(tp);
}

void RatKing::HandleHurt(float dt)
{
    if (hurtBuffer > 0) return;

    hurtTimer -= dt;
    if (!hasFlashedHurt && hurtTimer <= 0.0f) {
        hasFlashedHurt = true;
        ChangeState(IDLE);
    }
}

void RatKing::HandleDeath(float dt)
{
    if (currentSprite->hasLoopedOnce())
        isActive = false;
}

void RatKing::TakeDamage(int dmg)
{
    if (currentState == HURT || currentState == DEATH) return;

    health -= dmg;
    if (health <= 0) {
        ChangeState(DEATH);
    }
    else {
        ChangeState(HURT);
        hurtBuffer = 24;
        hurtTimer = 0.8f;
        hasFlashedHurt = false;
        currentSprite->ResetAnimation();
    }
}

bool RatKing::ShouldBeRemoved()
{
    return currentState == DEATH && currentSprite->hasLoopedOnce();
}

void RatKing::SetPlayerPosition(Vector2 playerPos)
{
    playerPosition = playerPos;
}

bool RatKing::IsInLowHealthMode()
{
    return hasTriggeredLowHealthMusic;
}

double RatKing::GetLowHealthTriggerTime() const
{
    return lowHealthTriggeredAt;
}

std::vector<Rectangle> RatKing::GetHitboxes()
{
    // If collisions are disabled, return an empty vector
    if (!collisionEnabled) {
        return std::vector<Rectangle>();
    }
    return { { position.x + 32, position.y + 32, currentSprite->GetScaledWidth(), currentSprite->GetScaledHeight() } };
}

void RatKing::SetCollisionEnabled(bool enabled)
{
    collisionEnabled = enabled;
    // Propagate the collision state to all active projectiles
    for (auto& tp : projectiles) {
        tp->SetCollisionEnabled(enabled);
    }
}