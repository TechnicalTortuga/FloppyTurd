# Toggle Button System

## Overview

The UISystem now provides built-in support for toggle buttons (checkboxes, switches, etc.) with atomic texture swapping to prevent visual flashing during state changes.

## Why Use This System?

When a toggle button changes state, both the `Sprite` and `UIElement` components need to update their textures. If these updates happen separately or through the normal button state system (hover/pressed), there can be a single-frame flash where the textures don't match. The toggle button system ensures atomic updates.

## UIElement Toggle Fields

The `UIElement` component has these toggle-specific fields:

```cpp
bool isToggle;              // Set to true to enable toggle behavior
bool toggleState;           // Current state: true = ON, false = OFF
std::string toggleOnTexture;   // Texture ID when toggle is ON
std::string toggleOffTexture;  // Texture ID when toggle is OFF
```

## How to Create a Toggle Button

### Step 1: Create the Entity and Components

```cpp
// Create entity
Entity toggleEntity = m_ecsCoordinator->CreateEntity();

// Create Transform
Transform t(GNVector2(x, y), 0.0f, GNVector2(scale, scale));

// Create Sprite with initial texture
Sprite s(initialState ? "on_texture" : "off_texture", 64, 64);
s.visible = true;
s.layer = yourLayer;

// Create UIElement configured as toggle
UIElement ui("", 
    initialState ? "on_texture" : "off_texture",
    initialState ? "on_texture" : "off_texture");
ui.visible = true;
ui.isEnabled = true;
ui.isToggle = true;  // IMPORTANT: Mark as toggle button
ui.toggleState = initialState;  // Set initial state
ui.toggleOnTexture = "on_texture";   // ON texture
ui.toggleOffTexture = "off_texture"; // OFF texture

// Add components
m_ecsCoordinator->AddComponent<Transform>(toggleEntity, t);
m_ecsCoordinator->AddComponent<Sprite>(toggleEntity, s);
m_ecsCoordinator->AddComponent<UIElement>(toggleEntity, ui);

// CRITICAL: Force-load BOTH textures to GPU to prevent first-click flash
// The sprite only loads the initial texture when created. We need to ensure
// BOTH toggle states are GPU-loaded so the first toggle is instant.
if (auto* rs = m_ecsCoordinator->GetSystemManager()->GetRenderSystem()) {
    rs->GetOrLoadTexture("texture_on", toggleEntity);
    rs->GetOrLoadTexture("texture_off", toggleEntity);
}
```

### Step 2: Handle Toggle Events

When the user clicks the toggle button (inline within your click handler):

```cpp
// Inside your HandleInput() or similar function
if (justPressed && toggleEntity != 0) {
    auto transform = m_ecsCoordinator->GetComponent<Transform>(toggleEntity);
    auto uiElement = m_ecsCoordinator->GetComponent<UIElement>(toggleEntity);
    
    if (transform && uiElement && uiElement->isEnabled && uiElement->visible) {
        // Check if click is within bounds
        if (IsClickInBounds(touchX, touchY, transform, uiElement)) {
            // Update your game state
            bool newState = !currentState;
            UpdateGameSetting(newState);
            
            // Use UISystem to atomically update the toggle button
            if (auto uiSystem = m_ecsCoordinator->GetSystemManager()->GetUISystem()) {
                uiSystem->SetToggleState(toggleEntity, newState);
            }
        }
    }
}
```

## UISystem Methods

### SetToggleState(entity, state)
Sets the toggle state and updates textures atomically.

```cpp
bool SetToggleState(Gnosis::Entity entity, bool state);
```

- **entity**: The entity with the toggle button
- **state**: The new state (true = ON, false = OFF)
- **Returns**: true if successful, false if entity is not a toggle button

### ToggleButton(entity)
Toggles the current state and updates textures atomically.

```cpp
bool ToggleButton(Gnosis::Entity entity);
```

- **entity**: The entity with the toggle button
- **Returns**: true if successful, false if entity is not a toggle button

### UpdateButtonSprite(entity)
Automatically handles both regular buttons and toggle buttons. For toggle buttons, uses the toggle state to determine the texture. Called automatically by `SetToggleState()` and `ToggleButton()`.

```cpp
void UpdateButtonSprite(Gnosis::Entity entity);
```

## Example: Vibration Toggle

See `MainMenuState::CreateOptionsTracksAndLabels()` for toggle button creation and `MainMenuState::HandleOptionsInput()` for inline click handling of the vibration toggle.

## Best Practices

1. **Always set `isToggle = true`** when creating toggle buttons
2. **Use `SetToggleState()` or `ToggleButton()`** instead of manually updating textures
3. **Preload both textures** in LoadingState - they will be automatically loaded to GPU
4. **Keep texture IDs without .png extension** (the system adds it automatically)
5. **Use descriptive texture names** like "xbuttonselected" and "xbuttonunselected"

### Texture Preloading

Both toggle textures MUST be included in the `GAME_TEXTURES` array in `LoadingState.cpp`:

```cpp
static const std::vector<std::string> GAME_TEXTURES = {
    // ... other textures ...
    "xbuttonselected",     // Toggle ON state (NO .png extension!)
    "xbuttonunselected",   // Toggle OFF state (NO .png extension!)
    // ... other textures ...
};
```

**CRITICAL**: Do NOT include file extensions in the preload list! The iOS asset catalog doesn't use extensions. Texture IDs must match exactly how they're referenced in code.

When `RenderSystem::PreloadTextures()` is called during loading, it will fully load these textures to the GPU. This ensures the first toggle has no delay or flashing.

**System Enhancement (November 2025)**: 
1. `PreloadTexture()` was updated to force GPU upload using `GetOrLoadTexture()` instead of just metadata caching
2. Fixed texture naming - removed all `.png` extensions from `GAME_TEXTURES` to match code references
3. This ensures textures in the preload list are truly GPU-resident and ready for instant use

## Future Enhancements

Potential improvements for the system:
- Animation support during toggle transitions
- Sound effect integration
- Haptic feedback integration (currently handled externally)
- Toggle groups (radio buttons where only one can be selected)
