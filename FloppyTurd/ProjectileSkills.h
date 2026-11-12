#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <raylib.h>
#include <cstdint>

// Ported from iOS SkillSystem/ProjectileSystem at ea665c2. Native-pixel
// distances use the iOS sprite scale of eight; angular rates stay unchanged.
namespace ProjectileSkills {
constexpr int MaxRank = 2;
enum class Kind { Homing, Split };
inline int NextCost(Kind skill, int rank) {
    if (rank < 0 || rank >= MaxRank) return 0;
    return skill == Kind::Homing ? (rank == 0 ? 300 : 700) : (rank == 0 ? 200 : 400);
}
template<class Spend>
bool Upgrade(Kind skill,std::uint8_t& rank,bool shootingUnlocked,Spend&& spend) {
    const int cost=NextCost(skill,rank);
    if (!shootingUnlocked || cost<=0 || !spend(cost)) return false;
    ++rank;
    return true;
}
inline int ShotCount(int rank) { return 1 + std::clamp(rank, 0, MaxRank); }
inline float ShotOffset(int index) { return index == 1 ? 15.0f : index == 2 ? -15.0f : 0.0f; }
inline float ConeDegrees(int rank) { return rank <= 0 ? 0 : rank == 1 ? 20.0f : 40.0f; }
inline float WrapAngle(float radians) {
    constexpr float pi=3.14159265358979323846f;
    return std::remainder(radians, 2*pi);
}
inline bool CanAcquire(Vector2 origin, Vector2 direction, Vector2 target, int rank) {
    const float dx=target.x-origin.x, dy=target.y-origin.y;
    if (rank <= 0 || dx <= 0) return false;
    const float angle=WrapAngle(std::atan2(dy,dx)-std::atan2(direction.y,direction.x));
    return std::abs(angle) <= ConeDegrees(rank)*DEG2RAD;
}
inline Vector2 Steer(Vector2 direction, Vector2 origin, Vector2 target, int rank, float dt) {
    if (rank <= 0 || dt <= 0) return direction;
    const float dx=target.x-origin.x, dy=target.y-origin.y;
    const float distance=std::hypot(dx,dy);
    if (distance <= 2.5f) return direction;
    const float current=std::atan2(direction.y,direction.x);
    const float difference=WrapAngle(std::atan2(dy,dx)-current);
    const float attraction=1+(1-std::min(distance/100.0f,1.0f))*2;
    const float rate=(rank == 1 ? 6.0f : 10.0f)*0.1f*attraction;
    const float angle=current+std::clamp(difference,-rate*dt,rate*dt);
    return {std::cos(angle),std::sin(angle)};
}
}
