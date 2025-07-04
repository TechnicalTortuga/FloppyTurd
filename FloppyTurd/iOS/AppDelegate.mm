#import "AppDelegate.h"
#import "GameViewController.h"
#import "MetalRaylibCompat.h"
#import "HapticsManager.h"

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Create window
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    
    // Create and set root view controller
    GameViewController *gameViewController = [[GameViewController alloc] init];
    self.window.rootViewController = gameViewController;
    
    // Make window visible
    [self.window makeKeyAndVisible];
    
    // Start haptics manager
    [[HapticsManager sharedManager] start];

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