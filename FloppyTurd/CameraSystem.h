#pragma once
#include <vector>
#include "Layer.h"  // The abstract base class

class CameraSystem {
public:
    void AddLayer(Layer* layer);
    void Update(float deltaTime);
    void Draw();

private:
    std::vector<Layer*> layers;
};
