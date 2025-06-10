#pragma once
#include "raylib.h"
#include "Sprite.h"
#include <string>

enum HatStatus {
    LOCKED,
    UNLOCKED
};

class Hat {
public:
    std::string name;
    Texture2D icon;

    // sprites[form][state]: form index 0 = turdlet, 1 = teen, 2 = big
    // state 0 = normal/jump, 1 = shooting
    Sprite* sprites[3][2];

    HatStatus status;

    // Full constructor: pass in icon path and sprite file paths with frame counts
    Hat(const std::string& name,
        const std::string& iconPath,
        const std::string& turdletNormalPath, int turdletNormalFrames,
        const std::string& turdletShootingPath, int turdletShootingFrames,
        const std::string& bigNormalPath, int bigNormalFrames,
        const std::string& bigShootingPath, int bigShootingFrames,
        HatStatus status = LOCKED);

    // Minimal constructor: only an icon is provided; load placeholder sprites
    Hat(const std::string& name,
        const std::string& iconPath,
        HatStatus status = LOCKED);

    ~Hat();

    // Returns the appropriate sprite given a form index and shooting flag
    Sprite* GetSprite(int formIndex, bool isShooting, bool isHurt);

    // Update the current sprite's animation and set its position to overlay the player
    void Update(float deltaTime, int formIndex, bool isShooting, bool isHurt, const Vector2& playerPos);

    // Draw the current sprite at the player's position
    void Draw(int formIndex, bool isShooting, bool isHurt, const Vector2& playerPos);
};