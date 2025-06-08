#include "SewerPipe.h"
#include <random>

SewerPipe::SewerPipe(int xPos, int yPos) {
    using namespace Resources;

    // Randomly pick which type of pipe to use:
    static std::default_random_engine engine{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(0, 3);
    int variant = dist(engine);

    switch (variant) {
    case 0:
        pipeType = PipeType::TopOrange;
        chosenPipe = LoadTexture(TopPipeOrange);
        break;
    case 1:
        pipeType = PipeType::TopBlue;
        chosenPipe = LoadTexture(TopPipeBlue);
        break;
    case 2:
        pipeType = PipeType::BottomOrange;
        chosenPipe = LoadTexture(BottomPipeOrange);
        break;
    case 3:
        pipeType = PipeType::BottomBlue;
        chosenPipe = LoadTexture(BottomPipeBlue);
        break;
    }

    // Set the x position and y position based on type
    pos.x = static_cast<float>(xPos);
    if (pipeType == PipeType::TopOrange || pipeType == PipeType::TopBlue) {
        pos.y = 0.0f;
    }
    else {
        pos.y = 180 - chosenPipe.height;
    }

    panSpeed = 320 * 0.002f; // Matches 80.0f used elsewhere
    UpdateHitbox();
}

SewerPipe::~SewerPipe() {
    UnloadTexture(chosenPipe);
}

void SewerPipe::SetPipeType(PipeType newType) {
    using namespace Resources;

    // Unload the current texture
    UnloadTexture(chosenPipe);

    // Set the new type and load the corresponding texture
    pipeType = newType;
    switch (pipeType) {
    case PipeType::TopOrange:
        chosenPipe = LoadTexture(TopPipeOrange);
        break;
    case PipeType::TopBlue:
        chosenPipe = LoadTexture(TopPipeBlue);
        break;
    case PipeType::BottomOrange:
        chosenPipe = LoadTexture(BottomPipeOrange);
        break;
    case PipeType::BottomBlue:
        chosenPipe = LoadTexture(BottomPipeBlue);
        break;
    }

    // Update y-position based on type
    if (pipeType == PipeType::TopOrange || pipeType == PipeType::TopBlue) {
        pos.y = 0.0f;
    }
    else {
        pos.y = 180 - chosenPipe.height;
    }

    // Update hitbox to reflect new position
    UpdateHitbox();
}

void SewerPipe::UpdateHitbox() {
    hitbox.x = pos.x + 5;
    hitbox.y = pos.y + 5;
    hitbox.width = static_cast<float>(chosenPipe.width) - 10;
    hitbox.height = static_cast<float>(chosenPipe.height) - 10;
}

void SewerPipe::Draw() {
    DrawTexturePro(
        chosenPipe,
        { 0, 0, static_cast<float>(chosenPipe.width), static_cast<float>(chosenPipe.height) },
        { pos.x, pos.y, static_cast<float>(chosenPipe.width), static_cast<float>(chosenPipe.height) },
        { 0, 0 },
        0.0f,
        WHITE
    );
}

void SewerPipe::Update(float deltaTime) {
    UpdateHitbox();
}

void SewerPipe::resetScore() {
    hasScored = false;
}

std::vector<Rectangle> SewerPipe::GetHitboxes() {
    // If collisions are disabled, return an empty vector
    if (!collisionEnabled) {
        return std::vector<Rectangle>();
    }

    std::vector<Rectangle> boxes;
    boxes.push_back(hitbox);
    return boxes;
}

void SewerPipe::SetCollisionEnabled(bool enabled)
{
    collisionEnabled = enabled;
}

void SewerPipe::yOffsetRandomizer() {
    static std::default_random_engine engine{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(-20.0f, 60.0f);
    yOffset = dist(engine);
}

float SewerPipe::GetEdge() const {
    if (pipeType == PipeType::TopOrange || pipeType == PipeType::TopBlue) {
        return pos.y + chosenPipe.height;
    }
    else {
        return pos.y;
    }
}