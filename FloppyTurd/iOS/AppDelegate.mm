#import "AppDelegate.h"
#import "GameViewController.h"
#import "HapticsManager.h"
#import "PlatformAPI.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    TraceLog(LOG_INFO, "[INIT] ========================================");
    TraceLog(LOG_INFO, "[INIT] AppDelegate didFinishLaunchingWithOptions STARTING");
    TraceLog(LOG_INFO, "[INIT] ========================================");
    
    // Create window
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    TraceLog(LOG_INFO, "[INIT] Created window: %p", self.window);
    if (!self.window) {
        TraceLog(LOG_ERROR, "[INIT] Window is nil after creation!");
        return NO;
    }
    
    // Create and set root view controller
    TraceLog(LOG_INFO, "[INIT] Creating GameViewController");
    GameViewController *gameViewController = [[GameViewController alloc] init];
    TraceLog(LOG_INFO, "[INIT] GameViewController created: %p", gameViewController);
    if (!gameViewController) {
        TraceLog(LOG_ERROR, "[INIT] GameViewController is nil after creation!");
        return NO;
    }
    self.window.rootViewController = gameViewController;
    TraceLog(LOG_INFO, "[INIT] Set rootViewController: %p", self.window.rootViewController);
    if (!self.window.rootViewController) {
        TraceLog(LOG_ERROR, "[INIT] rootViewController is nil after setting!");
        return NO;
    }
    
    // Make window key and visible
    [self.window makeKeyAndVisible];
    TraceLog(LOG_INFO, "[INIT] Made window key and visible");
    
    // Start haptics manager
    [[HapticsManager sharedManager] start];
    TraceLog(LOG_INFO, "[INIT] Started haptics manager");

    TraceLog(LOG_INFO, "[INIT] didFinishLaunchingWithOptions COMPLETED");
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state
    OnAppPause();
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers
    [[HapticsManager sharedManager] stop];
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the active state
    [[HapticsManager sharedManager] start];
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive
    OnAppResume();
}

@end 