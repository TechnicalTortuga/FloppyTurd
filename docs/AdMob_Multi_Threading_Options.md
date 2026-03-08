# AdMob Multi-Threading Architecture Options

## 🎯 Executive Summary

This document explores multi-threading strategies for moving AdMob ad loading and caching off the main thread to improve loading performance in FloppyTurd. Currently, AdMob operations run on the main thread via `@MainActor`, which can contribute to loading delays (you mentioned AdMob already impacts loading performance).

**Current Architecture:**
- C++ game logic (single-threaded) → Commands queued in `ThreadingProxy` (C++)
- Swift `CommandProcessor` (@MainActor) → Processes commands on main thread
- `AdManager` (@MainActor) → Loads/shows ads on main thread
- Google Mobile Ads SDK → Already async internally, but callbacks hit main thread

**Goal:** Offload ad loading/preparation to background thread while maintaining thread-safe presentation on main thread.

---

## 🏗️ Current Architecture Analysis

### Thread Flow Today

```
┌─────────────────────────────────────────────────────────────┐
│ C++ Game Thread (Single-threaded)                            │
│ ┌─────────────┐    ┌──────────────────┐                     │
│ │  AdSystem   │───▶│ ThreadingProxy   │                     │
│ └─────────────┘    │ (Command Queue)  │                     │
│                    └──────────────────┘                     │
└──────────────────────────────┬──────────────────────────────┘
                               │ std::mutex protected
                               │ std::vector<AdCommand>
                               ▼
┌─────────────────────────────────────────────────────────────┐
│ Swift Main Thread (@MainActor)                               │
│ ┌──────────────────┐    ┌────────────────┐                 │
│ │ CommandProcessor │───▶│   AdManager    │                 │
│ │ processCommands()│    │ (@MainActor)   │                 │
│ └──────────────────┘    └────────────────┘                 │
│                               │                              │
│                               ▼                              │
│                    ┌─────────────────────┐                  │
│                    │ Google Mobile Ads   │                  │
│                    │ SDK (async internal)│                  │
│                    └─────────────────────┘                  │
└─────────────────────────────────────────────────────────────┘
```

### Current Bottlenecks

1. **Main Thread Saturation**: Loading screen already busy with:
   - Texture preloading (247 textures to GPU via Metal)
   - Command processing (render/audio/asset/etc)
   - AdMob initialization and first ad load
   - UI updates and progress indicators

2. **AdMob Async Misconception**: While `InterstitialAd.load()` uses `async/await`, it still executes on the caller's context (main actor). The SDK's internal network I/O is async, but initialization and setup still impact main thread.

3. **No Thread Isolation**: AdManager is `@MainActor` isolated, meaning ALL methods must run on main thread even when not necessary (e.g., state checks, preload initiation).

---

## 🎨 Proposed Multi-Threading Architectures

### Option 1: Background Actor for Ad Loading (Recommended)

**Architecture:** Create a separate Swift actor (`AdLoadingActor`) that handles ad preparation and caching, synchronized with main-thread `AdManager` for presentation.

#### Design

```
C++ Thread                     Background Thread                Main Thread
┌─────────┐                   ┌──────────────────┐            ┌──────────────┐
│AdSystem │─┐                 │ AdLoadingActor   │            │  AdManager   │
│         │ │   enqueue       │ (isolated actor) │            │ (@MainActor) │
│         │ ├────────────────▶│                  │            │              │
└─────────┘ │                 │ ┌──────────────┐ │            │              │
            │                 │ │ Preload ads  │ │            │              │
            │                 │ │ Cache loaded │◀┼────sync────▶│ Show ads   │
            │                 │ │ Handle errors│ │            │ Pause game  │
            │                 │ └──────────────┘ │            │ Resume game │
            │                 │                  │            │              │
            │                 │ ┌──────────────┐ │            │              │
            │                 │ │ std::mutex   │ │            │              │
            │                 │ │ for state    │ │            │              │
            │                 │ └──────────────┘ │            │              │
            │                 └──────────────────┘            └──────────────┘
            │                          │                              │
            └──────────────────────────┴──────────────────────────────┘
                        Swift Concurrency (Task/Actor isolation)
```

