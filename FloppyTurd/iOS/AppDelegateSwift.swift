//
//  AppDelegateSwift.swift
//  Professional Game Engine - Native Swift App Delegate
//
//  Created by C++ Swift Interop Migration
//  Modern Swift app entry point with proper lifecycle management
//

#if canImport(UIKit)
import UIKit
#endif

@main
@MainActor
class AppDelegateSwift: UIResponder, UIApplicationDelegate {

    var window: UIWindow?
    private var gameViewController: GameViewControllerSwift?

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        
        print("[AppDelegateSwift] 🚀 FloppyTurd starting up...")
        
        // Create window
        window = UIWindow(frame: UIScreen.main.bounds)
        window?.backgroundColor = UIColor.black
        
        // Create and setup game view controller
        gameViewController = GameViewControllerSwift()
        
        // Set as root view controller
        window?.rootViewController = gameViewController
        window?.makeKeyAndVisible()
        
        print("[AppDelegateSwift] ✅ Application launched successfully")
        return true
    }

    func applicationWillResignActive(_ application: UIApplication) {
        print("[AppDelegateSwift] Application will resign active")
        
        // Pause the game when app loses focus
        gameViewController?.currentGameView?.pauseGameLoop()
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
        print("[AppDelegateSwift] Application entered background")
        
        // Handle background transition
        gameViewController?.currentGameView?.applicationDidEnterBackground()
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        print("[AppDelegateSwift] Application will enter foreground")
        
        // Handle foreground transition
        gameViewController?.currentGameView?.applicationWillEnterForeground()
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        print("[AppDelegateSwift] Application became active")
        
        // Resume the game when app becomes active
        gameViewController?.currentGameView?.resumeGameLoop()
    }

    func applicationWillTerminate(_ application: UIApplication) {
        print("[AppDelegateSwift] Application will terminate")
        
        // Perform final cleanup
        gameViewController?.currentGameView?.stopGameLoop()
        gameViewController = nil
    }
    
    func applicationDidReceiveMemoryWarning(_ application: UIApplication) {
        print("[AppDelegateSwift] Received memory warning")
        
        // Let the game handle memory cleanup
        GameEngine.handleMemoryWarning()
    }
}

// MARK: - Scene Delegate Support (iOS 13+)

@available(iOS 13.0, *)
@MainActor
class SceneDelegateSwift: UIResponder, UIWindowSceneDelegate {

    var window: UIWindow?
    private var gameViewController: GameViewControllerSwift?

    func scene(_ scene: UIScene, willConnectTo session: UISceneSession, options connectionOptions: UIScene.ConnectionOptions) {
        
        guard let windowScene = (scene as? UIWindowScene) else { return }
        
        print("[SceneDelegateSwift] 🚀 Scene connecting...")
        
        // Create window
        window = UIWindow(windowScene: windowScene)
        window?.backgroundColor = UIColor.black
        
        // Create and setup game view controller
        gameViewController = GameViewControllerSwift()
        
        // Set as root view controller
        window?.rootViewController = gameViewController
        window?.makeKeyAndVisible()
        
        print("[SceneDelegateSwift] ✅ Scene connected successfully")
    }

    func sceneDidDisconnect(_ scene: UIScene) {
        print("[SceneDelegateSwift] Scene disconnected")
        
        // Cleanup when scene disconnects
        gameViewController?.currentGameView?.stopGameLoop()
        gameViewController = nil
    }

    func sceneDidBecomeActive(_ scene: UIScene) {
        print("[SceneDelegateSwift] Scene became active")
        
        // Resume game
        gameViewController?.currentGameView?.resumeGameLoop()
    }

    func sceneWillResignActive(_ scene: UIScene) {
        print("[SceneDelegateSwift] Scene will resign active")
        
        // Pause game
        gameViewController?.currentGameView?.pauseGameLoop()
    }

    func sceneWillEnterForeground(_ scene: UIScene) {
        print("[SceneDelegateSwift] Scene entering foreground")
        
        gameViewController?.currentGameView?.applicationWillEnterForeground()
    }

    func sceneDidEnterBackground(_ scene: UIScene) {
        print("[SceneDelegateSwift] Scene entered background")
        
        gameViewController?.currentGameView?.applicationDidEnterBackground()
    }
}
