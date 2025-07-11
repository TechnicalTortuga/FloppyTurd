# Audio Architecture Refactor Plan

## Current Problem
- PlatformIOS is trying to manage audio objects directly
- AudioStateManager is using AudioClip which uses PlatformAPI
- We need proper separation: Game Code → PlatformAPI → PlatformIOS → AudioStateManager

## Target Architecture

### Layer 1: Game Code
- Calls PlatformAPI functions for audio operations
- Doesn't know about platform-specific details

### Layer 2: PlatformAPI (Unified Interface)
- Provides platform-agnostic audio function names
- Delegates to platform-specific implementations (PlatformIOS)

### Layer 3: PlatformIOS (Platform-Specific Implementation)
- Provides low-level platform-specific audio functions
- Uses AVAudioPlayer for iOS/Metal
- Delegates audio state management to AudioStateManager

### Layer 4: AudioStateManager (Audio State Management)
- Manages audio state, volume, current music, transitions
- Stores and manages AudioClip objects
- Handles audio caching and lifecycle

### Layer 5: AudioClip (Audio Object Wrapper)
- Stores platform-specific audio handles (void*)
- Provides platform-agnostic interface for individual audio objects
- Managed by AudioStateManager

## Implementation Plan

### Phase 1: AudioClip Refactor ✅ COMPLETED
- [x] AudioClip now uses PlatformAPI instead of direct Raylib calls
- [x] AudioClip stores platform-specific handles (void*)
- [x] AudioClip provides platform-agnostic interface

### Phase 2: PlatformAPI Audio Functions ✅ COMPLETED
- [x] PlatformAPI provides unified audio function names
- [x] PlatformAPI delegates to platform-specific implementations
- [x] PlatformAPI includes all necessary audio functions

### Phase 3: PlatformIOS Low-Level Implementation ✅ COMPLETED
- [x] PlatformIOS implements low-level audio functions
- [x] PlatformIOS uses AVAudioPlayer for iOS
- [x] PlatformIOS handles asset catalog and file loading

### Phase 4: AudioStateManager Integration (IN PROGRESS)
- [ ] AudioStateManager should manage AudioClip objects directly
- [ ] AudioStateManager should call PlatformAPI functions
- [ ] AudioStateManager should handle audio caching
- [ ] AudioStateManager should manage audio state transitions

### Phase 5: Game Code Integration
- [ ] Game code should call PlatformAPI functions
- [ ] Game code should not know about platform-specific details
- [ ] Game code should use AudioStateManager for state management

## Function Call Flow

### For Music Loading:
1. **Game Code**: `PlatformAPI::LoadMusic("music/track.mp3")`
2. **PlatformAPI**: Delegates to `PlatformIOS::LoadMusic()`
3. **PlatformIOS**: Creates AVAudioPlayer, returns handle
4. **AudioStateManager**: Stores AudioClip with the handle
5. **AudioClip**: Stores the platform-specific handle

### For Music Playback:
1. **Game Code**: `PlatformAPI::PlayMusic(musicHandle)`
2. **PlatformAPI**: Delegates to `PlatformIOS::PlayMusic()`
3. **PlatformIOS**: Calls AVAudioPlayer play method
4. **AudioStateManager**: Manages playback state

## Current Status

### ✅ COMPLETED
- PlatformAPI provides unified interface
- PlatformIOS implements low-level functions
- AudioClip uses PlatformAPI
- Basic audio loading and playback working

### 🔄 IN PROGRESS
- AudioStateManager integration with PlatformAPI
- AudioStateManager managing AudioClip objects
- Proper audio state management

### ❌ NOT STARTED
- Game code integration
- Audio caching in AudioStateManager
- Audio state transitions
- Sound effect management

## Next Steps

1. **Update AudioStateManager** to use PlatformAPI instead of AudioClip directly
2. **Modify AudioStateManager** to manage AudioClip objects that store platform handles
3. **Update AudioStateManager** to call PlatformAPI functions for low-level operations
4. **Test the complete flow** from Game Code → PlatformAPI → PlatformIOS → AudioStateManager
5. **Add audio caching** to AudioStateManager
6. **Implement proper audio state transitions**

## Files to Modify

### Primary Files:
- `AudioStateManager.h` - Update to use PlatformAPI and manage AudioClip objects
- `AudioStateManager.cpp` - Implement PlatformAPI integration
- `AudioClip.h` - Ensure proper platform handle storage
- `AudioClip.cpp` - Ensure proper PlatformAPI usage

### Secondary Files:
- `PlatformAPI.h` - Verify all audio functions are present
- `PlatformAPI.cpp` - Verify proper delegation to PlatformIOS
- `PlatformIOS.h` - Verify all low-level functions are present
- `PlatformIOS.cpp` - Verify proper AVAudioPlayer implementation

## Success Criteria

1. **Platform Agnostic**: Game code doesn't know about iOS/Metal specifics
2. **Proper Layering**: Each layer has clear responsibilities
3. **Audio State Management**: AudioStateManager properly manages audio state
4. **Performance**: No unnecessary function call overhead
5. **Maintainability**: Clear separation of concerns
6. **Extensibility**: Easy to add new platforms or audio features 