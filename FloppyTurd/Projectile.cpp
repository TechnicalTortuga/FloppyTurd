#include "Projectile.h"
#include "GameSettings.h"
#include "ProjectileSkills.h"
#include "LevelManager.h"
#include <algorithm>
#include <limits>

Projectile::Projectile(Vector2 position, Vector2 direction, float speed, float scale, const char* spriteFilePath, int rank)
    : position(position), direction(direction), speed(speed), scale(scale), homingRank(std::clamp(rank,0,2))
{
    // Initialize the sprite with scaling
    sprite = new Sprite(spriteFilePath, 5, 0.1f, scale, position, AtlasCategory::PARTICLES);

    // Initialize the hitbox based on the scaled sprite dimensions
    hitbox.width = sprite->GetScaledWidth();
    hitbox.height = sprite->GetScaledHeight();
    hitbox.x = position.x;
    hitbox.y = position.y;
}

Projectile::~Projectile() {
    delete sprite;
}

void Projectile::Update(float deltaTime) {
    if (homingRank > 0) {
        Vector2 target;
        if (!GetTargetPoint(target)) AcquireTarget();
        if (GetTargetPoint(target)) {
            const Vector2 center={position.x+hitbox.width/2,position.y+hitbox.height/2};
            direction=ProjectileSkills::Steer(direction,center,target,homingRank,deltaTime);
        }
    }
    position.x += direction.x * speed * deltaTime;
    position.y += direction.y * speed * deltaTime;

    // Update sprite position and animation
    sprite->SetPosition(position.x, position.y);
    sprite->Update(deltaTime);

    // Update hitbox position
    hitbox.x = position.x;
    hitbox.y = position.y;
}

namespace {
bool TargetOnScreen(Rectangle box) {
    return box.width>0 && box.height>0 && box.x+box.width>0 && box.y+box.height>0
        && box.x<GameSettings::GameWidth && box.y<GameSettings::GameHeight;
}
Vector2 Center(Rectangle box) { return {box.x+box.width/2,box.y+box.height/2}; }
}

bool Projectile::GetTargetPoint(Vector2& point) const {
    if (const auto enemy=targetEnemy.lock()) {
        const auto box=enemy->GetHitbox();
        if (enemy->IsCombatTarget() && !enemy->ShouldBeRemoved() && TargetOnScreen(box)) { point=Center(box); return true; }
    }
    if (const auto boss=targetBoss.lock(); boss && boss->isActive && boss->health>0 && boss->GetCurrentState()!=Boss::DEATH) {
        for (auto box:boss->GetHitboxes()) if (TargetOnScreen(box)) { point=Center(box); return true; }
    }
    return false;
}

void Projectile::AcquireTarget(const std::vector<const void*>& excluded) {
    targetEnemy.reset(); targetBoss.reset();
    auto* manager=LevelManager::GetInstance();
    if (homingRank<=0 || !manager) return;
    const Vector2 center={position.x+hitbox.width/2,position.y+hitbox.height/2};
    float nearest=std::numeric_limits<float>::max();
    auto candidate=[&](Rectangle box,const void* id) {
        if (!TargetOnScreen(box) || std::find(excluded.begin(),excluded.end(),id)!=excluded.end()) return false;
        const auto point=Center(box);
        if (!ProjectileSkills::CanAcquire(center,direction,point,homingRank)) return false;
        const float distance=std::hypot(point.x-center.x,point.y-center.y);
        if (distance>=nearest) return false;
        nearest=distance; return true;
    };
    for (const auto& enemy:manager->GetEnemies()) {
        if (enemy->IsCombatTarget() && !enemy->ShouldBeRemoved() && candidate(enemy->GetHitbox(),enemy.get())) targetEnemy=enemy;
    }
    const auto boss=manager->GetBoss();
    if (boss && boss->isActive && boss->health>0 && boss->GetCurrentState()!=Boss::DEATH) {
        for (auto box:boss->GetHitboxes()) if (candidate(box,boss.get())) { targetBoss=boss; targetEnemy.reset(); }
    }
}

const void* Projectile::TargetId() const {
    if (const auto enemy=targetEnemy.lock()) return enemy.get();
    if (const auto boss=targetBoss.lock()) return boss.get();
    return nullptr;
}

void Projectile::Draw() {
    sprite->Draw(position.x, position.y);
    // Optional: Draw hitbox for debugging
    // DrawRectangleLines(hitbox.x, hitbox.y, hitbox.width, hitbox.height, RED);
}

Rectangle Projectile::GetHitbox() const {
    return hitbox;
}

bool Projectile::IsOffScreen() const {
    return position.x > GameSettings::GameWidth || position.x < 0 ||
        position.y > GameSettings::GameHeight || position.y < 0;
}
