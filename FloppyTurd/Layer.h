#pragma once

class Layer {
public:
    virtual ~Layer() = default;

    // These are the methods all layers must implement.
    virtual void Update(float deltaTime) = 0;
    virtual void Draw() = 0;
};