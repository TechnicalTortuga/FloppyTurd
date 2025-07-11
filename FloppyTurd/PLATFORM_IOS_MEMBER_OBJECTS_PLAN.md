# PlatformIOS Member Objects Plan

## Current Problem
- PlatformIOS functions create new AVAudioPlayer objects dynamically
- This causes memory leaks, object overlap, and performance issues
- We need member objects that are reused properly
- Missing crossfading, preloading, and advanced audio transitions

## Target Architecture

### PlatformIOS Member Objects
```cpp
class PlatformIOS {
private:
    // Audio member objects (reused, not created dynamically)
    AVAudioPlayer* m_currentMusicPlayer = nullptr;
    AVAudioPlayer* m_nextMusicPlayer = nullptr;
    AVAudioPlayer* m_fadeOutPlayer = nullptr;  // For fade transitions
    
    // Audio state management
    bool m_isMusicPlaying = false;
    bool m_isMusicPaused = false;
    float m_musicVolume = 1.0f;
    bool m_musicLooping = false;
    
    // Transition state
    bool m_isTransitioning = false;
    float m_fadeProgress = 0.0f;
    float m_fadeDuration = 0.0f;
    bool m_isFadingOut = false;
    bool m_isFadingIn = false;
    
    // Sound management
    std::vector<AVAudioPlayer*> m_activeSounds;
    std::unordered_map<std::string, AVAudioPlayer*> m_loadedSounds;
    std::unordered_map<std::string, AVAudioPlayer*> m_soundCache;
    std::vector<std::string> m_soundCacheOrder;  // LRU order
    
    // Audio session management
    bool m_audioSessionActive = false;
    
    // Cache management
    static const size_t MAX_SOUND_CACHE_SIZE = 20;
    size_t m_soundCacheHits = 0;
    size_t m_soundCacheMisses = 0;
    size_t m_soundCacheEvictions = 0;
};
```

## Implementation Plan

### Phase 1: Add Member Objects to PlatformIOS ✅ COMPLETED
- [x] Add AVAudioPlayer member objects for music
- [x] Add sound cache and active sounds containers
- [x] Add audio state member variables
- [x] Add audio session management

### Phase 2: Update Audio Functions to Use Member Objects ✅ COMPLETED
- [x] `LoadMusic()` - Store in m_currentMusicPlayer or m_nextMusicPlayer
- [x] `PlayMusic()` - Use existing m_currentMusicPlayer
- [x] `StopMusic()` - Stop and reset m_currentMusicPlayer
- [x] `PauseMusic()` - Pause m_currentMusicPlayer
- [x] `ResumeMusic()` - Resume m_currentMusicPlayer
- [x] `SetMusicVolume()` - Update m_musicVolume and apply to player
- [x] `SetMusicLooping()` - Update m_musicLooping and apply to player

### Phase 3: Add Advanced Audio Features 🔄 IN PROGRESS
- [ ] **Preload functionality**
  - [ ] `PreloadNextTrack()` - Load into m_nextMusicPlayer
  - [ ] `SwitchToNextTrack()` - Swap current and next players
  - [ ] `ClearNextTrack()` - Clean up next track
- [ ] **Crossfading functionality**
  - [ ] `StartCrossfade()` - Begin crossfade between tracks
  - [ ] `UpdateCrossfade()` - Update fade progress
  - [ ] `CompleteCrossfade()` - Finish crossfade
- [ ] **Fade transitions**
  - [ ] `FadeOutMusic()` - Fade out current track
  - [ ] `FadeInMusic()` - Fade in new track
  - [ ] `UpdateFade()` - Update fade progress

### Phase 4: Add Sound Management ✅ COMPLETED
- [x] `LoadSound()` - Store in m_soundCache
- [x] `PlaySound()` - Create temporary player from cache
- [x] `UnloadSound()` - Remove from cache
- [x] `SetSoundVolume()` - Apply to cached sound
- [ ] Sound pooling for multiple simultaneous sounds

### Phase 5: Add Audio Caching ✅ COMPLETED
- [x] Cache loaded audio files by filename
- [x] LRU cache eviction for memory management
- [x] Cache statistics and monitoring
- [ ] Preload frequently used audio

