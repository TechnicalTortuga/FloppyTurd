#import <Foundation/Foundation.h>
#import <CoreHaptics/CoreHaptics.h>

NS_ASSUME_NONNULL_BEGIN

@interface HapticsManager : NSObject

+ (instancetype)sharedManager;

- (void)start;
- (void)stop;
- (void)playVibration;
- (void)playPattern:(CHHapticPattern *)pattern;

@end

NS_ASSUME_NONNULL_END 