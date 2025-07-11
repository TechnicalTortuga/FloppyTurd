# AudioStateManager Platform-Agnostic Refactor Plan

## Current Problem
The AudioStateManager and AudioClip classes are tightly coupled to platform-specific Raylib audio functions:
- `AudioClip` directly calls `LoadMusic()`, `PlayMusic()`, `StopMusic()` etc.
- `AudioStateManager` uses `AudioClip` which depends on Raylib audio
- This makes the audio system non-portable and platform-specific

## Solution: Platform-Agnostic Audio System

### Phase 1: Create Platform-Agnostic AudioClip
Replace the current AudioClip with a platform-agnostic version that uses PlatformAPI.

#### New AudioClip.h
```cpp
// AudioClip.h - Platform-agnostic version
#pragma once
#include <string>
#include "PlatformAPI.h"

class AudioClip {
public:
    AudioClip(const std::string& filePath);
    AudioClip(void* platformMusicHandle); // For pre-loaded music
    virtual ~AudioClip();

    void Play();
    void PlayLoop();
    void Stop();
    void Pause();
    void Resume();
    void Update();
    bool IsPlaying() const;
    void SetVolume(float vol);
    void SetLooping(bool looping);

private:
    void* m_platformMusicHandle; // Platform-specific music handle
    bool m_isLooping;
    float m_volume;
    bool m_isPlaying;
};
```

#### New AudioClip.cpp
```cpp
// AudioClip.cpp - Platform-agnostic implementation
#include "AudioClip.h"
#include "PlatformAPI.h"

AudioClip::AudioClip(const std::string& filePath) 
    : m_platformMusicHandle(nullptr)
    , m_isLooping(false)
    , m_volume(1.0f)
    , m_isPlaying(false) {
    
    // Use PlatformAPI to load music
    m_platformMusicHandle = PlatformAPI::LoadMusic(filePath.c_str());
    if (m_platformMusicHandle) {
        PlatformAPI::SetMusicVolume(m_platformMusicHandle, m_volume);
    }
}

AudioClip::AudioClip(void* platformMusicHandle)
    : m_platformMusicHandle(platformMusicHandle)
    , m_isLooping(false)
    , m_volume(1.0f)
    , m_isPlaying(false) {
    
    if (m_platformMusicHandle) {
        PlatformAPI::SetMusicVolume(m_platformMusicHandle, m_volume);
    }
}

AudioClip::~AudioClip() {
    if (m_platformMusicHandle) {
        PlatformAPI::UnloadMusic(m_platformMusicHandle);
    }
}

void AudioClip::Play() {
    if (m_platformMusicHandle) {
        PlatformAPI::PlayMusic(m_platformMusicHandle);
        m_isPlaying = true;
    }
}

void AudioClip::PlayLoop() {
    if (m_platformMusicHandle) {
        PlatformAPI::PlayMusic(m_platformMusicHandle);
        m_isPlaying = true;
        m_isLooping = true;
        // Note: Looping is handled by the platform implementation
    }
}

void AudioClip::Stop() {
    if (m_platformMusicHandle && m_isPlaying) {
        PlatformAPI::StopMusic(m_platformMusicHandle);
        m_isPlaying = false;
    }
}

void AudioClip::Pause() {
    if (m_platformMusicHandle && m_isPlaying) {
        // PlatformAPI doesn't have PauseMusic yet - need to add
        // PlatformAPI::PauseMusic(m_platformMusicHandle);
        m_isPlaying = false;
    }
}

void AudioClip::Resume() {
    if (m_platformMusicHandle && !m_isPlaying) {
        // PlatformAPI doesn't have ResumeMusic yet - need to add
        // PlatformAPI::ResumeMusic(m_platformMusicHandle);
        m_isPlaying = true;
    }
}

void AudioClip::Update() {
    if (m_platformMusicHandle) {
        PlatformAPI::UpdateMusic(m_platformMusicHandle);
    }
}

bool AudioClip::IsPlaying() const {
    if (m_platformMusicHandle) {
        return PlatformAPI::IsMusicPlaying(m_platformMusicHandle);
    }
    return false;
}

void AudioClip::SetVolume(float vol) {
    m_volume = vol;
    if (m_platformMusicHandle) {
        PlatformAPI::SetMusicVolume(m_platformMusicHandle, vol);
    }
}

void AudioClip::SetLooping(bool looping) {
    m_isLooping = looping;
    // Note: Looping is handled by the platform implementation
}
```

### Phase 2: Update PlatformAPI to Support Missing Functions

#### Add to PlatformSpecific.h
```cpp
// Add these to the audio section:
virtual void PauseMusic(void* music) = 0;
virtual void ResumeMusic(void* music) = 0;
virtual void SetMusicLooping(void* music, bool looping) = 0;
```

#### Add to PlatformAPI.cpp
```cpp
void PlatformAPI::PauseMusic(void* music) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->PauseMusic(music);
    }
}

void PlatformAPI::ResumeMusic(void* music) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->ResumeMusic(music);
    }
}

void PlatformAPI::SetMusicLooping(void* music, bool looping) {
    if (GetPlatformImpl()) {
        GetPlatformImpl()->SetMusicLooping(music, looping);
    }
}
```

