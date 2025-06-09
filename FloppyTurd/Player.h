#pragma once
#include "Sprite.h"
#include "Resources.h"
#include "GameSettings.h"
#include "Projectile.h"
#include <iostream>
#include <vector>
#include "Hat.h"
#include "Game.h"  

// Forward declaration of Game
class Game;

enum HeartMode { WHOLE = 1, HALVES = 2, THIRDS = 3 };

class Player {
public:
    Player(Game* game = nullptr);
    ~Player();

    enum HeartMode { WHOLE = 1, HALVES = 2, THIRDS = 3 };

    void    SetHeartMode(HeartMode mode);   // called by the skill tree
    HeartMode   GetHeartMode()        const { return heartMode; }
    int GetTotalHearts()      const { return hearts; }
    int GetSlicesPerHeart()   const { return (int)heartMode; }
    int GetSlicesLeft()       const { return liveSlices; }      // living slices
    int GetGhostSlicesLeft()  const { return ghostSlices; }     // “borrowed” by hollow-turds
    void EnableHollowTurds(bool enabled) { hollowTurds = enabled; }
    void SetInitialHearts(int difficultyIndex); // New method for difficulty-based hearts

    void Draw();
    void Update(float deltaTime);
    void PutTheHurtOn(int DAMAGE);
    bool SpendCoinsForShoot(); // New method to handle coin-based shooting
    float GetCircleRadius() const;
    Vector2 GetCircleCenter() const;
    int GetTurdPoints() const;
    void SetHat(Hat* hat);
    void InitSprites();
    void Jump();
    void Shoot(); // Method to shoot projectiles
    void Revive();
    void SetHealth(int hp);
    void ResetPosition();

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

    bool    isShooting;
    bool   shootingUnlocked{ false };
    void   EnableShooting(bool e) { shootingUnlocked = e; }
    bool   CanShoot()       const { return shootingUnlocked; }

    void AddHeartSlice(int amount);
    void SetMaxHearts(int max);
    void ActivateInvisibility(float duration);
    bool isAlive{ true };
    bool isInvisible = false;

    // Coin management
    void AddCoins(int amount); // Add coins to session and notify Playing
    int GetSessionCoins() const { return sessionCoins; } // Get current session coins
    void ResetSessionCoins(); // Reset session coins

    // Set the game reference
    void SetGame(Game* g) { game = g; }

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
    Vector2 pos{ 77.0f, 50.0f };
    Vector2 size{ 64.0f, 64.0f };
    Vector2 velocity{ 0.0f, 0.0f };

    Sprite* currentSprite;

    // Form 0: Turdlet sprites
    Sprite* idleSpriteTurdlet;
    Sprite* jumpSpriteTurdlet;
    Sprite* shootSpriteTurdlet;

    // Form 1: Teenage Turd sprites
    Sprite* idleSpriteTeen;
    Sprite* jumpSpriteTeen;
    Sprite* shootSpriteTeen;

    // Form 2: Big Turd sprites
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

    /* health bookkeeping */
    int        hearts{ 2 };         // total heart icons shown
    HeartMode  heartMode{ WHOLE };     // 1 = full hearts, 2 = halves, 3 = thirds
    int        liveSlices{ hearts * (int)heartMode };   // keep in sync
    int        ghostSlices{ 0 };         // slices temporarily “spent” by hollow-turds
    bool       hollowTurds{ false };     // set by Skill 4

    float invisibilityTimer = 0.0f;

    // Session coins
    int sessionCoins{ 0 }; // Coins collected in the current session

    Game* game; // Reference to Game for accessing Playing
};