#include "Outhouse.h"
#include "ToiletPair.h"

Outhouse::Outhouse(int xPos, int yPos)
{
    using namespace GameSettings;
    using namespace Resources;

    _Outhouse = LoadTexture(OuthouseSolo);
    _OuthouseToilet = LoadTexture(OuthouseToilet);

    pos.x = xPos;
    pos.y = yPos;

    objectScale = 1.0f;
    panSpeed = 320 * 0.002f;
}

Outhouse::~Outhouse()
{
    UnloadTexture(_Outhouse);
    UnloadTexture(_OuthouseToilet);
}

void Outhouse::Draw()
{
    using namespace GameSettings;

    // Draw the Outhouse Toilet
    DrawTexturePro(
        _OuthouseToilet,
        { 0, 0, (float)(_OuthouseToilet.width), (float)(_OuthouseToilet.height) },
        { pos.x, (float)180 - _OuthouseToilet.height, (float)_OuthouseToilet.width, (float)_OuthouseToilet.height },
        { 0, 0 }, // Origin of image
        0.0f, // Rotation
        WHITE
    );

    // Draw the Outhouse
    DrawTexturePro(
        _Outhouse,
        { 0, 0, (float)(_Outhouse.width), (float)(_Outhouse.height) },
        { pos.x, (float)180 - _Outhouse.height, (float)_Outhouse.width, (float)_Outhouse.height },
        { 0, 0 }, // Origin of image
        0.0f, // Rotation
        WHITE
    );

    // DEBUG DRAWING HITBOXES
    //DrawRectangleLines(_outhouseHitbox.x, _outhouseHitbox.y, _outhouseHitbox.width, _outhouseHitbox.height, BLUE);
    //DrawRectangleLines(_outhouseToiletHitbox.x, _outhouseToiletHitbox.y, _outhouseToiletHitbox.width, _outhouseToiletHitbox.height, BLUE);
}

void Outhouse::ResetScore()
{
    hasScored = false;
}

void Outhouse::Update(float deltaTime)
{
    UpdateHitbox();
}

Rectangle Outhouse::GetOuthouseHitbox()
{
    return _outhouseHitbox;
}

Rectangle Outhouse::GetOuthouseToiletHitbox()
{
    return _outhouseToiletHitbox;
}

std::vector<Rectangle> Outhouse::GetHitboxes()
{
    // If collisions are disabled, return an empty vector
    if (!collisionEnabled) {
        return std::vector<Rectangle>();
    }

    std::vector<Rectangle>* hitboxes = new std::vector<Rectangle>();

    hitboxes->push_back(_outhouseHitbox);
    hitboxes->push_back(_outhouseToiletHitbox);

    return *hitboxes;
}

void Outhouse::SetCollisionEnabled(bool enabled)
{
    collisionEnabled = enabled;
}

void Outhouse::UpdateHitbox()
{
    using namespace GameSettings;

    // Toilet hitbox
    _outhouseToiletHitbox.x = pos.x + (_OuthouseToilet.width / 4) + 4;
    _outhouseToiletHitbox.y = 180 - (3 * _OuthouseToilet.height / 4) - 12;
    _outhouseToiletHitbox.width = (_OuthouseToilet.width / 2) - 8;
    _outhouseToiletHitbox.height = _OuthouseToilet.height;

    // Outhouse hitbox
    _outhouseHitbox.x = pos.x + 8;
    _outhouseHitbox.y = 180 - (_Outhouse.height / 2);
    _outhouseHitbox.width = _Outhouse.width - 16;
    _outhouseHitbox.height = _Outhouse.height;
}