#include "Level.h"

void Level::AddPickUp(std::shared_ptr<PickUp> pickup) {
    pickups.push_back(std::move(pickup));
}

std::vector<std::shared_ptr<PickUp>>& Level::GetPickUps() {
    return pickups;
}