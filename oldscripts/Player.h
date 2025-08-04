#pragma once
#include "Sprite.h"
#include "Resources.h"
#include "GameSettings.h"
#include "Projectile.h"
#include <iostream>
#include <vector>
#include "Hat.h"
class Player {
public:
    Player();
    ~Player();

    void Draw();
    void Update(float deltaTime);
    void PutTheHurtOn(int DAMAGE);
    float GetCircleRadius() const;
    Vector2 GetCircleCenter() const;
    int GetTurdPoints() const;
    void SetHat(Hat* hat);
    void InitSprites();
    void Jump();
    void Shoot(); // Method to shoot projectiles
    void Reset();
    int GetHealth();

    // New method to get player position
    Vector2 GetPosition() const { return pos; }

    // Method to change the player form (0: Turdlet, 1: Teenage Turd, 2: Big Turd)
    void ChangeForm(int newForm);

    // Accessor for projectiles
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

private:
    PLAYERSTATE playerstate{ JUMPING };
    int health{ 99 };
    int formLevel{ 1 };
    int turdPoints{ 0 };

    Vector2 circleCenter{ 77.0f, 100.0f };
    float circleRadius{ 16 };
    float playerScale{ 1.0f };
    float MAXVELOCITY{ 4.0f };
    float GRAVITY{ 0.4f };
    float JUMPVELOCITY{ 20.0f };
    Vector2 pos{ 77.0f, 100.0f };
    Vector2 size{ 64.0f, 64.0f };
    Vector2 velocity{ 0.0f, 0.0f };
    bool isAlive{ true };

    Sprite* currentSprite;

    // Form 0: Turdlet sprites
    Sprite* idleSpriteTurdlet;
    Sprite* jumpSpriteTurdlet;
    Sprite* shootSpriteTurdlet;

    // Form 1: Teenage Turd sprites (assumes you add these assets in Resources.h)
    Sprite* idleSpriteTeen;
    Sprite* jumpSpriteTeen;
    Sprite* shootSpriteTeen;

    // Form 2: Big Turd sprites (assumes you add these assets in Resources.h)
    Sprite* idleSpriteBig;
    Sprite* jumpSpriteBig;
    Sprite* shootSpriteBig;

    Sprite* hurtSpriteTurdlet;
    Sprite* hurtSpriteTeen;
    Sprite* hurtSpriteBig;

    float hurtBuffer{ 1.0f };

    // Projectiles
    std::vector<Projectile*> projectiles;
    float shootCooldown{ 0.5f }; // Time between shots
    float shootTimer{ 0.0f };

    // Hat
    Hat* currentSelectedHat;

    Sound hurtSound;
};