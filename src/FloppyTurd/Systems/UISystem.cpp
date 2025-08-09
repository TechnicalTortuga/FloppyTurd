#include "UISystem.h"
#include "../../Engine/Core/GNLog.h"
#include <algorithm>

namespace GameCore {

    UISystem::UISystem(Gnosis::ECS* ecsCoordinator, const GameCore::PlatformDelegates& delegates)
        : m_ecsCoordinator(ecsCoordinator)
        , m_delegates(delegates)
        , m_screenInfoValid(false)
    {
        if (!m_ecsCoordinator) {
            GN_LOG_ERROR("UISystem: ECS coordinator is null");
        }
        
        GN_LOG_INFO("UISystem: Initialized with integrated UI rendering");
        
        // Initialize screen info and layout
        UpdateScreenInfo();
        SetupLayout();
    }

    void UISystem::Update(float deltaTime) {
        if (!m_ecsCoordinator) {
            return;
        }

        // UI system doesn't need update logic for now
        // Could add button animations, hover effects, etc. here in the future
    }

    void UISystem::Render() {
        // DISABLED: UISystem rendering is now handled by unified RenderSystem
        // This prevents duplicate rendering while preserving UI logic functionality
        GN_LOG_DEBUG("UISystem::Render() disabled - using unified RenderSystem for all rendering");
        return;
    }

