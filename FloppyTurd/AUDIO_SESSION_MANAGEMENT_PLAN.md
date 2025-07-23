# Audio Session Management Plan

## Current State
- Basic audio session activation/deactivation implemented
- Missing interruption handling (phone calls, Siri, etc.)
- Missing background/foreground transition handling
- Missing audio session lifecycle management

## Target Features

### 1. Audio Session Interruption Handling
- **Phone Calls**: Pause music when call comes in, resume when call ends
- **Siri**: Handle Siri activation/deactivation
- **Other Apps**: Handle audio from other apps
- **System Alerts**: Handle system audio interruptions

### 2. Background/Foreground Transitions
- **App Backgrounding**: Pause audio when app goes to background
- **App Foregrounding**: Resume audio when app comes to foreground
- **Audio Session Reactivation**: Proper session management

### 3. Audio Session Lifecycle
- **Session Configuration**: Set up proper audio session categories
- **Route Changes**: Handle headphone plug/unplug
- **Volume Changes**: Handle system volume changes
- **Audio Focus**: Manage audio focus with other apps

## Implementation Plan

### Phase 1: Audio Session Configuration
- [ ] Configure audio session category and options
- [ ] Set up audio session interruption notifications
- [ ] Set up route change notifications
- [ ] Set up audio focus notifications

### Phase 2: Interruption Handling
- [ ] Implement interruption begin handler
- [ ] Implement interruption end handler
- [ ] Handle different interruption types
- [ ] Save/restore audio state during interruptions

### Phase 3: Background/Foreground Handling
- [ ] Implement app background handler
- [ ] Implement app foreground handler
- [ ] Handle audio session reactivation
- [ ] Manage audio state during transitions

### Phase 4: Route and Volume Changes
- [ ] Handle headphone plug/unplug
- [ ] Handle Bluetooth device changes
- [ ] Handle system volume changes
- [ ] Update audio routing appropriately

## Technical Implementation

### Audio Session Configuration
```objc
// Configure audio session for game audio
AVAudioSession* session = [AVAudioSession sharedInstance];
[session setCategory:AVAudioSessionCategoryPlayback 
         withOptions:AVAudioSessionCategoryOptionMixWithOthers 
               error:&error];

// Set up interruption notifications
[[NSNotificationCenter defaultCenter] addObserver:self
                                         selector:@selector(handleInterruption:)
                                             name:AVAudioSessionInterruptionNotification
                                           object:session];

// Set up route change notifications
[[NSNotificationCenter defaultCenter] addObserver:self
                                         selector:@selector(handleRouteChange:)
                                             name:AVAudioSessionRouteChangeNotification
                                           object:session];
```

### Interruption Handling
```objc
- (void)handleInterruption:(NSNotification*)notification {
    NSDictionary* info = notification.userInfo;
    AVAudioSessionInterruptionType type = [info[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
    
    if (type == AVAudioSessionInterruptionTypeBegan) {
        // Interruption began - pause audio
        [self pauseAudioForInterruption];
    } else if (type == AVAudioSessionInterruptionTypeEnded) {
        // Interruption ended - resume audio
        AVAudioSessionInterruptionOptions options = [info[AVAudioSessionInterruptionOptionKey] unsignedIntegerValue];
        if (options & AVAudioSessionInterruptionOptionShouldResume) {
            [self resumeAudioAfterInterruption];
        }
    }
}
```

### Background/Foreground Handling
```objc
- (void)handleAppBackgrounding {
    // App is going to background
    [self pauseAudioForBackground];
    [self deactivateAudioSession];
}

- (void)handleAppForegrounding {
    // App is coming to foreground
    [self reactivateAudioSession];
    [self resumeAudioForForeground];
}
```

## Integration with PlatformIOS

### New Member Variables
```cpp
class PlatformIOS {
private:
    // Audio session management
    bool m_audioSessionActive;
    bool m_wasInterrupted;
    bool m_wasInBackground;
    float m_preInterruptionVolume;
    bool m_preInterruptionPlaying;
    
    // Notification observers
    id m_interruptionObserver;
    id m_routeChangeObserver;
    id m_appStateObserver;
};
```

### New Methods
```cpp
// Audio session management
void ConfigureAudioSession();
void HandleAudioInterruption(bool began);
void HandleRouteChange();
void HandleAppBackgrounding();
void HandleAppForegrounding();
void SaveAudioState();
void RestoreAudioState();
```

## Benefits

### 1. Professional Audio Behavior
- Proper handling of phone calls and system interruptions
- Seamless background/foreground transitions
- Professional audio session management

### 2. User Experience
- No audio conflicts with other apps
- Proper audio routing (headphones, speakers, etc.)
- Consistent audio behavior across iOS versions

### 3. System Integration
- Respects iOS audio session guidelines
- Proper audio focus management
- Handles all iOS audio interruption types

## Next Steps

1. **Implement audio session configuration** in PlatformIOS
2. **Add interruption handling** with proper state management
3. **Add background/foreground handling** for app lifecycle
4. **Test with real interruptions** (phone calls, Siri, etc.)
5. **Integrate with existing audio architecture** 