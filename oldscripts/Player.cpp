#include "Player.h"
#include "SoundManager.h"

Player::Player() {
    InitSprites();
}

Player::~Player() {
    // Clean up projectiles
    for (auto& projectile : projectiles) {
        delete projectile;
    }
    // Delete form 0 sprites
    delete idleSpriteTurdlet;
    delete jumpSpriteTurdlet;
    delete shootSpriteTurdlet;
    // Delete form 1 sprites
    delete idleSpriteTeen;
    delete jumpSpriteTeen;
    delete shootSpriteTeen;
    // Delete form 2 sprites
    delete idleSpriteBig;
    delete jumpSpriteBig;
    delete shootSpriteBig;
    // Delete hurt sprites for all forms
    delete hurtSpriteTurdlet;
    delete hurtSpriteTeen;
    delete hurtSpriteBig;

    // Unload hurt sound
    SoundManager::GetInstance().UnloadSoundClip(hurtSound);
}

void Player::Draw() {
    currentSprite->Draw(pos.x, pos.y);

    // Draw projectiles
    for (auto& projectile : projectiles) {
        projectile->Draw();
    }

    currentSelectedHat->Draw(formLevel, isShooting, hurtBuffer > 0.0f, pos);
}

void Player::Update(float deltaTime) {
    // Update current sprite based on player state (only one update call per frame)
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
        // Optionally handle dead state
    case HURT:
        currentSprite->Update(deltaTime);
        break;
    }

    // Gravity and position updates
    velocity.y += GRAVITY;
    if (velocity.y > MAXVELOCITY)  velocity.y = MAXVELOCITY;
    if (velocity.y < -MAXVELOCITY) velocity.y = -MAXVELOCITY;
    pos.y += velocity.y;

    // Update collision circle
    circleRadius = 11;
    circleCenter = { pos.x + 32, pos.y + 34 };

    // Boundary checks
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

    // Update sprite position to match player position
    currentSprite->SetPosition(pos.x, pos.y);

    // Update projectiles
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

    // Shooting cooldown
    if (shootTimer > 0.0f) {
        shootTimer -= deltaTime;
    }

    // Update hurt buffer timer if active.
    if (hurtBuffer > 0.0f) {
        hurtBuffer -= deltaTime;
        if (hurtBuffer <= 0.0f) {
            // Hurt period finished: switch back to normal (jumping) state.
            playerstate = JUMPING;
            ChangeForm(formLevel);
        }
    }

    // Update the hat and sync its animation frame with the player’s sprite
    currentSelectedHat->Update(deltaTime, formLevel, isShooting, hurtBuffer > 0.0f, pos);
    int currentFrame = currentSprite->GetFrameIndex();
    currentSelectedHat->GetSprite(formLevel, isShooting, hurtBuffer > 0.0f)->SetFrameIndex(currentFrame);
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

void Player::PutTheHurtOn(int DAMAGE) {
    // Only process damage if not already hurt.
    if (hurtBuffer > 0.0f)
        return;

    health -= DAMAGE;
    // Set hurt buffer duration to 1.0 second.
    hurtBuffer = 1.0f;
    playerstate = HURT;
    ChangeForm(formLevel);

    SoundManager::GetInstance().PlaySoundClip(hurtSound);
}

void Player::InitSprites() {
    using namespace Resources;
    float playerScale = 1.0f;
    int idleFrames = 1;   // Replace with actual frame counts
    int jumpFrames = 6;
    int shootFrames = 8;

    // Form 0: Turdlet
    idleSpriteTurdlet = new Sprite(TurdletIdle, idleFrames, 0.1f, playerScale, pos);
    jumpSpriteTurdlet = new Sprite(TurdletJump, jumpFrames, 0.1f, playerScale, pos);
    shootSpriteTurdlet = new Sprite(TurdletShoot, 5, 0.1f, playerScale, pos);

    // Form 1: Teenage Turd (ensure you add these assets in Resources.h)
    idleSpriteTeen = new Sprite(TeenageTurdIdle, idleFrames, 0.1f, playerScale, pos);
    jumpSpriteTeen = new Sprite(TeenageTurdJump, jumpFrames, 0.1f, playerScale, pos);
    shootSpriteTeen = new Sprite(TeenageTurdShoot, shootFrames, 0.1f, playerScale, pos);

    // Form 2: Big Turd (ensure you add these assets in Resources.h)
    idleSpriteBig = new Sprite(BigTurdIdle, idleFrames, 0.1f, playerScale, pos);
    jumpSpriteBig = new Sprite(BigTurdJump, jumpFrames, 0.1f, playerScale, pos);
    shootSpriteBig = new Sprite(BigTurdShoot, shootFrames, 0.1f, playerScale, pos);

    hurtSpriteTurdlet = new Sprite(TurdletHurt, jumpFrames, 0.1f, playerScale, pos);
    hurtSpriteTeen = new Sprite(TeenageTurdHurt, jumpFrames, 0.1f, playerScale, pos);
    hurtSpriteBig = new Sprite(BigTurdHurt, jumpFrames, 0.1f, playerScale, pos);

    // Default form is 0 (Turdlet)
    formLevel = 0;
    ChangeForm(formLevel);

    hurtSound = LoadSound(Hurt);
}

void Player::ChangeForm(int newForm) {
    if (newForm < 0 || newForm > 2)
        return;  // Ensure valid form levels

    formLevel = newForm;

    // Switch current sprite based on player's state and form
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

    if(currentSelectedHat != nullptr)
        currentSelectedHat->GetSprite(formLevel, isShooting, hurtBuffer > 0.0f)->ResetAnimation();
}

void Player::Jump() {
    if (isAlive)
        velocity.y -= JUMPVELOCITY;
}

void Player::Shoot() {
    using namespace Resources;

    if (isAlive && shootTimer <= 0.0f && hurtBuffer < 0.0f) {
        isShooting = true;
        playerstate = SHOOTING;
        ChangeForm(formLevel);

        float projectileScale = 1.0f;
        float projectileSpeed = 300.0f;
        const char* projectileSpritePath = nullptr;

        // Choose projectile sprite based on player form
        switch (formLevel) {
        case 0:
            projectileSpritePath = PoopSmall;
            break;
        case 1:
            projectileSpritePath = PoopMid;
            break;
        case 2:
            projectileSpritePath = PoopLarge;
            break;
        default:
            projectileSpritePath = PoopSmall;
            break;
        }

        float projectileWidth = 16.0f;
        float projectileHeight = 16.0f;
        Vector2 projectilePosition = {
            pos.x + size.x / 2.0f,
            pos.y + size.y / 2.0f - projectileHeight / 2.0f
        };
        Vector2 direction = { 1.0f, 0.0f };

        Projectile* newProjectile = new Projectile(projectilePosition, direction, projectileSpeed, projectileScale, projectileSpritePath);
        projectiles.push_back(newProjectile);
        shootTimer = shootCooldown;
    }
}

void Player::Reset() {
    playerstate = JUMPING;
    ChangeForm(formLevel);
    health = 100;
    isAlive = true;
    isShooting = false;
    currentSprite->ResetAnimation();
}

int Player::GetHealth() {
    return health;
}

const std::vector<Projectile*>& Player::GetProjectiles() const {
    return projectiles;
}

std::vector<Projectile*>& Player::GetProjectilesNonConst()
{
    return projectiles; // 'projectiles' is the private member in Player
}
