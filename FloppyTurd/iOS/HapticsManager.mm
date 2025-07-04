#import "HapticsManager.h"

@interface HapticsManager ()
@property (nonatomic, strong) CHHapticEngine *hapticEngine;
@end

@implementation HapticsManager

+ (instancetype)sharedManager {
    static HapticsManager *sharedManager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedManager = [[self alloc] init];
    });
    return sharedManager;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        [self createHapticEngine];
    }
    return self;
}

- (void)createHapticEngine {
    if (![CHHapticEngine capabilitiesForHardware].supportsHaptics) {
        NSLog(@"Haptics not supported on this device.");
        return;
    }

    NSError *error = nil;
    _hapticEngine = [[CHHapticEngine alloc] initWithAudioSession:nil error:&error];
    if (error) {
        NSLog(@"Haptic engine creation error: %@", error.localizedDescription);
        return;
    }

    // Set up handlers to restart the engine if it's stopped
    _hapticEngine.stoppedHandler = ^(CHHapticEngineStoppedReason reason) {
        NSLog(@"Haptic engine stopped for reason: %ld", (long)reason);
        // Restart the engine
        [self start];
    };
    _hapticEngine.resetHandler = ^{
        NSLog(@"Haptic engine reset.");
        // Try to restart the engine
        [self start];
    };
}

- (void)start {
    if (!_hapticEngine) return;
    
    NSError *error = nil;
    [_hapticEngine startAndReturnError:&error];
    if (error) {
        NSLog(@"Haptic engine start error: %@", error.localizedDescription);
    }
}

- (void)stop {
    if (_hapticEngine) {
        [_hapticEngine stopWithCompletionHandler:^(NSError * _Nullable error) {
            if (error) {
                NSLog(@"Haptic engine stop error: %@", error.localizedDescription);
            }
        }];
    }
}

- (void)playVibration {
    if (!_hapticEngine) return;

    // Create a simple vibration pattern
    CHHapticEvent *event = [[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticTransient
                                                         parameters:@[]
                                                   relativeTime:0];
    NSError *error = nil;
    CHHapticPattern *pattern = [[CHHapticPattern alloc] initWithEvents:@[event] parameters:@[] error:&error];
    if (error) {
        NSLog(@"Haptic pattern creation error: %@", error.localizedDescription);
        return;
    }
    
    [self playPattern:pattern];
}

- (void)playPattern:(CHHapticPattern *)pattern {
    if (!_hapticEngine) return;

    NSError *error = nil;
    id<CHHapticPatternPlayer> player = [_hapticEngine createPlayerWithPattern:pattern error:&error];
    if (error) {
        NSLog(@"Haptic player creation error: %@", error.localizedDescription);
        return;
    }
    
    [player startAtTime:0 error:&error];
    if (error) {
        NSLog(@"Haptic player start error: %@", error.localizedDescription);
    }
}

@end 