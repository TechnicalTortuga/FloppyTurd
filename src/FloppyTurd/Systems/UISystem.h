#pragma once

#include "../../Engine/Core/ECS.h"
#include "../../Engine/Platform/PlatformDelegates.h"
#include "../Components/GameComponents.h"
#include <vector>
#include <string>

namespace GameCore {

    /**
     * UISystem - Handles UI element rendering within the ECS architecture
     * Combines button and text rendering for UIElement components
     */
    class UISystem {
    private:
        Gnosis::ECS* m_ecsCoordinator;
        GameCore::PlatformDelegates m_delegates;
        
    public:
        UISystem(Gnosis::ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates);
        
        /**
         * Update UI system (called every frame)
         */
        void Update(float deltaTime);
        
        /**
         * Render all UI elements (called every frame after update)
         */
        void Render();
        
        /**
         * Render a single UI element (button + text)
         */
        void RenderUIElement(Gnosis::Entity entity, const Transform& transform, const UIElement& uiElement);
        
        /**
         * Check if an entity should be rendered
         */
        bool IsEntityVisible(Gnosis::Entity entity) const;
        
        /**
         * Create a button entity with all necessary components
         */
        Gnosis::Entity CreateButton(const std::string& text, 
                                   float x, float y, 
                                   float scale = 12.0f,
                                   const std::string& normalTexture = "button_normal",
                                   const std::string& hoverTexture = "button_hover", 
                                   const std::string& pressedTexture = "button_pressed",
                                   float fontSize = 32.0f);
        
        /**
         * Create a button with custom bounds (separate from sprite size)
         */
        Gnosis::Entity CreateButtonWithBounds(const std::string& text,
                                             float x, float y,
                                             float boundsWidth, float boundsHeight,
                                             float scale = 12.0f,
                                             const std::string& normalTexture = "button_normal",
                                             const std::string& hoverTexture = "button_hover",
                                             const std::string& pressedTexture = "button_pressed",
                                             float fontSize = 32.0f);
        
        /**
         * Check if a point is within an entity's bounds
         */
        bool IsPointInBounds(Gnosis::Entity entity, float x, float y) const;
        
        /**
         * Get the bounds of an entity (for debugging)
         */
        bool GetEntityBounds(Gnosis::Entity entity, float& left, float& right, float& top, float& bottom) const;
        
        /**
         * Update button sprite based on state (hover/pressed)
         */
        void UpdateButtonSprite(Gnosis::Entity entity);
        
        /**
         * Reset all button states to normal
         */
        void ResetAllButtonStates();
    };

} // namespace GameCore 