#include "Hat.h"
#include <string>
#include "ResourceManager.h"

// Full constructor
Hat::Hat(const std::string& name,
    const std::string& iconPath,
    const std::string& turdletNormalPath, int turdletNormalFrames,
    const std::string& turdletShootingPath, int turdletShootingFrames,
    const std::string& bigNormalPath, int bigNormalFrames,
    const std::string& bigShootingPath, int bigShootingFrames,
    HatStatus status)
    : name(name), status(status)
{
    // Use ResourceManager to load icon texture based on the path
    std::string iconId = "";
    if (iconPath.find("cowboyhat.png") != std::string::npos) iconId = "cowboy_hat";
    else if (iconPath.find("flowerhat.png") != std::string::npos) iconId = "flower_hat";
    else if (iconPath.find("dooraghat.png") != std::string::npos) iconId = "doorag_hat";
    else if (iconPath.find("ballcap.png") != std::string::npos) iconId = "ballcap_hat";
    else if (iconPath.find("PinwheelHat.png") != std::string::npos) iconId = "pinwheel_hat";
    else if (iconPath.find("strawhat.png") != std::string::npos) iconId = "straw_hat";
    else if (iconPath.find("SamuraiHelmet.png") != std::string::npos) iconId = "samurai_hat";
    else if (iconPath.find("tophat.png") != std::string::npos) iconId = "top_hat";
    else if (iconPath.find("ushanka.png") != std::string::npos) iconId = "ushanka_hat";
    else if (iconPath.find("Beret.png") != std::string::npos) iconId = "beret_hat";
    else if (iconPath.find("Crown.png") != std::string::npos) iconId = "crown_hat";
    else if (iconPath.find("poophat.png") != std::string::npos) iconId = "poop_hat";
    else if (iconPath.find("RamsesHat.png") != std::string::npos) iconId = "ramses_hat";
    else if (iconPath.find("SpartanHelmet.png") != std::string::npos) iconId = "spartan_hat";
    else if (iconPath.find("shellhat.png") != std::string::npos) iconId = "shell_hat";
    
    if (!iconId.empty()) {
        icon = ResourceManager::GetInstance().GetTexture(iconId);
    } else {
        // Fallback to direct loading if not found in ResourceManager
        icon = LoadTexture(iconPath.c_str());
    }
    
    sprites[0][0] = new Sprite(turdletNormalPath, turdletNormalFrames, 0.1f, 1.0f, {0,0}, AtlasCategory::PLAYER_SPRITES);
    sprites[0][1] = new Sprite(turdletShootingPath, turdletShootingFrames, 0.1f, 1.0f, {0,0}, AtlasCategory::PLAYER_SPRITES);
    // Use Turdlet sprites as placeholders for TeenageTurd
    sprites[1][0] = new Sprite(turdletNormalPath, turdletNormalFrames, 0.1f, 1.0f, {0,0}, AtlasCategory::PLAYER_SPRITES);
    sprites[1][1] = new Sprite(turdletShootingPath, turdletShootingFrames, 0.1f, 1.0f, {0,0}, AtlasCategory::PLAYER_SPRITES);
    sprites[2][0] = new Sprite(bigNormalPath, bigNormalFrames, 0.1f, 1.0f, {0,0}, AtlasCategory::PLAYER_SPRITES);
    sprites[2][1] = new Sprite(bigShootingPath, bigShootingFrames, 0.1f, 1.0f, {0,0}, AtlasCategory::PLAYER_SPRITES);
}

// Minimal constructor: uses a placeholder for all animated sprites
Hat::Hat(const std::string& name,
    const std::string& iconPath,
    HatStatus status)
    : name(name), status(status)
{
    // Use ResourceManager to load icon texture based on the path
    std::string iconId = "";
    if (iconPath.find("cowboyhat.png") != std::string::npos) iconId = "cowboy_hat";
    else if (iconPath.find("flowerhat.png") != std::string::npos) iconId = "flower_hat";
    else if (iconPath.find("dooraghat.png") != std::string::npos) iconId = "doorag_hat";
    else if (iconPath.find("ballcap.png") != std::string::npos) iconId = "ballcap_hat";
    else if (iconPath.find("PinwheelHat.png") != std::string::npos) iconId = "pinwheel_hat";
    else if (iconPath.find("strawhat.png") != std::string::npos) iconId = "straw_hat";
    else if (iconPath.find("SamuraiHelmet.png") != std::string::npos) iconId = "samurai_hat";
    else if (iconPath.find("tophat.png") != std::string::npos) iconId = "top_hat";
    else if (iconPath.find("ushanka.png") != std::string::npos) iconId = "ushanka_hat";
    else if (iconPath.find("Beret.png") != std::string::npos) iconId = "beret_hat";
    else if (iconPath.find("Crown.png") != std::string::npos) iconId = "crown_hat";
    else if (iconPath.find("poophat.png") != std::string::npos) iconId = "poop_hat";
    else if (iconPath.find("RamsesHat.png") != std::string::npos) iconId = "ramses_hat";
    else if (iconPath.find("SpartanHelmet.png") != std::string::npos) iconId = "spartan_hat";
    else if (iconPath.find("shellhat.png") != std::string::npos) iconId = "shell_hat";
    
    if (!iconId.empty()) {
        icon = ResourceManager::GetInstance().GetTexture(iconId);
    } else {
        // Fallback to direct loading if not found in ResourceManager
        icon = LoadTexture(iconPath.c_str());
    }
    
    std::string placeholderPath = "resources/hats/placeholder.png";
    // Assume placeholder has one frame
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

// Get the sprite for the given form index and state
Sprite* Hat::GetSprite(int formLevel, bool isShooting, bool isHurt) {
    // Determine state: shooting only if not hurt
    int state = isShooting && !isHurt ? 1 : 0;
    
    // Validate form index; default to 0 if out of range
    if (formLevel < 0 || formLevel > 2) {
        formLevel = 0;
    }
    
    // Check if the pointer looks valid before dereferencing
    Sprite* result = sprites[formLevel][state];
    
    if (result == nullptr) {
        return nullptr;
    }
    
    return result;
}

// Update the currently active sprite and set its position relative to the player
void Hat::Update(float deltaTime, int formIndex, bool isShooting, bool isHurt, const Vector2& playerPos) {
    Sprite* hatSprite = GetSprite(formIndex, isShooting, isHurt);
    
    if (hatSprite) {
        hatSprite->Update(deltaTime);
        hatSprite->SetPosition(playerPos.x, playerPos.y);
    }
}

// Draw the currently active sprite at the player's position
void Hat::Draw(int formIndex, bool isShooting, bool isHurt, const Vector2& playerPos) {
    Sprite* currentSprite = GetSprite(formIndex, isShooting, isHurt);
    if (currentSprite) {
        currentSprite->SetPosition(playerPos);
        currentSprite->Draw();
    }
}