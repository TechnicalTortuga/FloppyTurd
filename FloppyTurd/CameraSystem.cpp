#include "CameraSystem.h"

void CameraSystem::AddLayer(Layer* layer) {
    layers.push_back(layer);
}

void CameraSystem::Update(float deltaTime) {
    for (auto& layer : layers) {
        layer->Update(deltaTime);
    }
}

void CameraSystem::Draw() {
    for (auto& layer : layers) {
        layer->Draw();
    }
}