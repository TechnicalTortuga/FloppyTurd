# FloppyTurd - UI Enhancements & IAP Implementation Tracker

**Project**: FloppyTurd iOS  
**Phase**: 6 & 7 - UI Enhancements and In-App Purchase Integration  
**Version**: 1.0.0  
**Date**: 2024  
**Status**: Planning → Implementation

---

## Overview

This document provides a detailed, step-by-step implementation plan for:
1. **Ad Controls UI** - New menu for managing ad-related settings and purchases
2. **Vibration Toggle** - Options menu enhancement for haptic feedback control
3. **Version Number Display** - Main menu version indicator
4. **Remove Ads IAP** - StoreKit 2 in-app purchase integration

**Estimated Timeline**: 2-3 days for Phase 6 (UI), 1-2 days for Phase 7 (IAP)

---

## Table of Contents

1. [Asset Verification](#asset-verification)
2. [Phase 6A: Main Menu Enhancements](#phase-6a-main-menu-enhancements)
3. [Phase 6B: Ad Controls Menu](#phase-6b-ad-controls-menu)
4. [Phase 6C: Options Menu - Vibration Toggle](#phase-6c-options-menu---vibration-toggle)
5. [Phase 7: IAP Integration](#phase-7-iap-integration)
6. [Testing Checklist](#testing-checklist)
7. [Design Specifications](#design-specifications)

---

## Asset Verification

### Assets Confirmed ✅
- [x] `src/assets/graphics/ui/hud/adcontrolsbutton.png` - Ad controls button sprite
- [x] `src/assets/graphics/ui/hud/xbuttonselected.png` - Vibration ON state (X mark visible)
- [x] `src/assets/graphics/ui/hud/xbuttonunselected.png` - Vibration OFF state (no X mark)

### Asset Integration Checklist
- [ ] Verify all three assets are in correct directory
- [ ] Test asset loading in engine
- [ ] Confirm sprites render at expected scale
- [ ] Test on iPhone (various screen sizes)
- [ ] Test on iPad (if supported)
- [ ] Verify assets are included in Xcode asset catalog build

---

## Phase 6A: Main Menu Enhancements

**Goal**: Add ad controls button (bottom left) and version number (bottom right) to main menu.

### Task 6A.1: Ad Controls Button Integration

**Files to Modify**:
- `src/FloppyTurd/States/MainMenuState.h`
- `src/FloppyTurd/States/MainMenuState.cpp`

#### Step 1: Add Entity to MainMenuState.h
```cpp
// In MainMenuState class private section, with other UI entities:
Gnosis::Entity m_adControlsButtonEntity;
```

#### Step 2: Add Menu Mode Enum Value
```cpp
// In MainMenuState::MenuMode enum:
enum class MenuMode {
    MAIN_MENU,
    LEVEL_SELECT,
    OPTIONS,
    AD_CONTROLS  // ADD THIS
};
```

#### Step 3: Create Button in Layout Functions
Location: `MainMenuState.cpp` - in `CreateMobileLayout()` or `CreateDesktopLayout()`

```cpp
// Position: Bottom left corner
float adButtonX = m_screenWidth * 0.05f;  // 5% from left
float adButtonY = m_screenHeight * 0.95f;  // 95% down (near bottom)
float adButtonScale = 6.0f;  // Specified 6x scale

// Create entity
if (m_adControlsButtonEntity == 0) {
    m_adControlsButtonEntity = m_ecsCoordinator->CreateEntity();
}

Transform t(Gnosis::GNVector2(adButtonX, adButtonY), 0.0f, Gnosis::GNVector2(adButtonScale, adButtonScale));
Sprite s;
s.texturePath = "graphics/ui/hud/adcontrolsbutton";
s.visible = true;
s.layer = 5;

UIElement ui("", "graphics/ui/hud/adcontrolsbutton", "");
ui.isButton = true;
ui.visible = true;
ui.interactable = true;

// Add components...
```

#### Step 4: Add Input Handler
Location: `MainMenuState.cpp` - in `HandleMainMenuInput()` or `CheckMenuButtonClicks()`

```cpp
void MainMenuState::OnAdControlsButtonPressed() {
    // Transition to ad controls menu
    m_currentMode = MenuMode::AD_CONTROLS;
    SetMainMenuVisible(false);
    ShowAdControlsMenu();
}

// In CheckMenuButtonClicks():
// Check ad controls button
if (m_adControlsButtonEntity != 0) {
    auto transform = m_ecsCoordinator->GetComponent<Transform>(m_adControlsButtonEntity);
    auto uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_adControlsButtonEntity);
    
    if (transform && uiElement && uiElement->interactable) {
        // Calculate button bounds with scale
        float buttonW = 64.0f * 6.0f;  // Assuming 64px sprite at 6x scale
        float buttonH = 64.0f * 6.0f;
        
        if (touchX >= transform->position.x && touchX <= transform->position.x + buttonW &&
            touchY >= transform->position.y && touchY <= transform->position.y + buttonH) {
            OnAdControlsButtonPressed();
        }
    }
}
```

**Checklist**:
- [ ] Add `m_adControlsButtonEntity` to header
- [ ] Add `AD_CONTROLS` to `MenuMode` enum
- [ ] Create button in layout function (bottom left, 6x scale)
- [ ] Load `adcontrolsbutton` texture
- [ ] Add click detection in `CheckMenuButtonClicks()`
- [ ] Implement `OnAdControlsButtonPressed()` handler
- [ ] Test button appears on main menu
- [ ] Test button click transitions to ad controls menu
- [ ] Verify button position on different screen sizes

---

### Task 6A.2: Version Number Display

**Files to Modify**:
- `src/FloppyTurd/States/MainMenuState.h`
- `src/FloppyTurd/States/MainMenuState.cpp`

#### Step 1: Add Entity to MainMenuState.h
```cpp
// In MainMenuState class private section:
Gnosis::Entity m_versionTextEntity;
```

#### Step 2: Create Version Text in Layout
Location: `MainMenuState.cpp` - in `CreateMobileLayout()` or `CreateDesktopLayout()`

```cpp
// Position: Bottom right corner (opposite ad controls button)
float versionX = m_screenWidth * 0.95f;  // 95% from left (5% from right)
float versionY = m_screenHeight * 0.95f;  // 95% down (same height as ad button)

if (m_versionTextEntity == 0) {
    m_versionTextEntity = m_ecsCoordinator->CreateEntity();
}

Transform t(Gnosis::GNVector2(versionX, versionY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
Sprite s;
s.visible = false;  // Text only
s.layer = 5;

UIElement ui("1.0.0", "", "");  // Current version from CMakeLists.txt
ui.fontSize = m_isMobile ? 48.0f : 28.0f;
ui.textColor = Gnosis::GNColor(255, 255, 255, 255);  // White
ui.centerTextHorizontally = false;
ui.centerTextVertically = true;
ui.visible = true;

// Add components...
```

**Checklist**:
- [ ] Add `m_versionTextEntity` to header
- [ ] Create version text entity (bottom right corner)
- [ ] Display "1.0.0" text
- [ ] Set appropriate font size (48pt mobile, 28pt desktop)
- [ ] White text color
- [ ] Right-aligned (or manually position with padding)
- [ ] Test visibility on different screen sizes
- [ ] Verify doesn't overlap with ad controls button
- [ ] Consider adding "v" prefix (e.g., "v1.0.0") for clarity

---

## Phase 6B: Ad Controls Menu

**Goal**: Create new menu similar to leaderboard menu with "Remove Ads" label and "$2.00" button.

### Task 6B.1: Menu Structure Setup

**Files to Modify**:
- `src/FloppyTurd/States/MainMenuState.h`
- `src/FloppyTurd/States/MainMenuState.cpp`

#### Step 1: Add Entities to Header
```cpp
// In MainMenuState class private section:
// Ad Controls Menu entities
Gnosis::Entity m_adControlsTitleEntity;
Gnosis::Entity m_adControlsBackButtonEntity;
Gnosis::Entity m_removeAdsLabelEntity;
Gnosis::Entity m_removeAdsPriceButtonEntity;
```

#### Step 2: Create ShowAdControlsMenu() Function
```cpp
void MainMenuState::ShowAdControlsMenu() {
    CreateAdControlsLayout();
}

void MainMenuState::CreateAdControlsLayout() {
    if (!m_ecsCoordinator) return;
    
    // Calculate center overlay (similar to leaderboard/options)
    float overlayW = m_screenWidth * 0.8f;
    float overlayH = m_screenHeight * 0.7f;
    float overlayX = (m_screenWidth - overlayW) * 0.5f;
    float overlayY = (m_screenHeight - overlayH) * 0.5f;
    
    CreateAdControlsTitle(overlayX, overlayY, overlayW, overlayH);
    CreateRemoveAdsLabel(overlayX, overlayY, overlayW, overlayH);
    CreateRemoveAdsPriceButton(overlayX, overlayY, overlayW, overlayH);
    CreateAdControlsBackButton(overlayX, overlayY, overlayW, overlayH);
}
```

**Checklist**:
- [ ] Add ad controls menu entity declarations to header
- [ ] Implement `ShowAdControlsMenu()`
- [ ] Implement `CreateAdControlsLayout()`
- [ ] Add `HideAdControlsMenu()` function
- [ ] Calculate overlay dimensions (similar to other menus)

---

### Task 6B.2: Title Implementation

**Implementation**:
```cpp
void MainMenuState::CreateAdControlsTitle(float overlayX, float overlayY, float overlayW, float overlayH) {
    // Title centered at top
    float titleX = overlayX + overlayW * 0.5f;
    float titleY = overlayY + overlayH * 0.08f;
    
    if (m_adControlsTitleEntity == 0) {
        m_adControlsTitleEntity = m_ecsCoordinator->CreateEntity();
    }
    
    Transform t(Gnosis::GNVector2(titleX, titleY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    Sprite s;
    s.visible = false;
    s.layer = 4;
    
    UIElement ui("AD CONTROLS", "", "");
    ui.fontSize = m_isMobile ? 72.0f : 42.0f;
    ui.textColor = Gnosis::GNColor(255, 255, 255, 255);
    ui.centerTextHorizontally = true;
    ui.centerTextVertically = true;
    ui.visible = true;
    
    // Add components...
}
```

**Checklist**:
- [ ] Implement `CreateAdControlsTitle()`
- [ ] Display "AD CONTROLS" text
- [ ] Center horizontally at top of overlay
- [ ] Large font size (72pt mobile, 42pt desktop)
- [ ] White text
- [ ] Test visibility and positioning

---

### Task 6B.3: Remove Ads Label

**Implementation**:
```cpp
void MainMenuState::CreateRemoveAdsLabel(float overlayX, float overlayY, float overlayW, float overlayH) {
    // Label centered vertically in overlay
    float labelX = overlayX + overlayW * 0.5f;
    float labelY = overlayY + overlayH * 0.4f;
    
    if (m_removeAdsLabelEntity == 0) {
        m_removeAdsLabelEntity = m_ecsCoordinator->CreateEntity();
    }
    
    Transform t(Gnosis::GNVector2(labelX, labelY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    Sprite s;
    s.visible = false;
    s.layer = 4;
    
    UIElement ui("Remove Ads", "", "");
    ui.fontSize = m_isMobile ? 64.0f : 38.0f;
    ui.textColor = Gnosis::GNColor(255, 255, 255, 255);
    ui.centerTextHorizontally = true;
    ui.centerTextVertically = true;
    ui.visible = true;
    
    // Add components...
}
```

**Checklist**:
- [ ] Implement `CreateRemoveAdsLabel()`
- [ ] Display "Remove Ads" text
- [ ] Center horizontally
- [ ] Position in middle/upper-middle of overlay
- [ ] Font size (64pt mobile, 38pt desktop)
- [ ] White text
- [ ] Test positioning

---

### Task 6B.4: $2.00 Price Button

**Implementation**:
```cpp
void MainMenuState::CreateRemoveAdsPriceButton(float overlayX, float overlayY, float overlayW, float overlayH) {
    // Button at bottom right of overlay (opposite back button)
    float buttonX = overlayX + overlayW * 0.75f;  // 75% across overlay
    float buttonY = overlayY + overlayH * 0.85f;  // 85% down overlay
    
    if (m_removeAdsPriceButtonEntity == 0) {
        m_removeAdsPriceButtonEntity = m_ecsCoordinator->CreateEntity();
    }
    
    Transform t(Gnosis::GNVector2(buttonX, buttonY), 0.0f, Gnosis::GNVector2(8.0f, 8.0f));
    Sprite s;
    // Use existing button sprite or create new one
    s.texturePath = "graphics/ui/menus/main/button_normal";  // Reuse existing button
    s.visible = true;
    s.layer = 4;
    
    UIElement ui("$2.00", "graphics/ui/menus/main/button_normal", "graphics/ui/menus/main/button_pressed");
    ui.fontSize = m_isMobile ? 54.0f : 32.0f;
    ui.textColor = Gnosis::GNColor(255, 255, 255, 255);
    ui.centerTextHorizontally = true;
    ui.centerTextVertically = true;
    ui.isButton = true;
    ui.interactable = true;
    ui.visible = true;
    
    // Add components...
}
```

**Checklist**:
- [ ] Implement `CreateRemoveAdsPriceButton()`
- [ ] Display "$2.00" text (EXACTLY $2.00, not $1.99 💩)
- [ ] Position bottom right of overlay
- [ ] Opposite position from back button
- [ ] Match button style with existing menu buttons
- [ ] Scale 8x (or match other menu buttons)
- [ ] Add click handler (placeholder for now)
- [ ] Test button appearance and position

---

### Task 6B.5: Back Button

**Implementation**:
```cpp
void MainMenuState::CreateAdControlsBackButton(float overlayX, float overlayY, float overlayW, float overlayH) {
    // Back button at bottom left of overlay
    float buttonX = overlayX + overlayW * 0.15f;  // 15% across overlay
    float buttonY = overlayY + overlayH * 0.85f;  // 85% down overlay
    
    if (m_adControlsBackButtonEntity == 0) {
        m_adControlsBackButtonEntity = m_ecsCoordinator->CreateEntity();
    }
    
    // Reuse existing back button implementation from level select
    // ... (similar to level select back button)
}
```

**Checklist**:
- [ ] Implement `CreateAdControlsBackButton()`
- [ ] Reuse back button sprite/style from level select
- [ ] Position bottom left of overlay
- [ ] Add click handler to return to main menu
- [ ] Test back navigation

---

### Task 6B.6: Input Handling

**Implementation**:
```cpp
void MainMenuState::HandleAdControlsInput() {
    // Get touch/click input
    float touchX = /* get from input system */;
    float touchY = /* get from input system */;
    
    // Check back button
    if (/* back button clicked */) {
        OnAdControlsBackButtonPressed();
    }
    
    // Check price button
    if (/* price button clicked */) {
        OnRemoveAdsPurchasePressed();
    }
}

void MainMenuState::OnAdControlsBackButtonPressed() {
    HideAdControlsMenu();
    m_currentMode = MenuMode::MAIN_MENU;
    SetMainMenuVisible(true);
}

void MainMenuState::OnRemoveAdsPurchasePressed() {
    // Placeholder for Phase 7 IAP integration
    std::cout << "Remove Ads purchase button pressed - IAP not yet implemented\n";
    // TODO: Call StoreManager.purchase("com.floppyturd.game.removeads")
}
```

**Checklist**:
- [ ] Implement `HandleAdControlsInput()`
- [ ] Add to main `HandleInput()` switch on menu mode
- [ ] Implement `OnAdControlsBackButtonPressed()`
- [ ] Implement `OnRemoveAdsPurchasePressed()` (placeholder)
- [ ] Test back button navigation
- [ ] Test price button shows placeholder message
- [ ] Verify button click detection works

---

### Task 6B.7: Menu Visibility Management

**Implementation**:
```cpp
void MainMenuState::HideAdControlsMenu() {
    // Hide all ad controls menu entities
    if (m_adControlsTitleEntity != 0) {
        auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_adControlsTitleEntity);
        if (ui) ui->visible = false;
    }
    
    if (m_adControlsBackButtonEntity != 0) {
        auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_adControlsBackButtonEntity);
        if (ui) ui->visible = false;
    }
    
    if (m_removeAdsLabelEntity != 0) {
        auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_removeAdsLabelEntity);
        if (ui) ui->visible = false;
    }
    
    if (m_removeAdsPriceButtonEntity != 0) {
        auto ui = m_ecsCoordinator->GetComponent<UIElement>(m_removeAdsPriceButtonEntity);
        if (ui) ui->visible = false;
    }
}
```

**Checklist**:
- [ ] Implement `HideAdControlsMenu()`
- [ ] Hide all ad controls entities
- [ ] Ensure proper state cleanup
- [ ] Test menu shows/hides correctly
- [ ] Verify no visual glitches during transitions

---

## Phase 6C: Options Menu - Vibration Toggle

**Goal**: Add vibration toggle row below difficulty section using X button sprites.

### Task 6C.1: Add Toggle Entities

**Files to Modify**:
- `src/FloppyTurd/States/MainMenuState.h`
- `src/FloppyTurd/States/MainMenuState.cpp`

#### Step 1: Add Entities and State to Header
```cpp
// In MainMenuState class private section:
// Vibration toggle entities
Gnosis::Entity m_vibrationLabelEntity;
Gnosis::Entity m_vibrationToggleEntity;
bool m_vibrationsEnabled = true;  // Default ON
```

#### Step 2: Update CreateOptionsTracksAndLabels()
Location: `MainMenuState.cpp` - after difficulty section in `CreateOptionsTracksAndLabels()`

```cpp
// Add after difficulty section (below diffValueY + arrows)

// Vibration toggle row
if (m_vibrationLabelEntity == 0) m_vibrationLabelEntity = m_ecsCoordinator->CreateEntity();
if (m_vibrationToggleEntity == 0) m_vibrationToggleEntity = m_ecsCoordinator->CreateEntity();

float vibrationLabelY = m_screenHeight * 0.80f;  // Below difficulty at 80%
float vibrationToggleY = vibrationLabelY;  // Same row
float centerX = m_optionsOverlayX + m_optionsOverlayW * 0.5f;

// Label "VIBRATION" on left side
{
    float labelX = m_optionsOverlayX + m_optionsOverlayW * 0.3f;  // 30% across
    Transform t(Gnosis::GNVector2(labelX, vibrationLabelY), 0.0f, Gnosis::GNVector2(1.0f, 1.0f));
    Sprite s; s.visible = false; s.layer = 4;
    UIElement ui("VIBRATION", "", "");
    ui.fontSize = m_isMobile ? 54.0f : 32.0f;
    ui.textColor = Gnosis::GNColor(255, 255, 255, 255);
    ui.centerTextHorizontally = false;
    ui.centerTextVertically = true;
    ui.visible = true;
    // Add components...
}

// Toggle button (X sprite) on right side
{
    float toggleX = m_optionsOverlayX + m_optionsOverlayW * 0.7f;  // 70% across
    Transform t(Gnosis::GNVector2(toggleX, vibrationToggleY), 0.0f, Gnosis::GNVector2(6.0f, 6.0f));
    Sprite s;
    // Start with current state
    s.texturePath = m_vibrationsEnabled ? 
        "graphics/ui/hud/xbuttonselected" : 
        "graphics/ui/hud/xbuttonunselected";
    s.visible = true;
    s.layer = 4;
    
    UIElement ui("", 
        "graphics/ui/hud/xbuttonselected",    // ON state
        "graphics/ui/hud/xbuttonunselected");  // OFF state (pressed sprite used for toggle)
    ui.isButton = true;
    ui.interactable = true;
    ui.visible = true;
    // Add components...
}
```

**Checklist**:
- [ ] Add `m_vibrationLabelEntity` to header
- [ ] Add `m_vibrationToggleEntity` to header
- [ ] Add `m_vibrationsEnabled` bool to header
- [ ] Create vibration label in options menu
- [ ] Create X button toggle sprite
- [ ] Position below difficulty section (80% down)
- [ ] Label on left, toggle on right
- [ ] Load both X button states (selected/unselected)
- [ ] Initialize with current vibration preference
- [ ] Scale X button appropriately (6x suggested)
- [ ] Test toggle appears in options menu

---

### Task 6C.2: Toggle Click Handling

**Implementation**:
```cpp
void MainMenuState::HandleOptionsInput() {
    // ... existing options input handling ...
    
    // Check vibration toggle click
    if (m_vibrationToggleEntity != 0) {
        auto transform = m_ecsCoordinator->GetComponent<Transform>(m_vibrationToggleEntity);
        auto uiElement = m_ecsCoordinator->GetComponent<UIElement>(m_vibrationToggleEntity);
        auto sprite = m_ecsCoordinator->GetComponent<Sprite>(m_vibrationToggleEntity);
        
        if (transform && uiElement && sprite && uiElement->interactable) {
            float toggleW = 64.0f * 6.0f;  // Assuming 64px at 6x scale
            float toggleH = 64.0f * 6.0f;
            
            if (touchX >= transform->position.x && touchX <= transform->position.x + toggleW &&
                touchY >= transform->position.y && touchY <= transform->position.y + toggleH) {
                OnVibrationTogglePressed();
            }
        }
    }
}

void MainMenuState::OnVibrationTogglePressed() {
    // Toggle state
    m_vibrationsEnabled = !m_vibrationsEnabled;
    
    // Update sprite
    auto sprite = m_ecsCoordinator->GetComponent<Sprite>(m_vibrationToggleEntity);
    if (sprite) {
        sprite->texturePath = m_vibrationsEnabled ? 
            "graphics/ui/hud/xbuttonselected" : 
            "graphics/ui/hud/xbuttonunselected";
    }
    
    // Save preference
    SaveVibrationPreference(m_vibrationsEnabled);
    
    // Optional: Play haptic feedback for the toggle itself (if enabled)
    if (m_vibrationsEnabled && m_platformDelegates) {
        m_platformDelegates->TriggerHapticFeedback(HapticType::Selection);
    }
    
    std::cout << "Vibration toggled: " << (m_vibrationsEnabled ? "ON" : "OFF") << "\n";
}
```

**Checklist**:
- [ ] Add click detection for vibration toggle in `HandleOptionsInput()`
- [ ] Implement `OnVibrationTogglePressed()`
- [ ] Toggle `m_vibrationsEnabled` state
- [ ] Update sprite texture based on state
- [ ] Call `SaveVibrationPreference()`
- [ ] Optional: Play haptic on toggle (if enabled)
- [ ] Test toggle switches between X states visually
- [ ] Test click detection works

---

### Task 6C.3: Persistence Implementation

**Files to Modify**:
- `src/FloppyTurd/States/MainMenuState.h`
- `src/FloppyTurd/States/MainMenuState.cpp`
- Consider: Shared preferences/settings manager (if exists)

**Implementation**:
```cpp
void MainMenuState::SaveVibrationPreference(bool enabled) {
    // Save to persistent storage
    // Option 1: Use existing settings system (if available)
    // Option 2: Use UserDefaults via platform delegates
    if (m_platformDelegates) {
        // Assuming platform delegates have a save preference method
        m_platformDelegates->SaveBoolPreference("vibration_enabled", enabled);
    }
}

bool MainMenuState::LoadVibrationPreference() {
    // Load from persistent storage
    if (m_platformDelegates) {
        return m_platformDelegates->LoadBoolPreference("vibration_enabled", true);  // Default true
    }
    return true;  // Default enabled
}

// In MainMenuState::Enter() or initialization:
void MainMenuState::Enter() {
    // ... existing code ...
    
    // Load vibration preference
    m_vibrationsEnabled = LoadVibrationPreference();
}
```

**Alternative**: If settings system doesn't exist, create simple iOS UserDefaults wrapper:

**New File**: `src/iOS/UserPreferences.swift`
```swift
import Foundation

@objc public class UserPreferences: NSObject {
    @objc public static let shared = UserPreferences()
    
    private let defaults = UserDefaults.standard
    
    @objc public func saveBool(_ value: Bool, forKey key: String) {
        defaults.set(value, forKey: key)
        defaults.synchronize()
    }
    
    @objc public func loadBool(forKey key: String, defaultValue: Bool) -> Bool {
        if defaults.object(forKey: key) != nil {
            return defaults.bool(forKey: key)
        }
        return defaultValue
    }
}
```

**Checklist**:
- [ ] Implement `SaveVibrationPreference()`
- [ ] Implement `LoadVibrationPreference()`
- [ ] Add save call to toggle handler
- [ ] Load preference in `Enter()` or initialization
- [ ] Test preference saves across app restarts
- [ ] Test default value (true/enabled)
- [ ] Consider adding to existing settings system if available

---

### Task 6C.4: Gate Haptic Commands

**Files to Modify**:
- Anywhere `TriggerHapticFeedback()` is called
- Or: Add check in `PlatformDelegates` implementation

**Option 1: Gate in MainMenuState (and other states)**
```cpp
// Before calling haptics:
if (m_vibrationsEnabled && m_platformDelegates) {
    m_platformDelegates->TriggerHapticFeedback(HapticType::Impact);
}
```

**Option 2: Gate in PlatformDelegates (cleaner, no refactoring)**
```cpp
// In PlatformDelegates or ThreadingProxy haptic command processing:
bool vibrationsEnabled = LoadVibrationPreference();  // Cache this
if (vibrationsEnabled) {
    // Actually trigger haptic
    HapticManager.shared.trigger(...);
}
```

**Recommended Approach**: Option 2 - Add check in Swift `HapticManager` or command processor

**New File**: `src/iOS/Haptics/HapticManager.swift` (modify existing if exists)
```swift
class HapticManager {
    static let shared = HapticManager()
    
    private var enabled: Bool {
        return UserPreferences.shared.loadBool(forKey: "vibration_enabled", defaultValue: true)
    }
    
    func trigger(_ type: HapticType) {
        guard enabled else { return }  // Early exit if disabled
        
        // ... existing haptic trigger code ...
    }
}
```

**Checklist**:
- [ ] Decide gating approach (Option 1 or 2)
- [ ] If Option 1: Add checks before all `TriggerHapticFeedback()` calls
- [ ] If Option 2: Add check in `HapticManager` or command processor
- [ ] Ensure haptics respect toggle state immediately
- [ ] Test haptics don't fire when toggle is OFF
- [ ] Test haptics resume when toggle is ON
- [ ] Verify no refactoring needed (simple gate)

---

## Phase 7: IAP Integration

**Goal**: Implement "Remove Ads" in-app purchase using StoreKit 2.

### Task 7.1: StoreKit Configuration

**Files to Create**:
- `FloppyTurd.storekit`

#### Step 1: Create StoreKit Configuration File
In Xcode:
1. File → New → File
2. Select "StoreKit Configuration File"
3. Name: `FloppyTurd.storekit`
4. Add to project root

#### Step 2: Define Product
In `FloppyTurd.storekit`:
```json
{
  "identifier": "com.floppyturd.game.removeads",
  "type": "consumable",
  "displayName": "Remove Ads",
  "description": "Removes all advertisements from the game",
  "price": 2.00,
  "familyShareable": false
}
```

**Product ID**: `com.floppyturd.game.removeads`  
**Price**: $2.00 USD (exactly, no $1.99 because 💩)  
**Type**: Non-consumable (user keeps forever)

**Checklist**:
- [ ] Create `FloppyTurd.storekit` file in Xcode
- [ ] Add product: `com.floppyturd.game.removeads`
- [ ] Set display name: "Remove Ads"
- [ ] Set description
- [ ] Set price: $2.00
- [ ] Set as non-consumable
- [ ] Add to Xcode project
- [ ] Configure for local testing

---

### Task 7.2: StoreManager Implementation

**Files to Create**:
- `src/iOS/Store/StoreManager.swift`

**Implementation**:
```swift
import StoreKit
import Foundation

@available(iOS 15.0, *)
@MainActor
public class StoreManager: ObservableObject {
    static let shared = StoreManager()
    
    @Published private(set) var products: [Product] = []
    @Published private(set) var purchasedProductIDs: Set<String> = []
    
    private let productID = "com.floppyturd.game.removeads"
    private var transactionListener: Task<Void, Error>?
    
    private init() {
        // Start transaction listener
        transactionListener = listenForTransactions()
        
        Task {
            await loadProducts()
            await updatePurchasedProducts()
        }
    }
    
    deinit {
        transactionListener?.cancel()
    }
    
    // MARK: - Product Loading
    
    func loadProducts() async {
        do {
            products = try await Product.products(for: [productID])
            print("StoreManager: Loaded \(products.count) products")
        } catch {
            print("StoreManager: Failed to load products: \(error)")
        }
    }
    
    // MARK: - Purchase Flow
    
    func purchase(_ product: Product) async throws -> Transaction? {
        let result = try await product.purchase()
        
        switch result {
        case .success(let verification):
            let transaction = try checkVerified(verification)
            
            // Deliver content to user
            await updatePurchasedProducts()
            
            // Always finish the transaction
            await transaction.finish()
            
            return transaction
            
        case .userCancelled, .pending:
            return nil
            
        @unknown default:
            return nil
        }
    }
    
    // MARK: - Restore Purchases
    
    func restorePurchases() async {
        do {
            try await AppStore.sync()
            await updatePurchasedProducts()
        } catch {
            print("StoreManager: Failed to restore purchases: \(error)")
        }
    }
    
    // MARK: - Transaction Handling
    
    func listenForTransactions() -> Task<Void, Error> {
        return Task.detached {
            for await result in Transaction.updates {
                do {
                    let transaction = try self.checkVerified(result)
                    await self.updatePurchasedProducts()
                    await transaction.finish()
                } catch {
                    print("StoreManager: Transaction verification failed: \(error)")
                }
            }
        }
    }
    
    func updatePurchasedProducts() async {
        var purchased: Set<String> = []
        
        for await result in Transaction.currentEntitlements {
            do {
                let transaction = try checkVerified(result)
                
                if transaction.revocationDate == nil {
                    purchased.insert(transaction.productID)
                }
            } catch {
                print("StoreManager: Failed to verify transaction: \(error)")
            }
        }
        
        purchasedProductIDs = purchased
        
        // Update ad removal state
        let adsRemoved = purchasedProductIDs.contains(productID)
        AdManager.shared.setAdsEnabled(!adsRemoved)
        
        // Save locally
        UserDefaults.standard.set(adsRemoved, forKey: "ads_removed")
        UserDefaults.standard.synchronize()
    }
    
    // MARK: - Verification
    
    func checkVerified<T>(_ result: VerificationResult<T>) throws -> T {
        switch result {
        case .unverified:
            throw StoreError.failedVerification
        case .verified(let safe):
            return safe
        }
    }
    
    // MARK: - State Queries
    
    func hasRemoveAds() -> Bool {
        return purchasedProductIDs.contains(productID) ||
               UserDefaults.standard.bool(forKey: "ads_removed")
    }
}

enum StoreError: Error {
    case failedVerification
}

// MARK: - Objective-C Bridge (for C++ interop if needed)

@objc public class StoreManagerBridge: NSObject {
    @objc public static let shared = StoreManagerBridge()
    
    @objc public func purchaseRemoveAds(completion: @escaping (Bool, String?) -> Void) {
        if #available(iOS 15.0, *) {
            Task {
                do {
                    guard let product = await StoreManager.shared.products.first else {
                        completion(false, "Product not found")
                        return
                    }
                    
                    let transaction = try await StoreManager.shared.purchase(product)
                    completion(transaction != nil, nil)
                } catch {
                    completion(false, error.localizedDescription)
                }
            }
        } else {
            completion(false, "iOS 15+ required")
        }
    }
    
    @objc public func restorePurchases(completion: @escaping () -> Void) {
        if #available(iOS 15.0, *) {
            Task {
                await StoreManager.shared.restorePurchases()
                completion()
            }
        } else {
            completion()
        }
    }
    
    @objc public func hasRemoveAds() -> Bool {
        if #available(iOS 15.0, *) {
            return StoreManager.shared.hasRemoveAds()
        }
        return false
    }
}
```

**Checklist**:
- [ ] Create `StoreManager.swift`
- [ ] Implement product loading
- [ ] Implement purchase flow
- [ ] Implement restore purchases
- [ ] Implement transaction listener
- [ ] Add verification logic
- [ ] Update `AdManager.setAdsEnabled()` on purchase
- [ ] Save "ads removed" state locally
- [ ] Create Objective-C bridge if needed for C++ interop
- [ ] Test product loads
- [ ] Test purchase flow
- [ ] Test restore flow

---

### Task 7.3: Wire IAP to Ad Controls Menu

**Files to Modify**:
- `src/FloppyTurd/States/MainMenuState.cpp`

**Implementation**:
```cpp
void MainMenuState::OnRemoveAdsPurchasePressed() {
    // Call StoreManager to purchase
    std::cout << "Initiating Remove Ads purchase...\n";
    
    // If using Objective-C bridge:
    // [[StoreManagerBridge shared] purchaseRemoveAdsWithCompletion:^(BOOL success, NSString* error) {
    //     if (success) {
    //         // Show success message
    //         std::cout << "Purchase successful! Ads removed.\n";
    //         // Update UI or show confirmation
    //     } else {
    //         std::cout << "Purchase failed: " << error << "\n";
    //         // Show error message
    //     }
    // }];
    
    // Or via platform delegates if wired up
    if (m_platformDelegates) {
        m_platformDelegates->InitiateIAPPurchase("com.floppyturd.game.removeads");
    }
}
```

**Checklist**:
- [ ] Update `OnRemoveAdsPurchasePressed()` to call StoreManager
- [ ] Add purchase success UI feedback
- [ ] Add purchase error UI feedback
- [ ] Add loading state during purchase
- [ ] Test purchase flow from menu
- [ ] Test cancellation handling
- [ ] Test error handling

---

### Task 7.4: Ads State Management

**Files to Modify**:
- `src/iOS/Advertising/AdManager.swift`

**Update AdManager** (should already have this from Phase 5):
```swift
private var adsEnabled: Bool = true

func setAdsEnabled(_ enabled: Bool) {
    adsEnabled = enabled
    
    if !enabled {
        print("AdManager: Ads disabled (user purchased removal)")
        // Clear any cached ads
        interstitialAd = nil
    } else {
        print("AdManager: Ads enabled")
        // Preload ads
        preloadInterstitial()
    }
}

// Update showInterstitial to check:
func showInterstitial(from viewController: UIViewController) {
    guard adsEnabled else {
        print("AdManager: Ads disabled, not showing interstitial")
        return
    }
    
    // ... existing show logic ...
}
```

**Checklist**:
- [ ] Verify `AdManager.setAdsEnabled()` exists
- [ ] Ensure ads don't show when disabled
- [ ] Test state persists across app restarts
- [ ] Call `setAdsEnabled(false)` on purchase
- [ ] Call `setAdsEnabled(true)` if purchase revoked
- [ ] Clear cached ads when disabled

---

### Task 7.5: App Store Connect Configuration

**Steps** (outside codebase):
1. Log in to App Store Connect
2. Navigate to FloppyTurd app
3. Go to "In-App Purchases"
4. Add new IAP:
   - Product ID: `com.floppyturd.game.removeads`
   - Type: Non-consumable
   - Reference Name: "Remove Ads"
   - Price: $2.00 USD
   - Localized descriptions (at least English)
5. Submit for review (with app or separately)

**Checklist**:
- [ ] Create IAP in App Store Connect
- [ ] Set product ID: `com.floppyturd.game.removeads`
- [ ] Set as non-consumable
- [ ] Set price tier for $2.00
- [ ] Add localized descriptions
- [ ] Submit for review
- [ ] Wait for approval
- [ ] Test with TestFlight

---

## Testing Checklist

### Phase 6 Testing - UI Enhancements

#### Ad Controls Button
- [ ] Button appears on main menu (bottom left)
- [ ] Button scales correctly (6x)
- [ ] Button loads texture properly
- [ ] Button click opens Ad Controls menu
- [ ] Button visible on all screen sizes (iPhone/iPad)
- [ ] Button doesn't overlap with other UI elements

#### Version Number
- [ ] Version text appears on main menu (bottom right)
- [ ] Displays correct version: "1.0.0"
- [ ] Text is readable (appropriate size)
- [ ] Text is white color
- [ ] Text visible on all screen sizes
- [ ] Text doesn't overlap with ad controls button

#### Ad Controls Menu
- [ ] Menu opens when ad controls button clicked
- [ ] Title "AD CONTROLS" displays correctly
- [ ] "Remove Ads" label displays correctly
- [ ] "$2.00" button displays correctly
- [ ] Back button displays correctly
- [ ] Menu layout resembles leaderboard menu
- [ ] Back button returns to main menu
- [ ] Price button shows placeholder message (Phase 6)
- [ ] Menu hides when back is pressed
- [ ] No visual glitches during transitions

#### Vibration Toggle
- [ ] Toggle appears in options menu
- [ ] "VIBRATION" label displays correctly
- [ ] X button sprites load correctly
- [ ] Toggle shows correct initial state
- [ ] Click toggles between selected/unselected sprites
- [ ] Toggle state persists across app restarts
- [ ] Haptics disabled when toggle is OFF
- [ ] Haptics resume when toggle is ON
- [ ] Toggle positioned below difficulty section
- [ ] Toggle doesn't break existing options layout

### Phase 7 Testing - IAP

#### StoreKit Configuration
- [ ] StoreKit config file loads in Xcode
- [ ] Product displays in StoreKit Testing
- [ ] Price shows as $2.00
- [ ] Product description correct

#### Purchase Flow
- [ ] Tapping "$2.00" button initiates purchase
- [ ] StoreKit sheet appears with product details
- [ ] Price shows as $2.00 (not $1.99)
- [ ] Purchase success removes ads immediately
- [ ] Purchase cancellation handled gracefully
- [ ] Purchase error shown to user
- [ ] Loading state shows during purchase
- [ ] Confirmation shown on success

#### Ad Removal
- [ ] Ads stop showing after purchase
- [ ] `AdManager.setAdsEnabled(false)` called
- [ ] Ad preloading stops
- [ ] State persists across app restarts
- [ ] Game still loads/plays normally without ads

#### Restore Purchases
- [ ] Delete and reinstall app
- [ ] Restore purchases option available
- [ ] Restoring works correctly
- [ ] Ads remain disabled after restore
- [ ] No duplicate charges

#### App Store Testing
- [ ] IAP appears in TestFlight
- [ ] Sandbox accounts can purchase
- [ ] Real purchases work in production
- [ ] Receipt validation works
- [ ] No crashes related to IAP
- [ ] IAP shows in App Store Connect

---

## Design Specifications

### Main Menu Layout

```
┌────────────────────────────────────────────┐
│                                            │
│            [FLOPPY TURD LOGO]              │
│                                            │
│                   [F]                      │
│                                            │
│               [Play Button]                │
│            [Options Button]                │
│          [Quick Play Button]               │
│         [Leaderboard Button]               │
│                                            │
│                                            │
│  [Ad Controls]              [v1.0.0]       │ ← NEW
└────────────────────────────────────────────┘
  ↑ Bottom left (6x)          ↑ Bottom right
```

### Ad Controls Menu Layout

```
┌────────────────────────────────────────────┐
│                                            │
│              AD CONTROLS                   │
│                                            │
│                                            │
│              Remove Ads                    │
│                                            │
│                                            │
│                                            │
│                                            │
│                                            │
│  [Back]                      [$2.00]       │
└────────────────────────────────────────────┘
  ↑ Bottom left              ↑ Bottom right
```

### Options Menu (Updated)

```
┌────────────────────────────────────────────┐
│                                            │
│                 OPTIONS                    │
│                                            │
│  MASTER  [━━━━━━━━●━]                     │
│  MUSIC   [━━━━━●━━━━]                     │
│  SFX     [━━━━━━━━━●]                     │
│                                            │
│             DIFFICULTY                     │
│          [←] Regular [→]                   │
│                                            │
│  VIBRATION                    [X]          │ ← NEW
│                                            │
│  [Back]                                    │
└────────────────────────────────────────────┘
  ↑ X button toggles between selected/unselected
```

### Asset Specifications

#### adcontrolsbutton.png
- **Location**: `src/assets/graphics/ui/hud/adcontrolsbutton.png`
- **Scale**: 6x
- **Position**: Bottom left corner of main menu
- **Expected Size**: ~64x64px base (384x384px at 6x)

#### xbuttonselected.png
- **Location**: `src/assets/graphics/ui/hud/xbuttonselected.png`
- **Usage**: Vibration toggle ON state (X mark visible)
- **Scale**: 6x or match other option controls

#### xbuttonunselected.png
- **Location**: `src/assets/graphics/ui/hud/xbuttonunselected.png`
- **Usage**: Vibration toggle OFF state (empty/no X mark)
- **Scale**: 6x or match other option controls

### Color Specifications

- **Text Color**: White (`RGB(255, 255, 255)`)
- **Button Text**: White
- **Title Text**: White
- **Version Text**: White

### Font Sizes (Mobile)

- **Title (OPTIONS, AD CONTROLS)**: 72pt
- **Subtitle (Remove Ads)**: 64pt
- **Labels (VIBRATION, DIFFICULTY)**: 54pt
- **Button Text**: 54pt
- **Version Number**: 48pt

### Font Sizes (Desktop)

- **Title**: 42pt
- **Subtitle**: 38pt
- **Labels**: 32pt
- **Button Text**: 32pt
- **Version Number**: 28pt

---

## File Reference

### Files to Create
1. `FloppyTurd.storekit` - StoreKit configuration
2. `src/iOS/Store/StoreManager.swift` - IAP logic
3. `src/iOS/UserPreferences.swift` - Settings persistence (if needed)

### Files to Modify
1. `src/FloppyTurd/States/MainMenuState.h` - Add entities, menu modes, methods
2. `src/FloppyTurd/States/MainMenuState.cpp` - Implement UI and IAP logic
3. `src/iOS/Advertising/AdManager.swift` - Verify `setAdsEnabled()` integration
4. `src/Engine/Platform/PlatformDelegates.h` - Add IAP delegates (if needed)

### Assets Required ✅
1. `src/assets/graphics/ui/hud/adcontrolsbutton.png`
2. `src/assets/graphics/ui/hud/xbuttonselected.png`
3. `src/assets/graphics/ui/hud/xbuttonunselected.png`

---

## Implementation Order

### Day 1: Phase 6A & 6B (UI Foundation)
1. Add ad controls button to main menu (6A.1)
2. Add version number to main menu (6A.2)
3. Create Ad Controls menu structure (6B.1-6B.7)
4. Test menu navigation and layout

### Day 2: Phase 6C (Vibration Toggle)
1. Add vibration toggle to options menu (6C.1)
2. Implement toggle click handling (6C.2)
3. Add persistence (6C.3)
4. Gate haptic commands (6C.4)
5. Test vibration toggle functionality

### Day 3: Phase 7 (IAP Integration)
1. Create StoreKit configuration (7.1)
2. Implement StoreManager (7.2)
3. Wire IAP to Ad Controls menu (7.3)
4. Update AdManager integration (7.4)
5. Test purchase flow locally

### Day 4: Testing & App Store Connect
1. Configure IAP in App Store Connect (7.5)
2. Execute full testing checklist
3. TestFlight testing with sandbox accounts
4. Fix any issues found

---

## Notes & Considerations

### Design Notes
- **$2.00 Price**: Exactly $2.00 because "number 2 is for poop" 💩 (non-negotiable)
- **No Major Refactoring**: Vibration toggle should be simple gate, no architectural changes
- **Consistent Style**: Ad Controls menu should match leaderboard/options menu style
- **Simple Navigation**: Back button behavior consistent with other menus

### Technical Notes
- **StoreKit 2**: Requires iOS 15+, but offers modern async/await API
- **Receipt Validation**: Consider server-side validation in future for security
- **Restore Purchases**: Required by Apple for non-consumables
- **Persistence**: IAP state must persist locally AND sync with Apple
- **Ad State**: When ads disabled, ensure preloading stops to save bandwidth/battery

### Testing Notes
- **Sandbox Testing**: Use sandbox accounts for IAP testing before production
- **TestFlight**: Final IAP testing must be done via TestFlight with sandbox accounts
- **Edge Cases**: Test purchase cancellation, errors, network issues
- **State Synchronization**: Ensure ads state syncs properly across app restarts

### Future Enhancements (Post-Phase 7)
- [ ] Add "Restore Purchases" button to Ad Controls menu
- [ ] Add analytics for IAP funnel (views, taps, purchases)
- [ ] Consider promotional pricing or launch discount
- [ ] Add confirmation dialog before purchase
- [ ] Show "Thank you" message after purchase
- [ ] Add badge/indicator showing "Ad-Free" status

---

## Success Criteria

### Phase 6 Complete When:
- ✅ Ad controls button visible and functional on main menu
- ✅ Version number displayed correctly
- ✅ Ad Controls menu opens and navigates properly
- ✅ Vibration toggle works and persists
- ✅ Haptics respect toggle state
- ✅ All new UI scales properly on different devices
- ✅ No visual glitches or layout issues

### Phase 7 Complete When:
- ✅ StoreKit configuration created and tested
- ✅ Purchase flow works end-to-end
- ✅ Ads stop showing after purchase
- ✅ Purchase state persists across app restarts
- ✅ Restore purchases works correctly
- ✅ IAP configured in App Store Connect
- ✅ TestFlight testing passes
- ✅ No crashes or major bugs

---

## Questions & Decisions

### Resolved Decisions
✅ Ad controls button scale: 6x  
✅ IAP price: $2.00 exactly  
✅ Vibration toggle: Use X button sprites  
✅ Menu style: Match leaderboard menu  
✅ Version position: Bottom right  
✅ Haptic gating: Simple check, no refactoring  

### Open Questions
❓ Should we add "Restore Purchases" button now or post-launch?  
❓ Add confirmation dialog before purchase?  
❓ Show in-app message after successful purchase?  
❓ Include restore purchases option in Ad Controls menu?  
❓ Add analytics tracking for IAP funnel?

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Status**: Ready for Implementation  
**Next Step**: Begin Phase 6A - Main Menu Enhancements