#### Add to PlatformIOS.cpp
```cpp
void PlatformIOS::PauseMusic(void* music) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] PauseMusic - null music pointer");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] PauseMusic - pausing music");
    try {
        Music musicStruct = { music, 0 };
        PauseMusic_iOS(musicStruct);
        TraceLog(LOG_INFO, "[PlatformIOS] Music paused successfully");
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Exception pausing music: %s", e.what());
    }
}

void PlatformIOS::ResumeMusic(void* music) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] ResumeMusic - null music pointer");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] ResumeMusic - resuming music");
    try {
        Music musicStruct = { music, 0 };
        ResumeMusic_iOS(musicStruct);
        TraceLog(LOG_INFO, "[PlatformIOS] Music resumed successfully");
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Exception resuming music: %s", e.what());
    }
}

void PlatformIOS::SetMusicLooping(void* music, bool looping) {
    if (!music) {
        TraceLog(LOG_WARNING, "[PlatformIOS] SetMusicLooping - null music pointer");
        return;
    }
    
    TraceLog(LOG_INFO, "[PlatformIOS] SetMusicLooping - setting looping to %s", looping ? "true" : "false");
    try {
        Music musicStruct = { music, 0 };
        SetLooping(musicStruct, looping);
        TraceLog(LOG_INFO, "[PlatformIOS] Music looping set successfully");
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlatformIOS] Exception setting music looping: %s", e.what());
    }
}
```

### Phase 3: Update AudioStateManager

#### Update AudioStateManager.h
```cpp
// Remove the AudioClip.h include and add:
#include "AudioClip.h" // New platform-agnostic version
#include "PlatformAPI.h"

// The rest of the class remains the same, but now uses platform-agnostic AudioClip
```

#### Update AudioStateManager.cpp
```cpp
// Replace all direct Raylib audio calls with PlatformAPI calls
// The LoadAndPlayTrack method becomes:

void AudioStateManager::LoadAndPlayTrack(const AudioTrackInfo& trackInfo) {
    TraceLog(LOG_INFO, "[AUDIO] Loading track: %s", trackInfo.resourcePath.c_str());
    
    // Handle empty track path (for loading state)
    if (trackInfo.resourcePath.empty()) {
        TraceLog(LOG_INFO, "[AUDIO] No music for this state (loading)");
        if (currentMusic) {
            currentMusic->Stop();
            currentMusic.reset();
        }
        return;
    }
    
    try {
        // Create new audio clip using platform-agnostic system
        currentMusic = std::make_unique<AudioClip>(trackInfo.resourcePath);
        
        if (currentMusic && currentMusic->IsPlaying()) {
            // Set volume
            currentMusic->SetVolume(trackInfo.volume * musicVolume);
            
            // Set looping
            if (trackInfo.shouldLoop) {
                currentMusic->SetLooping(true);
            }
            
            TraceLog(LOG_INFO, "[AUDIO] Successfully loaded and started playing: %s", trackInfo.resourcePath.c_str());
        } else {
            TraceLog(LOG_ERROR, "[AUDIO] Failed to create valid AudioClip for %s", trackInfo.resourcePath.c_str());
        }
        
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[AUDIO] Exception loading track %s: %s", trackInfo.resourcePath.c_str(), e.what());
    }
}
```

### Phase 4: Update AudioManager

#### Update AudioManager.h
```cpp
// Remove RaylibCompat.h include and add:
#include "PlatformAPI.h"

// The AudioManager class remains largely the same, but now uses platform-agnostic audio
```

#### Update AudioManager.cpp
```cpp
// Replace direct Raylib audio calls with PlatformAPI calls
// For example, in StopMusic():

void AudioManager::StopMusic() {
    for (auto* clip : activeClips) {
        if (clip && clip->IsPlaying()) {
            clip->Stop();
        }
    }
}
```

### Phase 5: Update ResourceManager

#### Update ResourceManager.cpp
```cpp
// In LoadMusicInternal():

bool ResourceManager::LoadMusicInternal(const std::string& id) {
    // ... existing path resolution code ...
    
    // Use PlatformAPI instead of direct Raylib call
    void* musicHandle = PlatformAPI::LoadMusic(fullPath.c_str());
    if (!musicHandle) {
        TraceLog(LOG_ERROR, "Failed to load music: %s", fullPath.c_str());
        return false;
    }

    CachedResource<void*> cached;
    cached.resource = musicHandle;
    cached.isValid = true;
    cached.path = fullPath;
    cached.memoryUsage = streamingEnabled ? 1024 : 1024 * 1024;
    cached.lastAccessed = GetTime();

    musicCache[id] = cached;
    totalMemoryUsage += cached.memoryUsage;
    return true;
}
```

## Benefits of This Refactor

1. **Platform Agnostic**: AudioStateManager no longer depends on platform-specific audio APIs
2. **Consistent Interface**: All audio operations go through PlatformAPI
3. **Easier Testing**: Can mock PlatformAPI for unit tests
4. **Better Separation**: Audio logic is separated from platform implementation
5. **Future Proof**: Easy to add new platforms without changing audio logic

## Implementation Order

1. ✅ **Phase 1**: Create platform-agnostic AudioClip (updated existing AudioClip)
2. ✅ **Phase 2**: Add missing functions to PlatformAPI (PauseMusic, ResumeMusic, SetMusicLooping)
3. **Phase 3**: Update AudioStateManager to use platform-agnostic AudioClip
4. **Phase 4**: Update AudioManager
5. **Phase 5**: Update ResourceManager
6. **Testing**: Verify all audio functionality works correctly

## Migration Strategy

1. Create new files alongside existing ones
2. Update includes gradually
3. Test each phase before moving to the next
4. Keep old system as fallback during transition
5. Remove old system once new system is proven stable 