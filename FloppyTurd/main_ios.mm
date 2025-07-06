#if defined(PLATFORM_IOS)

#import <UIKit/UIKit.h>
#import "iOS/AppDelegate.h"

extern "C" int main(int argc, char *argv[]) {
    NSLog(@"[DEBUG] iOS main() starting with argc=%d", argc);
    @autoreleasepool {
        NSLog(@"[DEBUG] About to call UIApplicationMain");
        int result = UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
        NSLog(@"[DEBUG] UIApplicationMain returned: %d", result);
        return result;
    }
}

#endif 