#include "UISystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    UISystem::UISystem(Gnosis::ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates)
        : m_ecsCoordinator(ecsCoordinator)
        , m_delegates(delegates)
    {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("UISystem: ECS coordinator is null");
        }
        
        GN_LOG_INFO("UISystem: Initialized with integrated UI rendering");
    }

    void UISystem::Update(float deltaTime) {
        if (!m_ecsCoordinator) {
            return;
        }

        // UI system doesn't need update logic for now
        // Could add button animations, hover effects, etc. here in the future
    }

    void UISystem::Render() {
        if (!m_ecsCoordinator) {
            return;
        }

        // Get all entities with both Transform and UIElement components
        auto entities = m_ecsCoordinator->GetEntitiesWithComponents<Transform, UIElement>();
        
        GN_LOG_DEBUG("UISystem: Found " + std::to_string(entities.size()) + " entities with Transform and UIElement components");
        
        // Collect visible UI elements with their layer info
        std::vector<std::pair<Gnosis::Entity, int>> visibleUIElements;
        
        for (Gnosis::Entity entity : entities) {
            if (!IsEntityVisible(entity)) {
                continue;
            }
            
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
            if (uiElement && uiElement->isEnabled && uiElement->visible) {
                visibleUIElements.emplace_back(entity, uiElement->textLayer);
                GN_LOG_DEBUG("UISystem: Found visible UI element entity " + std::to_string(entity) + " with text '" + uiElement->buttonText + "'");
            }
        }
        
        GN_LOG_DEBUG("UISystem: Rendering " + std::to_string(visibleUIElements.size()) + " visible UI elements");
        
        // Sort by layer (lower layers render first)
        std::sort(visibleUIElements.begin(), visibleUIElements.end(), 
                  [](const std::pair<Gnosis::Entity, int>& a, const std::pair<Gnosis::Entity, int>& b) {
                      return a.second < b.second;
                  });
        
        // Render all visible UI elements in layer order
        for (const auto& entityLayer : visibleUIElements) {
            Gnosis::Entity entity = entityLayer.first;
            Transform* transform = m_ecsCoordinator->GetComponent<Transform>(entity);
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
            
            if (transform && uiElement) {
                GN_LOG_DEBUG("UISystem: Rendering UI element entity " + std::to_string(entity) + " at position (" + 
                           std::to_string(transform->position.x) + ", " + std::to_string(transform->position.y) + ")");
                RenderUIElement(entity, *transform, *uiElement);
            }
        }
    }

    void UISystem::RenderUIElement(Gnosis::Entity entity, const Transform& transform, const UIElement& uiElement) {
        if (!m_delegates.renderer.drawText) {
            GN_LOG_ERROR("UISystem: drawText delegate is null");
            return;
        }

        // Use the new centered text function for proper centering
        // Scale font size for mobile devices (make it bigger for better readability)
        float mobileFontSize = uiElement.fontSize * 1.2f; // 20% larger for better readability
        
        // Choose text color based on button state
        float r, g, b, a;
        if (uiElement.isHovered) {
            r = uiElement.textHoverColor.r / 255.0f;
            g = uiElement.textHoverColor.g / 255.0f;
            b = uiElement.textHoverColor.b / 255.0f;
            a = uiElement.textHoverColor.a / 255.0f;
        } else {
            r = uiElement.textColor.r / 255.0f;
            g = uiElement.textColor.g / 255.0f;
            b = uiElement.textColor.b / 255.0f;
            a = uiElement.textColor.a / 255.0f;
        }
        
        GN_LOG_DEBUG("UISystem: Rendering centered text '" + uiElement.buttonText + "' at (" + 
                   std::to_string(transform.position.x) + ", " + std::to_string(transform.position.y) + ") with size " + 
                   std::to_string(mobileFontSize) + ", color (" + 
                   std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a) + ")");
        
        // Debug: Check if alpha is 0 (transparent)
        if (a < 0.01f) {
            GN_LOG_ERROR("UISystem: Text alpha is very low (" + std::to_string(a) + ") - text may be invisible!");
        }
        
        // Get actual sprite dimensions from the Sprite component
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        float buttonWidth, buttonHeight;
        
        if (sprite) {
            // Use actual sprite dimensions scaled by transform
            buttonWidth = sprite->width * transform.scale.x;
            buttonHeight = sprite->height * transform.scale.y;
        } else {
            // Fallback to default button dimensions
            buttonWidth = 90.0f * transform.scale.x;   // Default button texture width scaled
            buttonHeight = 16.0f * transform.scale.y;  // Default button texture height scaled
        }
        
        // Calculate center position of the button
        float buttonCenterX = transform.position.x + (buttonWidth * 0.5f);   // Center X of button
        float buttonCenterY = transform.position.y + (buttonHeight * 0.5f);  // Center Y of button
        
        GN_LOG_DEBUG("UISystem: Button bounds - position(" + std::to_string(transform.position.x) + ", " + std::to_string(transform.position.y) + 
                   "), size(" + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + 
                   "), center(" + std::to_string(buttonCenterX) + ", " + std::to_string(buttonCenterY) + ")");
        
        // Draw the text centered within the button bounds
        m_delegates.renderer.drawTextCentered(uiElement.buttonText, buttonCenterX, buttonCenterY, mobileFontSize, r, g, b, a);
    }

    bool UISystem::IsEntityVisible(Gnosis::Entity entity) const {
        if (!m_ecsCoordinator) {
            return false;
        }
        
        Transform* transform = m_ecsCoordinator->GetComponent<Transform>(entity);
        UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
        
        // Check if both Transform and UIElement exist, and UIElement is visible
        return transform != nullptr && uiElement != nullptr && uiElement->visible;
    }

    Gnosis::Entity UISystem::CreateButton(const std::string& text, 
                                         float x, float y, 
                                         float scale,
                                         const std::string& normalTexture,
                                         const std::string& hoverTexture, 
                                         const std::string& pressedTexture,
                                         float fontSize) {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("UISystem: Cannot create button - ECS coordinator is null");
            return 0;
        }
        
        // Create entity
        Gnosis::Entity entity = m_ecsCoordinator->CreateEntity();
        
        // Create components
        Transform transform;
        transform.position = Gnosis::GNVector2(x, y);
        transform.scale = Gnosis::GNVector2(scale, scale);
        
        Sprite sprite;
        sprite.textureId = normalTexture;
        sprite.width = 200.0f;  // Default button sprite size
        sprite.height = 60.0f;
        
        UIElement uiElement;
        uiElement.buttonText = text;
        uiElement.fontSize = fontSize;
        uiElement.normalTextureId = normalTexture;
        uiElement.hoverTextureId = hoverTexture;
        uiElement.pressedTextureId = pressedTexture;
        uiElement.isEnabled = true;
        uiElement.textLayer = 1;
        uiElement.textColor = {255, 255, 255, 255};  // White text
        uiElement.textHoverColor = {255, 255, 0, 255};  // Yellow on hover
        
        // Create bounds component using actual button texture size (64x16)
        Bounds bounds(64.0f * scale, 16.0f * scale, 0.0f, 0.0f, true);
        
        // Increase font size for better readability
        uiElement.fontSize = fontSize * 1.2f;
        
        // Add components to entity
        m_ecsCoordinator->AddComponent<Transform>(entity, transform);
        m_ecsCoordinator->AddComponent<Sprite>(entity, sprite);
        m_ecsCoordinator->AddComponent<UIElement>(entity, uiElement);
        m_ecsCoordinator->AddComponent<Bounds>(entity, bounds);
        
        GN_LOG_INFO("UISystem: Created button '" + text + "' at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        return entity;
    }

    Gnosis::Entity UISystem::CreateButtonWithBounds(const std::string& text,
                                                   float x, float y,
                                                   float boundsWidth, float boundsHeight,
                                                   float scale,
                                                   const std::string& normalTexture,
                                                   const std::string& hoverTexture,
                                                   const std::string& pressedTexture,
                                                   float fontSize) {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("UISystem: Cannot create button - ECS coordinator is null");
            return 0;
        }
        
        // Create entity
        Gnosis::Entity entity = m_ecsCoordinator->CreateEntity();
        
        // Create components
        Transform transform;
        transform.position = Gnosis::GNVector2(x, y);
        transform.scale = Gnosis::GNVector2(scale, scale);
        
        Sprite sprite;
        sprite.textureId = normalTexture;
        sprite.width = 200.0f;  // Default button sprite size
        sprite.height = 60.0f;
        
        UIElement uiElement;
        uiElement.buttonText = text;
        uiElement.fontSize = fontSize;
        uiElement.normalTextureId = normalTexture;
        uiElement.hoverTextureId = hoverTexture;
        uiElement.pressedTextureId = pressedTexture;
        uiElement.isEnabled = true;
        uiElement.textLayer = 1;
        uiElement.textColor = {255, 255, 255, 255};  // White text
        uiElement.textHoverColor = {255, 255, 0, 255};  // Yellow on hover
        
        // Create custom bounds component
        Bounds bounds(boundsWidth, boundsHeight, 0.0f, 0.0f, false);
        
        // Increase font size for better readability
        uiElement.fontSize = fontSize * 1.2f;
        
        // Add components to entity
        m_ecsCoordinator->AddComponent<Transform>(entity, transform);
        m_ecsCoordinator->AddComponent<Sprite>(entity, sprite);
        m_ecsCoordinator->AddComponent<UIElement>(entity, uiElement);
        m_ecsCoordinator->AddComponent<Bounds>(entity, bounds);
        
        GN_LOG_INFO("UISystem: Created button with custom bounds '" + text + "' at (" + std::to_string(x) + ", " + std::to_string(y) + 
                   ") bounds: " + std::to_string(boundsWidth) + "x" + std::to_string(boundsHeight));
        return entity;
    }

    bool UISystem::IsPointInBounds(Gnosis::Entity entity, float x, float y) const {
        if (!m_ecsCoordinator) {
            return false;
        }
        
        Transform* transform = m_ecsCoordinator->GetComponent<Transform>(entity);
        Bounds* bounds = m_ecsCoordinator->GetComponent<Bounds>(entity);
        
        if (!transform || !bounds) {
            return false;
        }
        
        // Calculate bounds based on Bounds component
        float buttonWidth = bounds->width;
        float buttonHeight = bounds->height;
        float buttonLeft = transform->position.x + bounds->offsetX - (buttonWidth / 2.0f);
        float buttonRight = transform->position.x + bounds->offsetX + (buttonWidth / 2.0f);
        float buttonTop = transform->position.y + bounds->offsetY - (buttonHeight / 2.0f);
        float buttonBottom = transform->position.y + bounds->offsetY + (buttonHeight / 2.0f);
        
        return (x >= buttonLeft && x <= buttonRight && y >= buttonTop && y <= buttonBottom);
    }

    bool UISystem::GetEntityBounds(Gnosis::Entity entity, float& left, float& right, float& top, float& bottom) const {
        if (!m_ecsCoordinator) {
            return false;
        }
        
        Transform* transform = m_ecsCoordinator->GetComponent<Transform>(entity);
        Bounds* bounds = m_ecsCoordinator->GetComponent<Bounds>(entity);
        
        if (!transform || !bounds) {
            return false;
        }
        
        float buttonWidth = bounds->width;
        float buttonHeight = bounds->height;
        left = transform->position.x + bounds->offsetX - (buttonWidth / 2.0f);
        right = transform->position.x + bounds->offsetX + (buttonWidth / 2.0f);
        top = transform->position.y + bounds->offsetY - (buttonHeight / 2.0f);
        bottom = transform->position.y + bounds->offsetY + (buttonHeight / 2.0f);
        
        return true;
    }

    void UISystem::UpdateButtonSprite(Gnosis::Entity entity) {
        if (!m_ecsCoordinator) {
            return;
        }
        
        Sprite* sprite = m_ecsCoordinator->GetComponent<Sprite>(entity);
        UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
        
        if (!sprite || !uiElement) {
            return;
        }
        
        // Choose the appropriate texture based on button state
        std::string textureId;
        if (uiElement->isPressed) {
            textureId = uiElement->pressedTextureId;
        } else if (uiElement->isHovered) {
            textureId = uiElement->hoverTextureId;
        } else {
            textureId = uiElement->normalTextureId;
        }
        
        // Update the sprite texture
        sprite->textureId = textureId;
        GN_LOG_DEBUG("UISystem: Updated button sprite to: " + textureId);
    }

    void UISystem::ResetAllButtonStates() {
        if (!m_ecsCoordinator) {
            return;
        }
        
        // Get all entities with UIElement components
        auto entities = m_ecsCoordinator->GetEntitiesWithComponents<UIElement>();
        
        for (Gnosis::Entity entity : entities) {
            UIElement* uiElement = m_ecsCoordinator->GetComponent<UIElement>(entity);
            if (uiElement) {
                uiElement->isPressed = false;
                uiElement->isHovered = false;
                UpdateButtonSprite(entity);
            }
        }
    }

} // namespace GameCore 