### Phase 6: Audio Session Management
- [ ] Initialize audio session once
- [ ] Handle audio session interruptions
- [ ] Manage audio session lifecycle
- [ ] Handle background/foreground transitions

## Function Implementation Examples

### Preload Next Track
```cpp
void* PlatformIOS::PreloadNextTrack(const char* fileName) {
    // Clean up existing next track if needed
    if (m_nextMusicPlayer) {
        CleanupAVAudioPlayer(m_nextMusicPlayer);
        m_nextMusicPlayer = nullptr;
    }
    
    // Load new track into next player
    m_nextMusicPlayer = CreateAVAudioPlayer(fileName);
    if (m_nextMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_nextMusicPlayer;
        player.volume = 0.0f;  // Start silent for crossfade
        player.numberOfLoops = m_musicLooping ? -1 : 0;
        return m_nextMusicPlayer;
    }
    return nullptr;
}
```

### Crossfade Between Tracks
```cpp
void PlatformIOS::StartCrossfade(float duration) {
    if (!m_currentMusicPlayer || !m_nextMusicPlayer) {
        return;
    }
    
    m_isTransitioning = true;
    m_fadeProgress = 0.0f;
    m_fadeDuration = duration;
    m_isFadingOut = true;
    m_isFadingIn = true;
    
    // Start next track at volume 0
    AVAudioPlayer* nextPlayer = (__bridge AVAudioPlayer*)m_nextMusicPlayer;
    [nextPlayer play];
}
```

### Update Crossfade
```cpp
void PlatformIOS::UpdateCrossfade(float deltaTime) {
    if (!m_isTransitioning) return;
    
    m_fadeProgress += deltaTime / m_fadeDuration;
    if (m_fadeProgress >= 1.0f) {
        CompleteCrossfade();
        return;
    }
    
    // Fade out current track
    if (m_currentMusicPlayer && m_isFadingOut) {
        AVAudioPlayer* currentPlayer = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        currentPlayer.volume = m_musicVolume * (1.0f - m_fadeProgress);
    }
    
    // Fade in next track
    if (m_nextMusicPlayer && m_isFadingIn) {
        AVAudioPlayer* nextPlayer = (__bridge AVAudioPlayer*)m_nextMusicPlayer;
        nextPlayer.volume = m_musicVolume * m_fadeProgress;
    }
}
```

### Complete Crossfade
```cpp
void PlatformIOS::CompleteCrossfade() {
    // Stop and clean up current track
    if (m_currentMusicPlayer) {
        CleanupAVAudioPlayer(m_currentMusicPlayer);
        m_currentMusicPlayer = nullptr;
    }
    
    // Move next track to current
    m_currentMusicPlayer = m_nextMusicPlayer;
    m_nextMusicPlayer = nullptr;
    
    // Reset transition state
    m_isTransitioning = false;
    m_fadeProgress = 0.0f;
    m_fadeDuration = 0.0f;
    m_isFadingOut = false;
    m_isFadingIn = false;
    
    // Set final volume
    if (m_currentMusicPlayer) {
        AVAudioPlayer* player = (__bridge AVAudioPlayer*)m_currentMusicPlayer;
        player.volume = m_musicVolume;
    }
}
```

## Benefits of Member Objects

1. **Memory Management**: No dynamic allocation/deallocation
2. **Performance**: Reuse objects instead of creating new ones
3. **State Consistency**: Single source of truth for audio state
4. **Resource Management**: Proper cleanup and lifecycle management
5. **Thread Safety**: Easier to manage with member objects
6. **Debugging**: Clear ownership and state tracking
7. **Crossfading**: Multiple players for smooth transitions
8. **Preloading**: No interruption when switching tracks

## Integration with AudioStateManager

AudioStateManager will:
- Call PlatformIOS functions that use member objects
- Not need to manage platform-specific objects directly
- Focus on audio state and transitions
- Use PlatformAPI interface for all operations
- Handle crossfade timing and coordination

## Next Steps

1. **Update PlatformIOS.h** to add transition state member objects
2. **Update PlatformIOS.cpp** to implement preload and crossfade functions
3. **Add fade transition functions** for smooth audio changes
4. **Test the complete flow** with member objects
5. **Add audio caching** and preloading 