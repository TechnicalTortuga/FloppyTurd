#import "GameViewController.h"
#import "GameView.h"
#import "Game.h"
#import "UIManager.h"
#import "PlatformAPI.h"

@interface GameViewController ()
@property (nonatomic, strong) UIActivityIndicatorView *loadingIndicator;
@property (nonatomic, strong) UILabel *loadingLabel;
@end

@implementation GameViewController

- (void)dealloc {
    TraceLog(LOG_INFO, "[GameViewController] Deallocating...");
    
    // Clean up any remaining resources
    if (self.loadingIndicator) {
        [self.loadingIndicator stopAnimating];
    }
    
    // Ensure game loop is stopped
    if (self.gameView) {
        self.gameView.paused = YES;
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    TraceLog(LOG_INFO, "[GVC] viewDidLoad START");
    
    // Initialize loading UI first
    [self setupLoadingUI];
    [self showLoadingIndicator:YES];
    
    @try {
        // Step 1: Create and configure GameView - this will setup Metal synchronously
        TraceLog(LOG_INFO, "[INIT] Step 1: Creating GameView with Metal setup...");
        
        CGRect bounds = self.view.bounds;
        self.device = MTLCreateSystemDefaultDevice();
        if (!self.device) {
            TraceLog(LOG_ERROR, "[INIT] Failed to create Metal device");
            [self showLoadingIndicator:NO];
            [self showErrorAlert:@"Metal is not supported on this device"];
            return;
        }
        
        self.gameView = [[GameView alloc] initWithFrame:bounds device:self.device];
        if (!self.gameView) {
            TraceLog(LOG_ERROR, "[INIT] Failed to create GameView");
            [self showLoadingIndicator:NO];
            [self showErrorAlert:@"Failed to create game view"];
            return;
        }
        
        // Set the global game view for platform API access
        SetGlobalGameView((__bridge void*)self.gameView);
        TraceLog(LOG_INFO, "[INIT] SetGlobalGameView called with GameView: %p", self.gameView);
        
        [self.view insertSubview:self.gameView atIndex:0]; // Insert below loading UI
        TraceLog(LOG_INFO, "[INIT] Step 1: GameView created and Metal setup completed");
        
        // Step 2: Initialize UIManager with screen dimensions
        TraceLog(LOG_INFO, "[INIT] Step 2: Initializing UIManager...");
        
        CGRect nativeBounds = [[UIScreen mainScreen] nativeBounds];
        CGFloat nativeScale = [[UIScreen mainScreen] nativeScale];
        UIEdgeInsets insets = self.view.safeAreaInsets;
        
        CGSize size = bounds.size;
        CGSize sizePixels = CGSizeMake(nativeBounds.size.width, nativeBounds.size.height);
        CGRect safeAreaPoints = UIEdgeInsetsInsetRect(CGRectMake(0, 0, size.width, size.height), insets);
        CGRect safeAreaPixels = CGRectMake(safeAreaPoints.origin.x * nativeScale,
                                           safeAreaPoints.origin.y * nativeScale,
                                           safeAreaPoints.size.width * nativeScale,
                                           safeAreaPoints.size.height * nativeScale);
        
        UIManager& uiManager = UIManager::GetInstance();
        uiManager.Initialize((float)size.width, (float)size.height,
                            (float)sizePixels.width, (float)sizePixels.height,
                            { (float)safeAreaPoints.origin.x, (float)safeAreaPoints.origin.y, (float)safeAreaPoints.size.width, (float)safeAreaPoints.size.height },
                            { (float)safeAreaPixels.origin.x, (float)safeAreaPixels.origin.y, (float)safeAreaPixels.size.width, (float)safeAreaPixels.size.height });
        
        TraceLog(LOG_INFO, "[INIT] Step 2: UIManager initialized successfully");
        
        // Step 3: Now that Metal is ready, initialize the game
        TraceLog(LOG_INFO, "[INIT] Step 3: Metal device ready, initializing game...");
        
        int result = game_main(0, nullptr);
        if (result == 0) {
            self.gameInitialized = YES;
            TraceLog(LOG_INFO, "[INIT] Step 3: Game initialization completed successfully");
            
            // Step 4: Start the game loop (MTKView handles the rendering)
            self.gameView.paused = NO;
            [self showLoadingIndicator:NO];
            TraceLog(LOG_INFO, "[INIT] Step 4: Game loop started");
        } else {
            TraceLog(LOG_ERROR, "[INIT] Step 3: Game initialization failed with code: %d", result);
            [self showLoadingIndicator:NO];
            [self showErrorAlert:[NSString stringWithFormat:@"Game initialization failed. Error code: %d", result]];
            return;
        }
        
        TraceLog(LOG_INFO, "[INIT] GameViewController initialization COMPLETED successfully");
        
    } @catch (NSException *exception) {
        TraceLog(LOG_ERROR, "[EXCEPTION] Exception in viewDidLoad: %@", exception);
        [self showLoadingIndicator:NO];
        [self showErrorAlert:[NSString stringWithFormat:@"Initialization error: %@", exception.reason]];
    } @finally {
        TraceLog(LOG_INFO, "[GVC] viewDidLoad END");
    }
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    TraceLog(LOG_INFO, "[GameViewController] viewWillAppear");
    
    // Ensure the game loop is running if game is initialized
    if (self.gameInitialized && self.gameView) {
        self.gameView.paused = NO;
    }
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    TraceLog(LOG_INFO, "[GameViewController] viewDidAppear");
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    TraceLog(LOG_INFO, "[GameViewController] viewWillDisappear");
    
    // Pause the game when view disappears
    if (self.gameView) {
        self.gameView.paused = YES;
    }
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    TraceLog(LOG_INFO, "[GameViewController] viewDidDisappear");
}

- (void)viewWillLayoutSubviews {
    [super viewWillLayoutSubviews];
    if (self.gameView) {
        self.gameView.frame = self.view.bounds;
    }
    
    // Update loading UI layout
    [self updateLoadingUILayout];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    
    [coordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        CGRect nativeBounds = [[UIScreen mainScreen] nativeBounds];
        CGFloat nativeScale = [[UIScreen mainScreen] nativeScale];
        UIEdgeInsets insets = self.view.safeAreaInsets;
        
        CGSize sizePixels = CGSizeMake(nativeBounds.size.width, nativeBounds.size.height);
        CGRect safeAreaPoints = UIEdgeInsetsInsetRect(CGRectMake(0, 0, size.width, size.height), insets);
        CGRect safeAreaPixels = CGRectMake(safeAreaPoints.origin.x * nativeScale,
                                           safeAreaPoints.origin.y * nativeScale,
                                           safeAreaPoints.size.width * nativeScale,
                                           safeAreaPoints.size.height * nativeScale);
        
        UIManager& uiManager = UIManager::GetInstance();
        uiManager.Initialize((float)size.width, (float)size.height,
                            (float)sizePixels.width, (float)sizePixels.height,
                            { (float)safeAreaPoints.origin.x, (float)safeAreaPoints.origin.y, (float)safeAreaPoints.size.width, (float)safeAreaPoints.size.height },
                            { (float)safeAreaPixels.origin.x, (float)safeAreaPixels.origin.y, (float)safeAreaPixels.size.width, (float)safeAreaPixels.size.height });
        
        TraceLog(LOG_INFO, "[SYSTEM] Orientation changed, updated UIManager: points=%.0fx%.0f, pixels=%.0fx%.0f", 
              size.width, size.height, sizePixels.width, sizePixels.height);
    }];
}