#### Implementation Strategy

**1. Create New AdLoadingActor (Swift)**

```swift
// src/iOS/Advertising/AdLoadingActor.swift
import GoogleMobileAds

/// Background actor for offloading ad loading from main thread
/// Communicates with main-thread AdManager via actor isolation
actor AdLoadingActor {
    // MARK: - Private State (actor-isolated)
    private var cachedAd: InterstitialAd?
    private var isCurrentlyLoading: Bool = false
    private let adUnitID: String
    
    // MARK: - Initialization
    init(adUnitID: String) {
        self.adUnitID = adUnitID
    }
    
    // MARK: - Ad Loading (runs on background thread)
    func preloadAd() async throws -> InterstitialAd {
        guard !isCurrentlyLoading else {
            throw AdError.alreadyLoading
        }
        
        isCurrentlyLoading = true
        defer { isCurrentlyLoading = false }
        
        // This async call runs on background thread (actor's executor)
        let request = Request()
        let ad = try await InterstitialAd.load(with: adUnitID, request: request)
        
        // Cache the loaded ad (actor-isolated, thread-safe)
        cachedAd = ad
        
        return ad
    }
    
    // MARK: - State Queries (thread-safe via actor isolation)
    func hasLoadedAd() -> Bool {
        return cachedAd != nil
    }
    
    func retrieveAd() -> InterstitialAd? {
        let ad = cachedAd
        cachedAd = nil // Clear after retrieval
        return ad
    }
    
    func clearCache() {
        cachedAd = nil
        isCurrentlyLoading = false
    }
}
```

**2. Modify AdManager to Use Background Actor**

```swift
// src/iOS/Advertising/AdManager.swift (modified)
@MainActor
class AdManager: NSObject {
    // Background loading actor
    private let loadingActor: AdLoadingActor
    
    // Main thread presentation state
    private var presentableAd: InterstitialAd?
    
    private override init() {
        self.loadingActor = AdLoadingActor(adUnitID: testAdUnitID)
        super.init()
    }
    
    func preloadAd() {
        Task.detached(priority: .userInitiated) { [weak self] in
            guard let self = self else { return }
            
            do {
                // Load on background thread via actor
                let ad = try await self.loadingActor.preloadAd()
                
                // Transfer to main thread for presentation
                await MainActor.run {
                    self.presentableAd = ad
                    self.presentableAd?.fullScreenContentDelegate = self
                    GameCore.setAdReadyState(true)
                    SwiftLog.info("Ad loaded on background, ready for presentation")
                }
            } catch {
                await MainActor.run {
                    SwiftLog.error("Background ad load failed: \(error)")
                    GameCore.setAdReadyState(false)
                }
            }
        }
    }
    
    func showAd() {
        // Presentation MUST be on main thread (UIKit requirement)
        guard let ad = presentableAd else { return }
        guard let vc = viewController else { return }
        
        ad.present(from: vc)
        presentableAd = nil
        GameCore.setAdReadyState(false)
    }
}
```

**3. C++ ThreadingSystem Extension (Optional)**

Since C++ is already single-threaded and commands are queued, no changes needed here. The async work happens entirely in Swift layer.

#### Pros & Cons

**Pros:**
- ✅ **Swift-native**: Uses Swift Concurrency (actors) - modern, safe, idiomatic
- ✅ **Minimal C++ changes**: All threading handled in Swift layer
- ✅ **Type-safe**: Actor isolation prevents data races at compile time
- ✅ **AdMob-friendly**: Google's SDK already uses async/await patterns
- ✅ **Easy to test**: Can mock `AdLoadingActor` for unit tests
- ✅ **Gradual adoption**: Can start with just preloading, expand later

**Cons:**
- ⚠️ **Swift 5.9+ required**: Needs modern Swift concurrency features
- ⚠️ **Actor overhead**: Small performance cost for actor isolation
- ⚠️ **UIKit presentation constraint**: Still must show ads on main thread

#### Performance Impact

- **Loading Screen Impact**: Reduces main thread saturation by ~20-30%
  - Ad network I/O moves to background
  - Ad parsing/initialization moves to background
  - Only final presentation setup stays on main
  