    void UISystem::RenderUIElement(Gnosis::Entity entity, const Transform& transform, const UIElement& uiElement) {
        if (!m_delegates.renderer.drawText) {
            GN_LOG_ERROR("UISystem: drawText delegate is null");
            return;
        }

        // Use the font size as specified without additional scaling
        float mobileFontSize = uiElement.fontSize;
        
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
        
        // === IMPROVED TEXT POSITIONING WITH SDF BASELINE CORRECTION === //
        
        // Calculate text position based on centering properties
        float textX, textY;
        
        // Check if this is a text-only element (no button background)
        bool isTextOnly = uiElement.normalTextureId.empty();
        
        if (uiElement.centerTextHorizontally) {
            if (isTextOnly) {
                // For text-only elements, transform.position.x is already the center coordinate
                textX = transform.position.x;
            } else {
                // For button elements, transform.position.x is top-left, so calculate center
                textX = transform.position.x + (buttonWidth * 0.5f);   // Center X of button
            }
        } else {
            textX = transform.position.x;  // Top-left X of button or text position
        }
        
        if (uiElement.centerTextVertically) {
            if (isTextOnly) {
                // For text-only elements, transform.position.y is already the center coordinate
                textY = transform.position.y;
            } else {
                // For button elements, calculate the visual center Y position for text rendering
                // Start from the mathematical center of the button
                float buttonCenterY = transform.position.y + (buttonHeight * 0.5f);
                
                // Apply visual correction for SDF font baseline (font-specific adjustment)
                // Most fonts have their visual center slightly below the mathematical center
                float visualAdjustment = mobileFontSize * 0.15f;  // 15% of font size down from center
                textY = buttonCenterY + visualAdjustment;
            }
            
            // Note: MetalRenderer.drawTextCentered now only handles horizontal centering
            // All vertical positioning logic is handled here in UISystem
        } else {
            textY = transform.position.y;  // Top-left Y of button or text position
        }
        
        // Apply manual text offsets (after baseline correction)
        textX += uiElement.textOffsetX;
        textY += uiElement.textOffsetY;
        
        GN_LOG_DEBUG("UISystem: Text '" + uiElement.buttonText + "' - Button bounds: pos(" + std::to_string(transform.position.x) + "," + std::to_string(transform.position.y) + ") size(" + std::to_string(buttonWidth) + "x" + std::to_string(buttonHeight) + "), Text center: (" + std::to_string(textX) + "," + std::to_string(textY) + "), Font: " + std::to_string(mobileFontSize));
        
        // Choose appropriate text rendering function based on centering
        if (uiElement.centerTextHorizontally && uiElement.centerTextVertically) {
            // Use centered text rendering with improved baseline correction
            GN_LOG_DEBUG("UISystem: Drawing centered text '" + uiElement.buttonText + "' at (" + std::to_string(textX) + "," + std::to_string(textY) + ")");
            
            // Draw the actual centered text
            m_delegates.renderer.drawTextCentered(uiElement.buttonText, textX, textY, mobileFontSize, r, g, b, a);
        } else {
            // Use regular text rendering for non-centered text
            GN_LOG_DEBUG("UISystem: Drawing left-aligned text '" + uiElement.buttonText + "' at (" + std::to_string(textX) + "," + std::to_string(textY) + ")");
            m_delegates.renderer.drawText(uiElement.buttonText, textX, textY, mobileFontSize, r, g, b, a);
        }
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
        
        // Use the font size as specified
        uiElement.fontSize = fontSize;
        
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
        
        // Use the font size as specified
        uiElement.fontSize = fontSize;
        
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
    
    // Responsive layout implementation
    void UISystem::UpdateScreenInfo() {
        if (m_delegates.renderer.getScreenInfo) {
            m_delegates.renderer.getScreenInfo(&m_screenInfo);
            m_screenInfoValid = true;
            
            GN_LOG_INFO("UISystem: Screen info updated: " + 
                       std::to_string((int)m_screenInfo.logicalWidth) + "x" + 
                       std::to_string((int)m_screenInfo.logicalHeight));
        } else if (m_delegates.renderer.getScreenSize) {
            // Fallback to legacy screen size
            m_delegates.renderer.getScreenSize(&m_screenInfo.logicalWidth, &m_screenInfo.logicalHeight);
            m_screenInfo.pixelWidth = m_screenInfo.logicalWidth;
            m_screenInfo.pixelHeight = m_screenInfo.logicalHeight;
            m_screenInfo.scaleFactor = 1.0f;
            m_screenInfo.isPortrait = m_screenInfo.logicalHeight > m_screenInfo.logicalWidth;
            m_screenInfo.deviceModel = "Unknown";
            m_screenInfoValid = true;
            
            GN_LOG_WARN("UISystem: Using legacy screen size: " + 
                       std::to_string((int)m_screenInfo.logicalWidth) + "x" + 
                       std::to_string((int)m_screenInfo.logicalHeight));
        } else {
            GN_LOG_ERROR("UISystem: No screen size information available");
            m_screenInfoValid = false;
        }
    }
    
    float UISystem::GetResponsiveScale() const {
        if (!m_screenInfoValid) {
            return 1.0f;
        }
        
        // Calculate responsive scale based on screen size
        // Use logical height as base reference (iPhone 16 logical height is 852)
        const float referenceHeight = 852.0f;  // iPhone 16 logical height
        return m_screenInfo.logicalHeight / referenceHeight;
    }
    
    float UISystem::GetUIScale() const {
        if (!m_screenInfoValid) {
            return 1.0f;
        }
        
        // UI scaling with reasonable bounds
        float baseScale = GetResponsiveScale();
        return std::max(0.8f, std::min(2.0f, baseScale));
    }
    
    void UISystem::SetupLayout() {
        if (!m_screenInfoValid) {
            GN_LOG_WARN("UISystem: Cannot setup layout - screen info not valid");
            return;
        }
        
#ifdef PLATFORM_IOS
        SetupIOSLayout();
#else
        SetupDesktopLayout();
#endif
    }
    
    void UISystem::SetupIOSLayout() {
        GN_LOG_INFO("UISystem: Setting up iOS layout for device: " + m_screenInfo.deviceModel);
        
        float scale = GetUIScale();
        GN_LOG_INFO("UISystem: Calculated UI scale: " + std::to_string(scale));
        
        // iOS-specific UI setup can go here
        // e.g., safe area calculations, notch handling, etc.
    }
    
    void UISystem::SetupDesktopLayout() {
        GN_LOG_INFO("UISystem: Setting up desktop layout");
        
        float scale = GetUIScale();
        GN_LOG_INFO("UISystem: Calculated UI scale: " + std::to_string(scale));
        
        // Desktop-specific UI setup can go here
    }
    
    void UISystem::GetSafeArea(float& left, float& top, float& right, float& bottom) const {
        if (!m_screenInfoValid) {
            left = top = right = bottom = 0.0f;
            return;
        }
        
        // Default safe area - can be enhanced with platform-specific notch detection
        left = 0.0f;
        top = 0.0f;
        right = m_screenInfo.logicalWidth;
        bottom = m_screenInfo.logicalHeight;
        
#ifdef PLATFORM_IOS
        // iOS safe area considerations (notch, home indicator, etc.)
        // These values can be refined based on device model
        if (m_screenInfo.deviceModel.find("iPhone") != std::string::npos) {
            top = 44.0f;  // Status bar / notch area
            bottom = m_screenInfo.logicalHeight - 34.0f;  // Home indicator area
        }
#endif
    }
    
    float UISystem::CalculateResponsivePosition(float basePosition, bool isHorizontal) const {
        if (!m_screenInfoValid) {
            return basePosition;
        }
        
        float scale = GetUIScale();
        if (isHorizontal) {
            // Scale horizontal position based on screen width
            return basePosition * (m_screenInfo.logicalWidth / 393.0f); // iPhone 16 reference
        } else {
            // Scale vertical position based on screen height
            return basePosition * (m_screenInfo.logicalHeight / 852.0f); // iPhone 16 reference
        }
    }
    
    float UISystem::CalculateResponsiveSize(float baseSize) const {
        if (!m_screenInfoValid) {
            return baseSize;
        }
        
        return baseSize * GetUIScale();
    }

} // namespace GameCore 