- (BOOL)shouldAutorotate {
    return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

// Placeholder methods for header declarations
- (void)showErrorAlert:(NSString*)message {
    TraceLog(LOG_ERROR, "[ERROR] %s", [message UTF8String]);
    
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Error" 
                                                                   message:message 
                                                            preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];
    [alert addAction:okAction];
    [self presentViewController:alert animated:YES completion:nil];
}

- (void)showLoadingIndicator:(BOOL)show {
    TraceLog(LOG_INFO, "[UI] Loading indicator: %s", show ? "SHOW" : "HIDE");
    
    if (show) {
        if (!self.loadingIndicator) {
            [self setupLoadingUI];
        }
        
        self.loadingIndicator.hidden = NO;
        self.loadingLabel.hidden = NO;
        [self.loadingIndicator startAnimating];
        
        // Bring loading UI to front
        [self.view bringSubviewToFront:self.loadingIndicator];
        [self.view bringSubviewToFront:self.loadingLabel];
        
        // Update loading text
        self.loadingLabel.text = @"Loading FloppyTurd...";
    } else {
        if (self.loadingIndicator) {
            [self.loadingIndicator stopAnimating];
            self.loadingIndicator.hidden = YES;
            self.loadingLabel.hidden = YES;
        }
    }
}

- (void)startGameLoop {
    TraceLog(LOG_INFO, "[GAME] Starting game loop...");
    
    if (!self.gameInitialized) {
        TraceLog(LOG_WARNING, "[GAME] Cannot start game loop - game not initialized");
        return;
    }
    
    if (!self.gameView) {
        TraceLog(LOG_ERROR, "[GAME] Cannot start game loop - GameView not available");
        return;
    }
    
    // MTKView handles the game loop automatically
    self.gameView.paused = NO;
    self.gameView.enableSetNeedsDisplay = NO; // Use display link instead of setNeedsDisplay
    
    TraceLog(LOG_INFO, "[GAME] Game loop started via MTKView");
}