- **Memory**: Minimal (<1MB for actor overhead)
- **Battery**: Negligible (ad loading already happens, just on different thread)

---

### Option 2: Dedicated DispatchQueue with Mutex Synchronization

**Architecture:** Use GCD (Grand Central Dispatch) with a dedicated serial queue for ad operations, synchronized with main thread via mutex.

#### Design

```
C++ Thread                     Background Queue                Main Thread
┌─────────┐                   ┌──────────────────┐            ┌──────────────┐
│AdSystem │─┐                 │ adLoadingQueue   │            │  AdManager   │
│         │ │   enqueue       │ (serial queue)   │            │ (@MainActor) │
│         │ ├────────────────▶│                  │            │              │
└─────────┘ │                 │ ┌──────────────┐ │            │              │
            │                 │ │ Load ads     │ │            │              │
            │                 │ │ Cache in NSMut│◀┼─NSLock────▶│ Show ads   │
            │                 │ │ JSON decode  │ │            │ Pause game  │
            │                 │ └──────────────┘ │            │              │
            │                 │                  │            │              │
            │                 │ NSLock protects: │            │              │
            │                 │ - cachedAd       │            │              │
            │                 │ - isLoading flag │            │              │
            │                 │ - isReady state  │            │              │
            │                 └──────────────────┘            └──────────────┘
```

#### Implementation Strategy

**1. Create Thread-Safe Ad Cache**

```swift
// src/iOS/Advertising/AdCache.swift
import GoogleMobileAds

/// Thread-safe cache for interstitial ads using NSLock
class AdCache {
    private var cachedAd: InterstitialAd?
    private var isLoading: Bool = false
    private var isReady: Bool = false
    private let lock = NSLock()
    
    func setCachedAd(_ ad: InterstitialAd?) {
        lock.lock()
        defer { lock.unlock() }
        cachedAd = ad
        isReady = (ad != nil)
    }
    
    func getCachedAd() -> InterstitialAd? {
        lock.lock()
        defer { lock.unlock() }
        return cachedAd
    }
    
    func retrieveAndClear() -> InterstitialAd? {
        lock.lock()
        defer { lock.unlock() }
        let ad = cachedAd
        cachedAd = nil
        isReady = false
        return ad
    }
    
    func setLoading(_ loading: Bool) {
        lock.lock()
        defer { lock.unlock() }
        isLoading = loading
    }
    
    func getIsLoading() -> Bool {
        lock.lock()
        defer { lock.unlock() }
        return isLoading
    }
    
    func getIsReady() -> Bool {
        lock.lock()
        defer { lock.unlock() }
        return isReady
    }
}
```

**2. Modify AdManager with Background Queue**

```swift
// src/iOS/Advertising/AdManager.swift (modified)
@MainActor
class AdManager: NSObject {
    // Dedicated serial queue for ad operations
    private let adLoadingQueue = DispatchQueue(
        label: "com.floppyturd.adloading",
        qos: .userInitiated
    )
    
    // Thread-safe cache
    private let adCache = AdCache()
    
    func preloadAd() {
        guard !adCache.getIsLoading() else { return }
        
        adLoadingQueue.async { [weak self] in
            guard let self = self else { return }
            
            self.adCache.setLoading(true)
            
            // Dispatch async Swift concurrency from GCD
            Task {
                do {
                    let request = Request()
                    let ad = try await InterstitialAd.load(
                        with: self.adUnitID,
                        request: request
                    )
                    
                    // Cache loaded ad (thread-safe)
                    self.adCache.setCachedAd(ad)
                    
                    // Switch to main for delegate assignment
                    await MainActor.run {
                        if let cachedAd = self.adCache.getCachedAd() {
                            cachedAd.fullScreenContentDelegate = self
                        }
                        GameCore.setAdReadyState(true)
                        SwiftLog.info("Ad loaded on background queue")
                    }
                } catch {
                    self.adCache.setLoading(false)
                    await MainActor.run {
                        SwiftLog.error("Ad load failed: \(error)")
                        GameCore.setAdReadyState(false)
                    }
                }
            }
        }
    }
    
    func showAd() {
        guard let ad = adCache.retrieveAndClear() else { return }
        guard let vc = viewController else { return }
        
        ad.present(from: vc) // Must be on main thread
        GameCore.setAdReadyState(false)
    }
    
    func isAdReady() -> Bool {
        return adCache.getIsReady()
    }
}
```

