#pragma once

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

// Forward declarations
@class PlatformLayerDelegate;

@interface GameViewController : UIViewController

- (void)showErrorAlert:(NSString*)message;
- (void)showLoadingIndicator:(BOOL)show;

@end