- (void)gameLoopTick:(CADisplayLink *)sender {
    // This method is provided for compatibility but not used in MTKView approach
    // MTKView manages its own display link and calls drawInMTKView automatically
    // If needed for fallback or special cases, would need to call Game::Update() here
    TraceLog(LOG_DEBUG, "[GAME] gameLoopTick called (MTKView approach doesn't use this)");
}

- (void)initializeGameAfterMetalSetup {
    TraceLog(LOG_INFO, "[GAME] initializeGameAfterMetalSetup called");
    
    // This method is called by GameView after Metal setup is complete
    // In our simplified single-threaded approach, this is not needed since
    // Metal setup is synchronous in viewDidLoad, but we keep it for compatibility
    
    if (self.gameInitialized) {
        TraceLog(LOG_INFO, "[GAME] Game already initialized, skipping");
        return;
    }
    
    TraceLog(LOG_INFO, "[GAME] Game not yet initialized, calling game_main...");
    
    int result = game_main(0, nullptr);
    if (result == 0) {
        self.gameInitialized = YES;
        [self showLoadingIndicator:NO];
        [self startGameLoop];
        TraceLog(LOG_INFO, "[GAME] Game initialization completed successfully");
    } else {
        TraceLog(LOG_ERROR, "[GAME] Game initialization failed with code: %d", result);
        [self showLoadingIndicator:NO];
        [self showErrorAlert:[NSString stringWithFormat:@"Game initialization failed. Error code: %d", result]];
    }
}

// MARK: - Loading UI Setup

- (void)setupLoadingUI {
    if (self.loadingIndicator) {
        return; // Already setup
    }
    
    // Create loading indicator
    self.loadingIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleLarge];
    self.loadingIndicator.color = [UIColor whiteColor];
    self.loadingIndicator.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:self.loadingIndicator];
    
    // Create loading label
    self.loadingLabel = [[UILabel alloc] init];
    self.loadingLabel.text = @"Loading FloppyTurd...";
    self.loadingLabel.textColor = [UIColor whiteColor];
    self.loadingLabel.font = [UIFont systemFontOfSize:18.0];
    self.loadingLabel.textAlignment = NSTextAlignmentCenter;
    self.loadingLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:self.loadingLabel];
    
    [self updateLoadingUILayout];
    
    TraceLog(LOG_INFO, "[UI] Loading UI setup completed");
}

- (void)updateLoadingUILayout {
    if (!self.loadingIndicator || !self.loadingLabel) {
        return;
    }
    
    // Remove existing constraints
    [self.loadingIndicator removeFromSuperview];
    [self.loadingLabel removeFromSuperview];
    [self.view addSubview:self.loadingIndicator];
    [self.view addSubview:self.loadingLabel];
    
    // Center the loading indicator
    [NSLayoutConstraint activateConstraints:@[
        [self.loadingIndicator.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [self.loadingIndicator.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor constant:-30]
    ]];
    
    // Position label below indicator
    [NSLayoutConstraint activateConstraints:@[
        [self.loadingLabel.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [self.loadingLabel.topAnchor constraintEqualToAnchor:self.loadingIndicator.bottomAnchor constant:20],
        [self.loadingLabel.leadingAnchor constraintGreaterThanOrEqualToAnchor:self.view.leadingAnchor constant:20],
        [self.loadingLabel.trailingAnchor constraintLessThanOrEqualToAnchor:self.view.trailingAnchor constant:-20]
    ]];
}

@end