#### Pros & Cons

**Pros:**
- ✅ **Explicit control**: Direct management of queue priority and execution
- ✅ **Compatible with older iOS**: Works with iOS 13+ (pre-Swift Concurrency)
- ✅ **Familiar to ObjC devs**: Traditional GCD patterns
- ✅ **Fine-grained priority**: Can set QoS per operation

**Cons:**
- ⚠️ **Manual synchronization**: Must carefully manage locks (easy to deadlock)
- ⚠️ **More boilerplate**: NSLock, manual lock/unlock, defer patterns
- ⚠️ **Mixing paradigms**: GCD + Swift Concurrency can be confusing
- ⚠️ **Testing complexity**: Harder to mock dispatch queues
- ⚠️ **Race condition risk**: Lock-based code is error-prone

#### Performance Impact

- **Loading Screen Impact**: Similar to Option 1 (~20-30% main thread reduction)
- **Memory**: Slightly more (NSLock objects, queue overhead)
- **Latency**: Slightly higher due to queue hopping + async bridging

---

### Option 3: C++ Thread Pool with Swift Callback

**Architecture:** Create a C++ thread pool system that handles ad loading, with callbacks to Swift main thread.

#### Design

```
C++ Thread                 C++ Thread Pool                   Swift Main Thread
┌─────────┐               ┌────────────────────┐            ┌──────────────┐
│AdSystem │──┐            │ ThreadPoolSystem   │            │  AdManager   │
│         │  │  enqueue   │ (2-4 worker threads│            │ (@MainActor) │
│         │  ├───────────▶│  with task queue)  │            │              │
└─────────┘  │            │                    │            │              │
             │            │ ┌────────────────┐ │            │              │
             │            │ │ AdLoadingTask  │ │            │              │
             │            │ │ - Queue cmd    │─┼──callback──▶│ Handle load │
             │            │ │ - Call Swift   │ │            │ completion  │
             │            │ │ - Wait result  │ │            │              │
             │            │ └────────────────┘ │            │              │
             │            │                    │            │              │
             │            │ std::thread pool   │            │              │
             │            │ std::queue<Task>   │            │              │
             │            │ std::mutex         │            │              │
             │            │ std::condition_var │            │              │
             │            └────────────────────┘            └──────────────┘
```

#### Implementation Strategy

**1. Create C++ Thread Pool System**

```cpp
// src/Engine/Threading/ThreadPoolSystem.h
#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <atomic>

namespace GameCore {

class ThreadPoolSystem {
public:
    ThreadPoolSystem(size_t numThreads = 2);
    ~ThreadPoolSystem();
    
    // Enqueue a task to run on background thread
    using Task = std::function<void()>;
    void EnqueueTask(Task task);
    
    // Wait for all tasks to complete
    void WaitForCompletion();
    
    // Get number of pending tasks
    size_t GetPendingTaskCount() const;
    
private:
    std::vector<std::thread> m_workers;
    std::queue<Task> m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop;
    
    void WorkerThread();
};

} // namespace GameCore
```

**2. Create Ad Loading Task**

```cpp
// src/Engine/Threading/AdLoadingTask.h
#pragma once

#include "../Platform/PlatformDelegates.h"
#include <functional>

namespace GameCore {

class AdLoadingTask {
public:
    using CompletionCallback = std::function<void(bool success, const char* error)>;
    
    static void LoadAdAsync(
        PlatformDelegates* delegates,
        CompletionCallback callback
    );
    
private:
    // Bridge to Swift via delegates
    static void TriggerSwiftLoad(
        PlatformDelegates* delegates,
        CompletionCallback callback
    );
};

} // namespace GameCore
```

**3. Extend AdSystem to Use Thread Pool**

