#if defined(PLATFORM_IOS)

#import <UIKit/UIKit.h>
#import "iOS/AppDelegate.h"

extern "C" int main(int argc, char *argv[]) {
    NSLog(@"[INIT] ========================================");
    NSLog(@"[INIT] iOS main() STARTING");
    NSLog(@"[INIT] argc=%d", argc);
    NSLog(@"[INIT] ========================================");
    @autoreleasepool {
        NSLog(@"[INIT] About to call UIApplicationMain");
        int result = UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
        NSLog(@"[INIT] UIApplicationMain returned: %d", result);
        return result;
    }
}

#endif 