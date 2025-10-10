# URGENT CRASH FIX - Scene Transition

## The Problem

**Crash Location:** `GameplayState::RescaleBackgroundsForOrientation()`

**Root Cause:** GameplayState registers a callback with ConfigManager:
```cpp
ConfigManager::Instance().SetScreenInfoUpdateCallback([this]() {
    // This lambda captures 'this' pointer
    UpdateUILayoutForOrientation();
    RescaleBackgroundsForOrientation(...);
});
```

**Issue:** When GameplayState is destroyed (scene transition), the callback is NOT unregistered. When ConfigManager fires the callback later, it calls a **deleted object** → SEGFAULT

## Stack Trace Analysis

```
Thread 0 Crashed:
1. GameplayState::RescaleBackgroundsForOrientation() + 1192  ← ACCESSING DELETED OBJECT
2. GameplayState::RepositionUIElementsLandscape() + 337
3. GameplayState::UpdateUILayoutForOrientation() + 1235
4. GameplayState::RegisterScreenInfoCallback()::$_0::operator()() + 1370  ← LAMBDA FIRES
5. ConfigManager::SetScreenInfoDirect() + 962  ← CALLBACK INVOKED
6. FloppyTurdGame::UpdateScreenInfo() + 41
7. GameViewController.unlockOrientation() + 239
```

**Timeline:**
1. GameplayState registers callback with ConfigManager
2. User transitions from Desert → MainMenu
3. GameplayState is destroyed
4. Later: Orientation changes
5. ConfigManager fires callback
6. Callback tries to access **deleted** GameplayState object
7. **CRASH**

## The Fix

**Option 1: Unregister callback in destructor** (RECOMMENDED)
```cpp
// In GameplayState.h
~GameplayState() {
    // Clear the callback before destruction
    ConfigManager::Instance().SetScreenInfoUpdateCallback(nullptr);
}
```

**Option 2: Use weak_ptr pattern** (MORE COMPLEX)
Would require refactoring to shared_ptr/weak_ptr for state management

## Files to Modify

1. `src/FloppyTurd/States/GameplayState.h` - Add destructor
2. `src/FloppyTurd/States/GameplayState.cpp` - Implement destructor with callback cleanup

## Priority

**CRITICAL** - Blocking all scene transitions