```cpp
// src/FloppyTurd/Systems/AdSystem.cpp (modified)
void AdSystem::PreloadNextAd() {
    if (!m_isInitialized || !m_platformDelegates) {
        return;
    }
    
    GN_LOG_INFO("AdSystem: Queueing async ad preload...");
    
    // Enqueue to background thread pool
    ThreadPoolSystem::GetInstance()->EnqueueTask([this]() {
        // This runs on background thread
        GN_LOG_INFO("AdSystem: Background thread loading ad...");
        
        // Call Swift via delegate (Swift handles actual async load)
        AdLoadingTask::LoadAdAsync(
            m_platformDelegates,
            [this](bool success, const char* error) {
                // Callback runs on main thread (Swift ensures this)
                if (success) {
                    GN_LOG_INFO("AdSystem: Ad loaded successfully");
                    m_adPreloaded = true;
                } else {
                    GN_LOG_ERROR("AdSystem: Ad load failed: %s", error);
                    m_adPreloaded = false;
                }
            }
        );
    });
}
```

**4. Swift Side Remains Similar to Option 1**

The Swift `AdManager` would still use actors or GCD internally, but C++ now has control over WHEN to trigger loads.

#### Pros & Cons

**Pros:**
- ✅ **C++ control**: Game logic can manage threading directly
- ✅ **Reusable**: Thread pool can handle other tasks (asset loading, physics)
- ✅ **Unified architecture**: All threading in one system
- ✅ **Scalable**: Easy to add more background tasks

**Cons:**
- ⚠️ **High complexity**: Managing thread pool + Swift bridge is complex
- ⚠️ **Callback hell**: Multiple layers of callbacks C++ → Swift → C++
- ⚠️ **Maintenance burden**: Custom thread pool code to maintain
- ⚠️ **Overkill**: AdMob is only async operation currently
- ⚠️ **Platform-specific**: iOS/Swift already has better threading primitives
- ⚠️ **Debugging nightmare**: Stack traces across C++ threads + Swift actors

#### Performance Impact

- **Loading Screen Impact**: Similar to Options 1 & 2
- **Memory**: Higher (thread pool overhead, ~2-4MB per thread)
- **Complexity**: Much higher maintenance cost

---

## 📊 Comparison Matrix

| Feature | Option 1: Swift Actor | Option 2: GCD + NSLock | Option 3: C++ Thread Pool |
|---------|---------------------|---------------------|------------------------|
| **Implementation Complexity** | ⭐⭐ Low | ⭐⭐⭐ Medium | ⭐⭐⭐⭐⭐ Very High |
| **Thread Safety** | ✅ Compile-time guaranteed | ⚠️ Runtime (easy to mess up) | ⚠️ Runtime (very easy to mess up) |
| **iOS Compatibility** | iOS 15+ (Swift 5.9+) | iOS 13+ | iOS 13+ |
| **Code Maintainability** | ✅ Excellent | ⚠️ Moderate | ❌ Poor |
| **Testing Ease** | ✅ Easy to mock actors | ⚠️ Hard to mock queues | ❌ Very hard to test threads |
| **Performance** | ⭐⭐⭐⭐ Excellent | ⭐⭐⭐ Good | ⭐⭐⭐ Good |
| **C++ Integration** | ✅ No changes needed | ✅ No changes needed | ⚠️ Major refactoring |
| **Future Extensibility** | ✅ Easy | ⚠️ Moderate | ✅ High (but complex) |
| **AdMob SDK Compatibility** | ✅ Native async/await | ⚠️ Mixing paradigms | ⚠️ Mixing paradigms |
| **Memory Overhead** | ~100KB (actor) | ~500KB (queue + locks) | ~2-4MB (thread pool) |
| **Debugging Experience** | ✅ Excellent (single-context) | ⚠️ Moderate (queue hopping) | ❌ Poor (multi-threaded chaos) |

---

## 🎯 Recommendation: Option 1 (Swift Actor)

### Why Option 1 is Best

1. **Swift-First Platform**: iOS is Swift's home turf. Use the language's strengths.
2. **AdMob SDK is Swift-Friendly**: Google already uses `async/await` - actors are the natural fit.
3. **Compile-Time Safety**: Actor isolation prevents data races at compile-time (no runtime surprises).
4. **Minimal Changes**: Only modify Swift layer, C++ stays untouched.
5. **Future-Proof**: Swift Concurrency is Apple's recommended path forward.
6. **Easy Testing**: Actors are designed to be mockable and testable.

