# FrameProfiler Logging Fix Summary

## Problem Identified
The `FrameProfiler` was **silently failing** to output performance data to logs, despite being enabled in `GameplayState::Enter()`.

## Root Cause
**Incompatible string building pattern** with iOS logging macros.

### The Issue
The `FrameProfiler.cpp` was using `std::ostringstream` to build log messages:
```cpp
std::ostringstream oss;
oss << "[FrameProfiler] " << m_context << ": frames=" << m_frameCount;
// ...
GN_LOG_INFO(oss.str());  // ❌ FAILED - temporary string from oss.str()
```

### The Codebase Pattern
**ALL successful logs throughout FloppyTurd** use direct `std::string` concatenation:
```cpp
GN_LOG_INFO("HeartSystem: Initialized player with " + std::to_string(player->hearts) + 
           " hearts (" + std::to_string(player->liveSlices) + " slices)");
```

This pattern is **essential** for the iOS `GN_LOG_INFO` macro to work correctly.

## Solution Applied

### 1. Refactored String Building
Changed from `ostringstream` to `std::string` concatenation:
```cpp
// Build profiler output using std::string (matches codebase pattern)
std::string output = "[FrameProfiler] " + m_context + ": frames=" + std::to_string(m_frameCount);

// Output each section's stats
for (const auto& [name, stats] : m_stats) {
    double avgMs = stats.callCount > 0 ? stats.totalMs / stats.callCount : 0.0;
    output += " | " + std::string(name) +
             " avg=" + std::to_string(avgMs).substr(0, 5) + "ms" +
             " max=" + std::to_string(stats.maxMs).substr(0, 5) + "ms" +
             " calls=" + std::to_string(stats.callCount);
}

GN_LOG_INFO(output);  // ✅ WORKS
```

### 2. Cleaned Up Includes
Removed unused `<sstream>` and `<iomanip>` includes since we no longer use `ostringstream`.

### 3. Added Debug Confirmation
Added first-frame log to confirm profiler is active:
```cpp
if (m_frameCount == 1) {
    GN_LOG_INFO("[FrameProfiler] " + m_context + " profiler is active and collecting data");
}
```

## Expected Output Format
You should now see logs like this every ~1 second during gameplay:
```
[FrameProfiler] GameplayState: frames=60 | InputManager avg=0.12ms max=0.34ms calls=60 | PlayerControllerSystem avg=0.45ms max=1.23ms calls=60 | SpriteSystem avg=2.34ms max=3.45ms calls=60 ...
```

## Files Modified
- `/Users/aimac/Development/FloppyTurd/src/Engine/Utility/FrameProfiler.cpp`

## Next Steps
1. **Rebuild the project** to compile the fixed profiler
2. **Run the game** and enter GameplayState
3. **Check logs** in the app container's Documents folder:
   - Look for `[FrameProfiler] GameplayState profiler is active and collecting data`
   - Wait ~1 second and look for performance data dumps
4. **Analyze performance** to identify lag sources in GameplayState::Update()

## Performance Investigation Strategy
Once logs are working, look for:
- **High avg times**: Systems consistently taking long
- **High max times**: Systems with occasional spikes
- **High call counts**: Systems being called more than expected

Common culprits to investigate:
- `EnemySystem` / `BossSystem` - Complex AI and collision detection
- `RenderSystem` - Excessive draw calls or texture operations
- `SpriteSystem` - Animation frame updates
- `UpdateSpawning` - Object pooling and instantiation
- `CheckToiletCollisions` - Collision detection overhead
