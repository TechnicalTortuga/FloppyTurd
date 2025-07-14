#pragma once

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>

// Forward declarations
@class GameView;

@interface GameViewController : UIViewController

// Metal and rendering
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) GameView *gameView;

// Game state
@property (nonatomic, assign) BOOL gameInitialized;
@property (nonatomic, strong) dispatch_queue_t gameQueue;

// Game loop
@property (nonatomic, strong) CADisplayLink *displayLink;
@property (nonatomic, assign) CFTimeInterval previousTime;

// Methods
- (void)showErrorAlert:(NSString*)message;
- (void)showLoadingIndicator:(BOOL)show;
- (void)startGameLoop;
- (void)gameLoopTick:(CADisplayLink *)sender;
- (void)initializeGameAfterMetalSetup; // Called by GameView when Metal is ready

@end