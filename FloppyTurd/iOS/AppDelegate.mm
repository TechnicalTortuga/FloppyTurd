#import "AppDelegate.h"
#import "GameViewController.h"
#import "HapticsManager.h"
#import "PlatformAPI.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    NSLog(@"[INIT] ========================================");
    NSLog(@"[INIT] AppDelegate didFinishLaunchingWithOptions STARTING");
    NSLog(@"[INIT] ========================================");
    
    // Create window
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    NSLog(@"[INIT] Created window with frame: %@", NSStringFromCGRect([[UIScreen mainScreen] bounds]));
    
    // Create and set root view controller
    NSLog(@"[INIT] Creating GameViewController");
    GameViewController *gameViewController = [[GameViewController alloc] init];
    NSLog(@"[INIT] Created GameViewController: %@", gameViewController);
    
    self.window.rootViewController = gameViewController;
    NSLog(@"[INIT] Set root view controller");
    
    // Make window visible
    [self.window makeKeyAndVisible];
    NSLog(@"[INIT] Made window key and visible");
    
    // Start haptics manager
    [[HapticsManager sharedManager] start];
    NSLog(@"[INIT] Started haptics manager");

    NSLog(@"[INIT] ========================================");
    NSLog(@"[INIT] AppDelegate didFinishLaunchingWithOptions COMPLETED");
    NSLog(@"[INIT] ========================================");
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