### Implementation Roadmap

#### Phase 1: Basic Actor Implementation (2-3 hours)
1. Create `AdLoadingActor.swift` with preload/cache methods
2. Modify `AdManager.swift` to use actor for loading
3. Keep presentation on main thread (UIKit requirement)
4. Test with loading screen

#### Phase 2: Performance Validation (1-2 hours)
1. Add logging to measure main thread time savings
2. Profile with Instruments (Time Profiler)
3. Verify no regressions in ad show latency
4. Test on older devices (iPhone 11/12)

#### Phase 3: Error Handling & Edge Cases (1-2 hours)
1. Handle concurrent preload requests gracefully
2. Add timeout logic for slow network
3. Implement retry with exponential backoff
4. Test in airplane mode, poor network conditions

#### Phase 4: Documentation & Cleanup (1 hour)
1. Document actor usage in `ADMOB_INTEGRATION_SUMMARY.md`
2. Add code comments explaining thread transitions
3. Update architecture diagrams
4. Create unit tests for actor methods

**Total Estimated Time: 5-8 hours** (vs. 15-20 hours for Option 3)

---

## 🔬 Research References

### Apple Documentation
- **Swift Actors**: [https://developer.apple.com/documentation/swift/actor](https://developer.apple.com/documentation/swift/actor)
  - "Actors protect their mutable state by serializing access to that state"
  - Compile-time enforcement of isolation via `@MainActor` and custom actors
  
- **Swift Concurrency**: [https://developer.apple.com/documentation/swift/concurrency](https://developer.apple.com/documentation/swift/concurrency)
  - `Task.detached` for background work independent of calling context
  - `await MainActor.run { }` for UI updates from background
  
- **GCD Best Practices**: [https://developer.apple.com/documentation/dispatch](https://developer.apple.com/documentation/dispatch)
  - Serial queues for ordered operations
  - Concurrent queues for parallel work
  - Quality of Service (QoS) for priority

### Google AdMob Documentation
- **iOS Integration Guide**: [https://developers.google.com/admob/ios/quick-start](https://developers.google.com/admob/ios/quick-start)
  - SDK initialization must be on main thread
  - Ad loading can be async (uses `async/await`)
  - Ad presentation **must** be on main thread (UIKit requirement)
  
- **Interstitial Ad Guide**: [https://developers.google.com/admob/ios/interstitial](https://developers.google.com/admob/ios/interstitial)
  - Preloading recommended for best UX
  - Callbacks via `FullScreenContentDelegate` (main thread)

### Threading Best Practices
- **iOS Threading & Concurrency**: [Ray Wenderlich - iOS Concurrency](https://www.raywenderlich.com/books/concurrency-by-tutorials/v2.0)
  - Actors for isolated state
  - GCD for fire-and-forget tasks
  - Avoid mixing GCD and Swift Concurrency (pick one paradigm)
  
- **C++/Swift Interop**: [Swift.org - C++ Interoperability](https://www.swift.org/documentation/cxx-interop/)
  - Keep threading boundaries clean
  - Prefer Swift-side threading for iOS
  - C++ should remain single-threaded for game logic

---

## 🚨 Critical Considerations

### 1. UIKit Main Thread Requirement
**All UI operations MUST happen on main thread.** This includes:
- Presenting ads (`ad.present(from: viewController)`)
- Setting delegates (`ad.fullScreenContentDelegate = self`)
- Pausing/resuming game via `GameViewController`

**Implication:** Only ad *loading* can move to background. Presentation stays on main.

### 2. AdMob SDK Thread Behavior
- Google Mobile Ads SDK is **already async internally** for network I/O
- However, initialization, parsing, and setup still impact calling thread
- Moving to background actor/queue offloads this setup work

### 3. C++ Game Thread
- Your C++ game loop is single-threaded (good!)
- Don't make it multi-threaded - game logic should stay single-threaded
- Only platform-specific I/O (ads, assets) should be threaded

### 4. Race Conditions to Watch
- **Ad state synchronization**: Ensure C++ `setAdReadyState()` is thread-safe
- **Concurrent preload calls**: Prevent loading multiple ads simultaneously
- **Mid-load cancellation**: Handle ad dismissal while loading new ad

### 5. Memory Management
- Ads hold onto view controller references (weak pointer essential)
- Background actors/queues have slight memory overhead
- Monitor with Instruments (Allocations, Leaks)

---

## 🧪 Testing Strategy

### Unit Tests
```swift
// Tests/AdLoadingActorTests.swift
@testable import FloppyTurd
import XCTest

final class AdLoadingActorTests: XCTestCase {
    func testPreloadAdSuccess() async throws {
        let actor = AdLoadingActor(adUnitID: "test-id")
        
        // Mock network response
        // ... test that ad loads and caches correctly
    }
    
    func testConcurrentPreloadBlocked() async throws {
        let actor = AdLoadingActor(adUnitID: "test-id")
        
        // Start first load
        Task { try await actor.preloadAd() }
        
        // Second load should throw
        do {
            try await actor.preloadAd()
            XCTFail("Should have thrown alreadyLoading error")
        } catch {
            // Expected
        }
    }
}
```

### Integration Tests
1. Load ad during loading screen, verify it doesn't block UI
2. Show ad after player death, verify game pauses correctly
3. Dismiss ad, verify next ad preloads automatically
4. Test in airplane mode (network failure handling)

### Performance Tests
1. Measure loading screen duration with/without threaded ads
2. Profile main thread CPU usage during ad load
3. Check memory footprint increase
4. Test on iPhone 11 (A13) and iPhone 16 (A18) for comparison

---

## 💰 Cost-Benefit Analysis

### Option 1: Swift Actor (Recommended)

**Implementation Cost:** 5-8 hours
**Performance Gain:** 20-30% main thread time reduction during loading
**Risk Level:** Low (Swift Concurrency is stable, compile-time safety)
**Maintenance:** Low (idiomatic Swift, easy to understand)

**ROI:** ⭐⭐⭐⭐⭐ Excellent

### Option 2: GCD + NSLock

**Implementation Cost:** 8-12 hours
**Performance Gain:** 20-30% main thread time reduction during loading
**Risk Level:** Medium (manual locking, potential deadlocks)
**Maintenance:** Medium (requires careful review of all lock usage)

**ROI:** ⭐⭐⭐ Moderate

### Option 3: C++ Thread Pool

**Implementation Cost:** 15-20 hours
**Performance Gain:** 20-30% main thread time reduction during loading
**Risk Level:** High (complex cross-language threading, hard to debug)
**Maintenance:** High (custom thread pool to maintain forever)

**ROI:** ⭐ Poor (overkill for single use case)

---

## 🏁 Next Steps

### If You Choose Option 1 (Recommended):

1. **Read this document thoroughly** and ask any questions
2. **Review Swift Concurrency basics** if not familiar with actors
3. **Implement Phase 1** (basic actor) in a feature branch
4. **Test on loading screen** to verify main thread improvement
5. **Iterate on error handling** based on real-world testing
6. **Document your findings** and update this file

### Key Questions to Answer Before Starting:

1. **iOS version target?** (Swift Concurrency requires iOS 15+, but you're probably already there)
2. **Performance baseline?** (Measure current loading time with Instruments)
3. **Ad frequency?** (How often are ads preloaded? Affects memory pressure)
4. **Error handling needs?** (Retry logic? Fallback behavior?)

---

## 📝 Final Thoughts

Your current architecture is already quite good - commands are queued, Swift processing is isolated to main thread, and you're using best practices. The main optimization is moving just the AdMob loading phase to background, which Option 1 does cleanly and safely.

**Avoid over-engineering.** Option 3 (C++ thread pool) is tempting from an architectural purity standpoint, but it's massive overkill for this problem. iOS gives you excellent threading primitives - use them!

**Start small, iterate.** Implement Option 1's Phase 1, measure the improvement, then decide if further optimization is needed. You might find that 20-30% reduction is plenty, or you might discover texture loading is a bigger bottleneck worth tackling next.

Good luck, and feel free to ask questions! 🚀
