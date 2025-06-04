#include "Hat.h"
#include <string>

// Full constructor
Hat::Hat(const std::string& name,
    const std::string& iconPath,
    const std::string& turdletNormalPath, int turdletNormalFrames,
    const std::string& turdletShootingPath, int turdletShootingFrames,
    const std::string& teenNormalPath, int teenNormalFrames,
    const std::string& teenShootingPath, int teenShootingFrames,
    const std::string& bigNormalPath, int bigNormalFrames,
    const std::string& bigShootingPath, int bigShootingFrames,
    HatStatus status)
    : name(name), status(status)
{
    icon = LoadTexture(iconPath.c_str());
    sprites[0][0] = new Sprite(turdletNormalPath, turdletNormalFrames);
    sprites[0][1] = new Sprite(turdletShootingPath, turdletShootingFrames);
    sprites[1][0] = new Sprite(teenNormalPath, teenNormalFrames);
    sprites[1][1] = new Sprite(teenShootingPath, teenShootingFrames);
    sprites[2][0] = new Sprite(bigNormalPath, bigNormalFrames);
    sprites[2][1] = new Sprite(bigShootingPath, bigShootingFrames);
}

// Minimal constructor: uses a placeholder for all animated sprites.
Hat::Hat(const std::string& name,
    const std::string& iconPath,
    HatStatus status)
    : name(name), status(status)
{
    icon = LoadTexture(iconPath.c_str());
    std::string placeholderPath = "resources/hats/placeholder.png";
    // Assume placeholder has one frame.
    for (int form = 0; form < 3; form++) {
        sprites[form][0] = new Sprite(placeholderPath, 1);
        sprites[form][1] = new Sprite(placeholderPath, 1);
    }
}

// Destructor
Hat::~Hat() {
    UnloadTexture(icon);
    for (int form = 0; form < 3; form++) {
        for (int state = 0; state < 2; state++) {
            delete sprites[form][state];
        }
    }
}

// Get the sprite for the given form index and state.
Sprite* Hat::GetSprite(int formIndex, bool isShooting, bool isHurt) {
    int state;

    if (!isHurt)
        state = isShooting ? 1 : 0;
    else
        state = 0;

    // Validate form index; default to 0 if out of range.
    if (formIndex < 0 || formIndex > 2)
        formIndex = 0;
    return sprites[formIndex][state];
}

// Update the currently active sprite and set its position relative to the player.
void Hat::Update(float deltaTime, int formIndex, bool isShooting, bool isHurt, const Vector2& playerPos) {
    Sprite* currentSprite = GetSprite(formIndex, isShooting, isHurt);
    if (currentSprite) {
        // Set the sprite's position to match the player's.
        currentSprite->SetPosition(playerPos);
        currentSprite->Update(deltaTime);
    }
}

// Draw the currently active sprite at the player's position.
void Hat::Draw(int formIndex, bool isShooting, bool isHurt, const Vector2& playerPos) {
    Sprite* currentSprite = GetSprite(formIndex, isShooting, isHurt);
    if (currentSprite) {
        // Ensure the sprite is drawn at the current player position.
        currentSprite->SetPosition(playerPos);
        currentSprite->Draw();
    }
}
